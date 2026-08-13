/* SPDX-License-Identifier: GPL-2.0 */
#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//#include <stdbool.h>

#include "spi-mem.h"
#include "nand.h"
#include "spinand.h"
#include "mtd.h"
#include "mtdcore.h"
#include "scai_fpgaqspi.h"
#include "scai_fpgaqspi_service.h"
#include "hss_debug.h"

#include "tinycli_hexdump.h"

static struct scai_fpgaqspi_priv w25_priv = {
#if defined(CONFIG_BOARD_SCAI_DPU460) || defined(CONFIG_BOARD_SCAI_DPU250)
    .regs          = 0x40000510,
    .gpio1_regs    = 0x0,
    .gpio2_regs    = 0x0,
#elif defined(CONFIG_BOARD_SCAI_NAVC250) || defined(CONFIG_BOARD_SCAI_NAVC460) 
    .regs          = 0x40000310,
    .gpio1_regs    = 0x40000110,
    .gpio2_regs    = 0x40000120,
#else
    #error "FPGA QSPI doesn't support..."
#endif
    .ctrl1_sw_copy = 0,
    .tx_buf        = NULL,
    .rx_buf        = NULL,
    .tx_len        = 0,
    .rx_len        = 0,
};

static struct spinand_device spinand_device = { 0 };
static struct spinand_device *spinand = &spinand_device;

static struct mtd_info mtd_info = { 0 };
static struct mtd_info *mtd = &mtd_info;

static bool is_initialized = false;
//static struct nand_device nand_device = { 0 };
//static struct nand_device *nand = &nand_device;
//

bool FPGA_Flash_init(void)
{
	int err = 0;

	if (is_initialized)
		return is_initialized;

	struct nand_device *nand = spinand_to_nand(spinand);
	spinand->priv = (void *)&w25_priv;

	nand->mtd = mtd;
	mtd->priv = nand;

	scai_fpgaqspi_probe(spinand);

	err = spinand_init(spinand);
	if (!err)
		is_initialized = true;

	return is_initialized;
}

bool FPGA_Flash_getinfo(uint32_t *writesize, uint32_t *erasesize,
		uint32_t *nandsize)
{
	if (!is_initialized)
		return false;

	*writesize = mtd->writesize;
	*erasesize = mtd->erasesize;
	*nandsize = mtd->size;

	return is_initialized;
}

bool FPGA_Flash_isbad(void)
{
	uint32_t off;

	if (!is_initialized) 
		return false;

	if (!mtd_can_have_bb(mtd)) {
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Only NAND-based devices can have bad blocks\n");
		return false;
	}

	for (off = 0; off < mtd->size; off += mtd->erasesize) {
		if (mtd_block_isbad(mtd, off))
			mHSS_DEBUG_PRINTF(LOG_ERROR, "\t0x%08llx is bad...\n", off);
	}

	return true;
}

static bool mtd_is_aligned_with_min_io_size(struct mtd_info *p_mtd,
		uint64_t len)
{
	return !do_div(len, p_mtd->writesize);
}

static bool mtd_is_aligned_with_block_size(struct mtd_info *p_mtd,
		uint64_t len)
{
	return !do_div(len, p_mtd->erasesize);
}

bool FPGA_Flash_read(uintptr_t dest, uint32_t off, uint64_t len)
{
	struct mtd_oob_ops io_op = {0};
	uint8_t buf[4096] = { 0 };
	uintptr_t cur = dest;
	uint64_t remaining = len;
	size_t copy_len = 0;

	if (!is_initialized) 
		return false;

	if (!mtd_is_aligned_with_min_io_size(mtd, off)) {
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Offset not aligned with a page (0x%x)\n", 
				mtd->writesize);
		return false;
	}

	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Size not on a page boundary (0x%x), rounding from 0x%llx to 0x%llx\n",
				mtd->writesize, remaining, len);
	}

	io_op.mode = MTD_OPS_RAW;
	io_op.len = mtd->writesize;
	io_op.ooblen = mtd->oobsize;
	io_op.datbuf = buf;
	io_op.oobbuf = &buf[mtd->writesize];

	while (len) {
		memset(buf, 0, sizeof(buf));

		mtd_read_oob(mtd, off, &io_op);

		if (cur) {
			copy_len = (remaining < io_op.retlen) ? remaining : io_op.retlen;
			memcpy((void *)cur, buf, copy_len);
		} else {
			HSS_TinyCLI_HexDumpEx((uint8_t *)buf, io_op.len + io_op.ooblen, off);
		}

#if 0
		mHSS_DEBUG_PRINTF(LOG_ERROR, "io_op.retlen : 0x%08X\n", io_op.retlen);
		mHSS_DEBUG_PRINTF(LOG_ERROR, "len : 0x%08X\n", len);
		mHSS_DEBUG_PRINTF(LOG_ERROR, "off : 0x%08X\n", off);
#endif
		if (cur) {
			cur += copy_len;
			remaining -= copy_len;
		}
		off += io_op.retlen;
		len -= io_op.retlen;
	}

	return true;
}

