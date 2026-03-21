// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016-2017 Micron Technology, Inc.
 *
 * Authors:
 *	Peter Pan <peterpandong@micron.com>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 */
/*
 * Derived from U-Boot drivers/mtd/nand/spi/core.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "list.h"
#include "nand.h"
#include "spi-mem.h"
#include "spinand.h"
#include "scai_fpgaqspi.h"
#include "mtd-abi.h"
#include "mtd.h"
#include "nandcore.h"
#include "mtdcore.h"

#include "hss_debug.h"

#if 0
static struct spinand_device *nand_to_spinand(struct nand_device *nand)
{
	return container_of(nand, struct spinand_device, base);
}
#endif

static uint8_t g_scratchbuf[SPINAND_MAX_ID_LEN];
static uint8_t g_databuf[2048+64]; //W25N01GV hard coded
static uint8_t g_cfg_cache[2]; //W25N01GV hard coded







#if 0
static int spinand_manufacturer_detect(struct spinand_device *spinand)
{
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(spinand_manufacturers); i++) {
		ret = spinand_manufacturers[i]->ops->detect(spinand);
		if (ret > 0) {
			spinand->manufacturer = spinand_manufacturers[i];
			return 0;
		} else if (ret < 0) {
			return ret;
		}
	}

	return -524;
}
#endif


static void spinand_cache_op_adjust_colum(struct spinand_device *spinand,
					  const struct nand_page_io_req *req,
					  uint16_t *column)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int shift;

	if (nand->memorg.planes_per_lun < 2)
		return;

	/* The plane number is passed in MSB just above the column address */
	shift = fls(nand->memorg.pagesize);
	*column |= req->pos.plane << shift;
}

static int spinand_read_reg_op(struct spinand_device *spinand,
		uint8_t reg, uint8_t *val)
{
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(reg, spinand->scratchbuf);
	int ret;

	ret = scai_fpgaqspi_exec_op(spinand, &op);
	if (ret)
		return ret;

	*val = spinand->scratchbuf[0];
	return 0;
}

int spinand_write_reg_op(struct spinand_device *spinand, uint8_t reg, uint8_t val)
{
	struct spi_mem_op op = SPINAND_SET_FEATURE_OP(reg,
						      spinand->scratchbuf);

	*spinand->scratchbuf = val;
	return scai_fpgaqspi_exec_op(spinand, &op);
//	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_read_status(struct spinand_device *spinand, uint8_t *status)
{
	return spinand_read_reg_op(spinand, REG_STATUS, status);
}

int spinand_get_cfg(struct spinand_device *spinand, uint8_t *cfg)
{
//	struct nand_device *nand = spinand_to_nand(spinand);

	*cfg = spinand->cfg_cache[spinand->cur_target];
	return 0;
}

int spinand_set_cfg(struct spinand_device *spinand, uint8_t cfg)
{
	//struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	if (spinand->cfg_cache[spinand->cur_target] == cfg)
		return 0;

	ret = spinand_write_reg_op(spinand, REG_CFG, cfg);
	if (ret)
		return ret;

	spinand->cfg_cache[spinand->cur_target] = cfg;
	return 0;
}

/**
 * spinand_upd_cfg() - Update the configuration register
 * @spinand: the spinand device
 * @mask: the mask encoding the bits to update in the config reg
 * @val: the new value to apply
 *
 * Update the configuration register.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_upd_cfg(struct spinand_device *spinand, uint8_t mask, uint8_t val)
{
	int ret;
	uint8_t cfg;

	ret = spinand_get_cfg(spinand, &cfg);
	if (ret)
		return ret;

	cfg &= ~mask;
	cfg |= val;

	return spinand_set_cfg(spinand, cfg);
}

/**
 * spinand_select_target() - Select a specific NAND target/die
 * @spinand: the spinand device
 * @target: the target/die to select
 *
 * Select a new target/die. If chip only has one die, this function is a NOOP.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_select_target(struct spinand_device *spinand, unsigned int target)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	if (target >= nand->memorg.ntargets)
		return -EINVAL;

	if (spinand->cur_target == target)
		return 0;

	if (nand->memorg.ntargets == 1) {
		spinand->cur_target = target;
		return 0;
	}

	ret = spinand->select_target(spinand, target);
	if (ret)
		return ret;

	spinand->cur_target = target;
	return 0;
}

static int spinand_init_cfg_cache(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int target;
	int ret;

	spinand->cfg_cache = g_cfg_cache;
	if (!spinand->cfg_cache)
		return -ENOMEM;

	for (target = 0; target < nand->memorg.ntargets; target++) {
		ret = spinand_select_target(spinand, target);
		if (ret)
			return ret;

		/*
		 * We use spinand_read_reg_op() instead of spinand_get_cfg()
		 * here to bypass the config cache.
		 */
		ret = spinand_read_reg_op(spinand, REG_CFG,
					  &spinand->cfg_cache[target]);
		if (ret)
			return ret;
	}

	return 0;
}

static int spinand_init_quad_enable(struct spinand_device *spinand)
{
	bool enable = false;

	if (!(spinand->flags & SPINAND_HAS_QE_BIT))
		return 0;

	if (spinand->op_templates.read_cache->data.buswidth == 4 ||
	    spinand->op_templates.write_cache->data.buswidth == 4 ||
	    spinand->op_templates.update_cache->data.buswidth == 4)
		enable = true;

	return spinand_upd_cfg(spinand, CFG_QUAD_ENABLE,
			       enable ? CFG_QUAD_ENABLE : 0);
}

static int spinand_ecc_enable(struct spinand_device *spinand,
			      bool enable)
{
	return spinand_upd_cfg(spinand, CFG_ECC_ENABLE,
			       enable ? CFG_ECC_ENABLE : 0);
}

static int spinand_write_enable_op(struct spinand_device *spinand)
{
	struct spi_mem_op op = SPINAND_WR_EN_DIS_OP(true);

	return scai_fpgaqspi_exec_op(spinand, &op);
}

static int spinand_load_page_op(struct spinand_device *spinand,
				const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int row = nanddev_pos_to_row(nand, &req->pos);
	struct spi_mem_op op = SPINAND_PAGE_READ_OP(row);

	return scai_fpgaqspi_exec_op(spinand, &op);
}

static int spinand_read_from_cache_op(struct spinand_device *spinand,
				      const struct nand_page_io_req *req)
{
	struct spi_mem_op op = *spinand->op_templates.read_cache;
	struct nand_device *nand = spinand_to_nand(spinand);
	struct mtd_info *mtd = nanddev_to_mtd(nand);
	struct nand_page_io_req adjreq = *req;
	unsigned int nbytes = 0;
	void *buf = NULL;
	uint16_t column = 0;
	int ret;

	if (req->datalen) {
		adjreq.datalen = nanddev_page_size(nand);
		adjreq.dataoffs = 0;
		adjreq.databuf.in = spinand->databuf;
		buf = spinand->databuf;
		nbytes = adjreq.datalen;
	}

	if (req->ooblen) {
		adjreq.ooblen = nanddev_per_page_oobsize(nand);
		adjreq.ooboffs = 0;
		adjreq.oobbuf.in = spinand->oobbuf;
		nbytes += nanddev_per_page_oobsize(nand);
		if (!buf) {
			buf = spinand->oobbuf;
			column = nanddev_page_size(nand);
		}
	}

	spinand_cache_op_adjust_colum(spinand, &adjreq, &column);
	op.addr.val = column;

	/*
	 * Some controllers are limited in term of max RX data size. In this
	 * case, just repeat the READ_CACHE operation after updating the
	 * column.
	 */
	while (nbytes) {
		op.data.buf.in = buf;
		op.data.nbytes = nbytes;
		ret = scai_fpgaqspi_adjust_op_size(spinand, &op);
		if (ret)
			return ret;

		ret = scai_fpgaqspi_exec_op(spinand, &op);
		if (ret)
			return ret;

		buf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;
	}

	if (req->datalen)
		memcpy(req->databuf.in, spinand->databuf + req->dataoffs,
		       req->datalen);

	if (req->ooblen) {
		if (req->mode == MTD_OPS_AUTO_OOB)
			mtd_ooblayout_get_databytes(mtd, req->oobbuf.in,
						    spinand->oobbuf,
						    req->ooboffs,
						    req->ooblen);
		else
			memcpy(req->oobbuf.in, spinand->oobbuf + req->ooboffs,
			       req->ooblen);
	}

	return 0;
}