#if 1
bool FPGA_Flash_write(uintptr_t src, uint32_t off, uint64_t len)
{
	struct mtd_oob_ops io_op = {0};
	uint64_t remaining = len;
	uint8_t buf[4096];

	if (!is_initialized)
		return false;

	if (!mtd_is_aligned_with_min_io_size(mtd, off)) {
		len = round_up(len, mtd->writesize);
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Size not on a page boundary (0x%x), rounding from 0x%llx to 0x%llx\n",
				mtd->writesize, remaining, len);
	}

	io_op.mode = MTD_OPS_RAW;
	io_op.len = mtd->writesize;
	io_op.ooblen = 0;
	io_op.oobbuf = NULL;

	while (remaining) {
		size_t write_len = (remaining < mtd->writesize) ?
			remaining : mtd->writesize;

		if (write_len != mtd->writesize) {
			memset(buf, 0xFF, sizeof(buf));
			memcpy(buf, (void *)src, write_len);
			io_op.datbuf = buf;
		} else {
			io_op.datbuf = (uint8_t *)src;
		}

		mtd_write_oob(mtd, off, &io_op);

		off += io_op.retlen;
		src += write_len;
		remaining -= write_len;
	}

	return true;
}
#else
bool FPGA_Flash_write(uintptr_t src, uint32_t off, uint64_t len)
{
	struct mtd_oob_ops io_op = {0};
	uint64_t remaining = len;

	if (!is_initialized) 
		return false;

	if (!mtd_is_aligned_with_min_io_size(mtd, off)) {
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Offset not aligned with a page (0x%x)\n", 
				mtd->writesize);
		return false;
	}
	if (!mtd_is_aligned_with_min_io_size(mtd, len)) {
		len = round_up(len, mtd->writesize);
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Size not on a page boundary (0x%x), rounding from 0x%llx to 0x%llx\n",
				mtd->writesize, remaining, len);
	}

	io_op.mode = MTD_OPS_RAW;
	io_op.len = mtd->writesize;
	io_op.ooblen = 0;//mtd->oobsize;
	io_op.datbuf = (uint8_t *)src;
	io_op.oobbuf = NULL;//&buf[mtd->writesize];

	while (len) {
		mtd_write_oob(mtd, off, &io_op);

		off += io_op.retlen;
		len -= io_op.retlen;
		io_op.datbuf += io_op.retlen;
		//io_op.oobbuf += io_op.oobretlen;
	}

	return true;
}
#endif

bool FPGA_Flash_erase(uint32_t off, uint64_t len)
{
	struct erase_info erase_op = { 0 };

	if (!is_initialized) 
		return false;

	if (!mtd_is_aligned_with_block_size(mtd, off)) {
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Offset not aligned with a block (0x%x)\n", 
				mtd->erasesize);
		return false;
	}

	if (!mtd_is_aligned_with_block_size(mtd, len)) {
		mHSS_DEBUG_PRINTF(LOG_ERROR,
				"Size not a multiple of a block (0x%x)\n", 
				mtd->erasesize);
		return false;
	}

	mHSS_DEBUG_PRINTF(LOG_NORMAL,
			"Erasing 0x%08llx ... 0x%08llx (%d eraseblock(s))\n",
			off, off + len - 1, mtd_div_by_eb(len, mtd));

	erase_op.mtd = mtd;
	erase_op.addr = off;
	erase_op.len = mtd->erasesize;

	while (len) {
		mtd_erase(mtd, &erase_op);

		len -= mtd->erasesize;
		erase_op.addr += mtd->erasesize;
	}

	return true;
}