static int spinand_write_to_cache_op(struct spinand_device *spinand,
				     const struct nand_page_io_req *req)
{
	struct spi_mem_op op = *spinand->op_templates.write_cache;
	struct nand_device *nand = spinand_to_nand(spinand);
	struct mtd_info *mtd = nanddev_to_mtd(nand);
	struct nand_page_io_req adjreq = *req;
	unsigned int nbytes = 0;
	void *buf = NULL;
	uint16_t column = 0;
	int ret;

	memset(spinand->databuf, 0xff,
	       nanddev_page_size(nand) +
	       nanddev_per_page_oobsize(nand));

	if (req->datalen) {
		memcpy(spinand->databuf + req->dataoffs, req->databuf.out,
		       req->datalen);
		adjreq.dataoffs = 0;
		adjreq.datalen = nanddev_page_size(nand);
		adjreq.databuf.out = spinand->databuf;
		nbytes = adjreq.datalen;
		buf = spinand->databuf;
	}

	if (req->ooblen) {
		if (req->mode == MTD_OPS_AUTO_OOB)
			mtd_ooblayout_set_databytes(mtd, req->oobbuf.out,
						    spinand->oobbuf,
						    req->ooboffs,
						    req->ooblen);
		else
			memcpy(spinand->oobbuf + req->ooboffs, req->oobbuf.out,
			       req->ooblen);

		adjreq.ooblen = nanddev_per_page_oobsize(nand);
		adjreq.ooboffs = 0;
		nbytes += nanddev_per_page_oobsize(nand);
		if (!buf) {
			buf = spinand->oobbuf;
			column = nanddev_page_size(nand);
		}
	}

	spinand_cache_op_adjust_colum(spinand, &adjreq, &column);

	op = *spinand->op_templates.write_cache;
	op.addr.val = column;

	/*
	 * Some controllers are limited in term of max TX data size. In this
	 * case, split the operation into one LOAD CACHE and one or more
	 * LOAD RANDOM CACHE.
	 */
	while (nbytes) {
		op.data.buf.out = buf;
		op.data.nbytes = nbytes;

		ret = scai_fpgaqspi_adjust_op_size(spinand, &op);
		if (ret)
			return ret;

		ret = scai_fpgaqspi_exec_op(spinand, &op);
		if (ret)
			return ret;

		buf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;

		/*
		 * We need to use the RANDOM LOAD CACHE operation if there's
		 * more than one iteration, because the LOAD operation resets
		 * the cache to 0xff.
		 */
		if (nbytes) {
			column = op.addr.val;
			op = *spinand->op_templates.update_cache;
			op.addr.val = column;
		}
	}

	return 0;
}

static int spinand_program_op(struct spinand_device *spinand,
			      const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int row = nanddev_pos_to_row(nand, &req->pos);
	struct spi_mem_op op = SPINAND_PROG_EXEC_OP(row);

	return scai_fpgaqspi_exec_op(spinand, &op);
}

static int spinand_erase_op(struct spinand_device *spinand,
			    const struct nand_pos *pos)
{
	struct nand_device *nand = &spinand->base;
	unsigned int row = nanddev_pos_to_row(nand, pos);
	struct spi_mem_op op = SPINAND_BLK_ERASE_OP(row);

	return scai_fpgaqspi_exec_op(spinand, &op);
}


static int spinand_wait(struct spinand_device *spinand, uint8_t *s)
{
	unsigned long start, stop;
	uint8_t status;
	int ret;

	start = 0;//get_timer(0);
	stop = 40000;
	do {
		ret = spinand_read_status(spinand, &status);
		if (ret)
			return ret;

		if (!(status & STATUS_BUSY))
			goto out;
	} while (start++ < stop);

	/*
	 * Extra read, just in case the STATUS_READY bit has changed
	 * since our last check
	 */
	ret = spinand_read_status(spinand, &status);
	if (ret)
		return ret;

out:
	if (s)
		*s = status;

	return status & STATUS_BUSY ? -ETIMEDOUT : 0;
}

static int spinand_read_id_op(struct spinand_device *spinand, uint8_t *buf)
{
	struct spi_mem_op op = SPINAND_READID_OP(0, spinand->scratchbuf,
						 SPINAND_MAX_ID_LEN);
	int ret;

	ret = scai_fpgaqspi_exec_op(spinand, &op);
	if (!ret)
		memcpy(buf, spinand->scratchbuf, SPINAND_MAX_ID_LEN);

	return ret;
}

static int spinand_reset_op(struct spinand_device *spinand)
{
	struct spi_mem_op op = SPINAND_RESET_OP;
	int ret;

	ret = scai_fpgaqspi_exec_op(spinand, &op);
	if (ret)
		return ret;

	return spinand_wait(spinand, NULL);
}

static int spinand_lock_block(struct spinand_device *spinand, uint8_t lock)
{
	return spinand_write_reg_op(spinand, REG_BLOCK_LOCK, lock);
}

static int spinand_check_ecc_status(struct spinand_device *spinand,
		uint8_t status)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	if (spinand->eccinfo.get_status)
		return spinand->eccinfo.get_status(spinand, status);

	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case STATUS_ECC_HAS_BITFLIPS:
		/*
		 * We have no way to know exactly how many bitflips have been
		 * fixed, so let's return the maximum possible value so that
		 * wear-leveling layers move the data immediately.
		 */
		return nand->eccreq.strength;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	default:
		break;
	}

	return -EINVAL;
}

static int spinand_read_page(struct spinand_device *spinand,
			     const struct nand_page_io_req *req,
			     bool ecc_enabled)
{
	uint8_t status;
	int ret;

	ret = spinand_load_page_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (ret < 0)
		return ret;

	ret = spinand_read_from_cache_op(spinand, req);
	if (ret)
		return ret;

	if (!ecc_enabled)
		return 0;

	return spinand_check_ecc_status(spinand, status);
}

static int spinand_write_page(struct spinand_device *spinand,
			      const struct nand_page_io_req *req)
{
	uint8_t status;
	int ret;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_write_to_cache_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_program_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_PROG_FAILED))
		ret = -EIO;

	return ret;
}

static int spinand_mtd_read(struct mtd_info *mtd, off_t from,
			    struct mtd_oob_ops *ops)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	unsigned int max_bitflips = 0;
	struct nand_io_iter iter;
	bool enable_ecc = false;
	bool ecc_failed = false;
	int ret = 0;

	if (ops->mode != MTD_OPS_RAW && spinand->eccinfo.ooblayout)
		enable_ecc = true;

#ifndef __UBOOT__
#endif

	nanddev_io_for_each_page(nand, from, ops, &iter) {
		ret = spinand_select_target(spinand, iter.req.pos.target);
		if (ret)
			break;

		ret = spinand_ecc_enable(spinand, enable_ecc);
		if (ret)
			break;

		ret = spinand_read_page(spinand, &iter.req, enable_ecc);
		if (ret < 0 && ret != -EBADMSG)
			break;

		if (ret == -EBADMSG) {
			ecc_failed = true;
			mtd->ecc_stats.failed++;
			ret = 0;
		} else {
			mtd->ecc_stats.corrected += ret;
			max_bitflips = max_t(unsigned int, max_bitflips, ret);
		}

		ops->retlen += iter.req.datalen;
		ops->oobretlen += iter.req.ooblen;
	}

#ifndef __UBOOT__
#endif
	if (ecc_failed && !ret)
		ret = -EBADMSG;

	return ret ? ret : max_bitflips;
}

static int spinand_mtd_write(struct mtd_info *mtd, off_t to,
			     struct mtd_oob_ops *ops)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_io_iter iter;
	bool enable_ecc = false;
	int ret = 0;

	if (ops->mode != MTD_OPS_RAW && mtd->ooblayout)
		enable_ecc = true;

#ifndef __UBOOT__
#endif

	nanddev_io_for_each_page(nand, to, ops, &iter) {
		ret = spinand_select_target(spinand, iter.req.pos.target);
		if (ret)
			break;

		ret = spinand_ecc_enable(spinand, enable_ecc);
		if (ret)
			break;

		ret = spinand_write_page(spinand, &iter.req);
		if (ret)
			break;

		ops->retlen += iter.req.datalen;
		ops->oobretlen += iter.req.ooblen;
	}

#ifndef __UBOOT__
#endif

	return ret;
}

static bool spinand_isbad(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	uint8_t marker[2] = { };
	struct nand_page_io_req req = {
		.pos = *pos,
		.ooblen = sizeof(marker),
		.ooboffs = 0,
		.oobbuf.in = marker,
		.mode = MTD_OPS_RAW,
	};
	int ret;

	ret = spinand_select_target(spinand, pos->target);
	if (ret)
		return ret;

	ret = spinand_read_page(spinand, &req, false);
	if (ret)
		return ret;

#if 0
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "marker[0]: 0x%02X, marker[1]: 0x%02X\n", 
			marker[0], marker[1]);
#endif

	if (marker[0] != 0xff || marker[1] != 0xff)
		return true;

	return false;
}

static int spinand_mtd_block_isbad(struct mtd_info *mtd, off_t offs)
{
	struct nand_device *nand = mtd_to_nanddev(mtd);
#ifndef __UBOOT__
//	struct spinand_device *spinand = nand_to_spinand(nand);
#endif
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
#ifndef __UBOOT__
#endif
	ret = nanddev_isbad(nand, &pos);
#ifndef __UBOOT__
#endif
	return ret;
}

static int spinand_markbad(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	uint8_t marker[2] = { };
	struct nand_page_io_req req = {
		.pos = *pos,
		.ooboffs = 0,
		.ooblen = sizeof(marker),
		.oobbuf.out = marker,
		.mode = MTD_OPS_RAW,
	};
	int ret;

	ret = spinand_select_target(spinand, pos->target);
	if (ret)
		return ret;

	return spinand_write_page(spinand, &req);
}

static int spinand_mtd_block_markbad(struct mtd_info *mtd, off_t offs)
{
	struct nand_device *nand = mtd_to_nanddev(mtd);
#ifndef __UBOOT__
	//struct spinand_device *spinand = nand_to_spinand(nand);
#endif
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
#ifndef __UBOOT__
#endif
	ret = nanddev_markbad(nand, &pos);
#ifndef __UBOOT__
#endif
	return ret;
}

static int spinand_erase(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	uint8_t status;
	int ret;

	ret = spinand_select_target(spinand, pos->target);
	if (ret)
		return ret;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_erase_op(spinand, pos);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_ERASE_FAILED))
		ret = -EIO;

	return ret;
}

static int spinand_mtd_erase(struct mtd_info *mtd,
			     struct erase_info *einfo)
{
#ifndef __UBOOT__
	//struct spinand_device *spinand = mtd_to_spinand(mtd);
#endif
	int ret;

#ifndef __UBOOT__
#endif
	ret = nanddev_mtd_erase(mtd, einfo);
#ifndef __UBOOT__
#endif

	return ret;
}

static int spinand_mtd_block_isreserved(struct mtd_info *mtd, off_t offs)
{
#ifndef __UBOOT__
	//struct spinand_device *spinand = mtd_to_spinand(mtd);
#endif
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
#ifndef __UBOOT__
#endif
	ret = nanddev_isreserved(nand, &pos);
#ifndef __UBOOT__
#endif

	return ret;
}

static const struct nand_ops spinand_ops = {
	.erase = spinand_erase,
	.markbad = spinand_markbad,
	.isbad = spinand_isbad,
};

static const struct spinand_manufacturer *spinand_manufacturers[] = {
	&winbond_spinand_manufacturer,
};

static int spinand_manufacturer_detect(struct spinand_device *spinand)
{
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(spinand_manufacturers); i++) {
		ret = spinand_manufacturers[i]->ops->detect(spinand);
		if (ret > 0) {
			spinand->manufacturer = spinand_manufacturers[i];
			return 0;
		} else if (ret < 0) {
			return ret;
		}
	}

	return -95;
}

static int spinand_manufacturer_init(struct spinand_device *spinand)
{
	if (spinand->manufacturer->ops->init)
		return spinand->manufacturer->ops->init(spinand);

	return 0;
}

static const struct spi_mem_op *
spinand_select_op_variant(struct spinand_device *spinand,
			  const struct spinand_op_variants *variants)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	for (i = 0; i < variants->nops; i++) {
		struct spi_mem_op op = variants->ops[i];
		unsigned int nbytes;
		int ret;

		nbytes = nanddev_per_page_oobsize(nand) +
			 nanddev_page_size(nand);

		while (nbytes) {
			op.data.nbytes = nbytes;
			ret = scai_fpgaqspi_adjust_op_size(spinand, &op);
			if (ret)
				break;

			if (!scai_fpgaqspi_supports_op(spinand, &op))
				break;

			nbytes -= op.data.nbytes;
		}

		if (!nbytes)
			return &variants->ops[i];
	}

	return NULL;
}

/**
 * spinand_match_and_init() - Try to find a match between a device ID and an
 *			      entry in a spinand_info table
 * @spinand: SPI NAND object
 * @table: SPI NAND device description table
 * @table_size: size of the device description table
 *
 * Should be used by SPI NAND manufacturer drivers when they want to find a
 * match between a device ID retrieved through the READ_ID command and an
 * entry in the SPI NAND description table. If a match is found, the spinand
 * object will be initialized with information provided by the matching
 * spinand_info entry.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_match_and_init(struct spinand_device *spinand,
			   const struct spinand_info *table,
			   unsigned int table_size, uint8_t devid)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	for (i = 0; i < table_size; i++) {
		const struct spinand_info *info = &table[i];
		const struct spi_mem_op *op;

		if (devid != info->devid)
			continue;

		nand->memorg = table[i].memorg;
		nand->eccreq = table[i].eccreq;
		spinand->eccinfo = table[i].eccinfo;
		spinand->flags = table[i].flags;
		spinand->select_target = table[i].select_target;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.read_cache);
		if (!op)
			return -95;

		spinand->op_templates.read_cache = op;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.write_cache);
		if (!op)
			return -95;

		spinand->op_templates.write_cache = op;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.update_cache);
		spinand->op_templates.update_cache = op;

		return 0;
	}

	return -95;
}

static int spinand_detect(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	ret = spinand_reset_op(spinand);
	if (ret)
		return ret;

	ret = spinand_read_id_op(spinand, spinand->id.data);
	if (ret)
		return ret;
	
	spinand->id.len = SPINAND_MAX_ID_LEN;

	ret = spinand_manufacturer_detect(spinand);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "raw ID %02x %02x %02x %02x\n",
			spinand->id.data[0], spinand->id.data[1],
			spinand->id.data[2], spinand->id.data[3]);
	if (ret) {
		return ret;
	}

	if (nand->memorg.ntargets > 1 && !spinand->select_target) {
		mHSS_DEBUG_PRINTF(LOG_ERROR,
			"SPI NANDs with more than one die must implement ->select_target()\n");
		return -EINVAL;
	}

	mHSS_DEBUG_PRINTF(LOG_NORMAL,
		 "%s SPI NAND was found.\n", spinand->manufacturer->name);
	mHSS_DEBUG_PRINTF(LOG_NORMAL,
		 "%llu MiB, block size: %u KiB, page size: %u, OOB size: %u\n",
		 nanddev_size(nand) >> 20, nanddev_eraseblock_size(nand) >> 10,
		 nanddev_page_size(nand), nanddev_per_page_oobsize(nand));

	return 0;
}

static int spinand_noecc_ooblayout_ecc(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *region)
{
	return -ERANGE;
}

static int spinand_noecc_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	/* Reserve 2 bytes for the BBM. */
	region->offset = 2;
	region->length = 62;

	return 0;
}

static const struct mtd_ooblayout_ops spinand_noecc_ooblayout = {
	.ecc = spinand_noecc_ooblayout_ecc,
	.rfree = spinand_noecc_ooblayout_free,
};

int spinand_init(struct spinand_device *spinand)
{
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	int ret, i;

	spinand->scratchbuf = g_scratchbuf;
	
	ret = spinand_detect(spinand);
	if (ret)
		return ret;

	spinand->databuf = g_databuf;
	spinand->oobbuf = spinand->databuf + nanddev_page_size(nand);

	ret = spinand_init_cfg_cache(spinand);
	if (ret)
		return ret;

	ret = spinand_init_quad_enable(spinand);
	if (ret)
		return ret;

	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, 0);
	if (ret)
		return ret;

	ret = spinand_manufacturer_init(spinand);
	if (ret) {
		mHSS_DEBUG_PRINTF(LOG_NORMAL,
			"Failed to initialize the SPI NAND chip (err = %d)\n",
			ret);
		return ret;
	}

	/* After power up, all blocks are locked, so unlock them here. */
	for (i = 0; i < nand->memorg.ntargets; i++) {
		ret = spinand_select_target(spinand, i);
		if (ret)
			return ret;

		ret = spinand_lock_block(spinand, BL_ALL_UNLOCKED);
		if (ret)
			return ret;
	}

	ret = nanddev_init(nand, &spinand_ops);
	if (ret)
		return ret;

	/*
	 * Right now, we don't support ECC, so let the whole oob
	 * area is available for user.
	 */
	mtd->_read_oob = spinand_mtd_read;
	mtd->_write_oob = spinand_mtd_write;
	mtd->_block_isbad = spinand_mtd_block_isbad;
	mtd->_block_markbad = spinand_mtd_block_markbad;
	mtd->_block_isreserved = spinand_mtd_block_isreserved;
	mtd->_erase = spinand_mtd_erase;

	if (spinand->eccinfo.ooblayout)
		mtd_set_ooblayout(mtd, spinand->eccinfo.ooblayout);
	else
		mtd_set_ooblayout(mtd, &spinand_noecc_ooblayout);

	ret = mtd_ooblayout_count_freebytes(mtd);
	if (ret < 0)
		return ret;

	mtd->oobavail = ret;

#if 0
	mHSS_DEBUG_PRINTF(LOG_ERROR, "mtd->type : 0x%02X\n", mtd->type);
	mHSS_DEBUG_PRINTF(LOG_ERROR, "mtd->flags : 0x%08X\n", mtd->flags);
	mHSS_DEBUG_PRINTF(LOG_ERROR, "mtd->size : 0x%16X\n", mtd->size);
	mHSS_DEBUG_PRINTF(LOG_ERROR, "mtd->erasesize : 0x%08X\n", mtd->erasesize);
	mHSS_DEBUG_PRINTF(LOG_ERROR, "mtd->writesize : 0x%08X\n", mtd->writesize);
	mHSS_DEBUG_PRINTF(LOG_ERROR, "mtd->oobsize : 0x%08X\n", mtd->oobsize);
	mHSS_DEBUG_PRINTF(LOG_ERROR, "mtd->oobavail : 0x%08X\n", mtd->oobavail);
#endif

	return 0;
}
