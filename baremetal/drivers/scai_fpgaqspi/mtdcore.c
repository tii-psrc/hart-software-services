#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//#include <stdbool.h>

#include "mtdcore.h"
#include "mtd.h"
#include "scai_fpgaqspi.h"

/**
 * mtd_ooblayout_count_bytes - count the number of bytes in a OOB category
 * @mtd: mtd info structure
 * @iter: category iterator
 *
 * Count the number of bytes in a given category.
 *
 * Returns a positive value on success, a negative error code otherwise.
 */
static int mtd_ooblayout_count_bytes(struct mtd_info *mtd,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	struct mtd_oob_region oobregion;
	int section = 0, ret, nbytes = 0;

	while (1) {
		ret = iter(mtd, section++, &oobregion);
		if (ret) {
			if (ret == -ERANGE)
				ret = nbytes;
			break;
		}

		nbytes += oobregion.length;
	}

	return ret;
}

/**
 * mtd_ooblayout_count_freebytes - count the number of free bytes in OOB
 * @mtd: mtd info structure
 *
 * Works like mtd_ooblayout_count_bytes(), except it count free bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_count_freebytes(struct mtd_info *mtd)
{
	return mtd_ooblayout_count_bytes(mtd, mtd_ooblayout_free);
}


/**
 * mtd_ooblayout_free - Get the OOB region definition of a specific free
 *			section
 * @mtd: MTD device structure
 * @section: Free section you are interested in. Depending on the layout
 *	     you may have all the free bytes stored in a single contiguous
 *	     section, or one section per ECC chunk plus an extra section
 *	     for the remaining bytes (or other funky layout).
 * @oobfree: OOB region struct filled with the appropriate free position
 *	     information
 *
 * This function returns free bytes position in the OOB area. If you want
 * to get all the free bytes information, then you should call
 * mtd_ooblayout_free(mtd, section++, oobfree) until it returns -ERANGE.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_free(struct mtd_info *mtd, int section,
		       struct mtd_oob_region *oobfree)
{
	memset(oobfree, 0, sizeof(*oobfree));

	if (!mtd || section < 0)
		return -EINVAL;

	if (!mtd->ooblayout || !mtd->ooblayout->rfree)
		return -524;

	return mtd->ooblayout->rfree(mtd, section, oobfree);
}




/**
 * mtd_ooblayout_find_region - Find the region attached to a specific byte
 * @mtd: mtd info structure
 * @byte: the byte we are searching for
 * @sectionp: pointer where the section id will be stored
 * @oobregion: used to retrieve the ECC position
 * @iter: iterator function. Should be either mtd_ooblayout_free or
 *	  mtd_ooblayout_ecc depending on the region type you're searching for
 *
 * This function returns the section id and oobregion information of a
 * specific byte. For example, say you want to know where the 4th ECC byte is
 * stored, you'll use:
 *
 * mtd_ooblayout_find_region(mtd, 3, &section, &oobregion, mtd_ooblayout_ecc);
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ooblayout_find_region(struct mtd_info *mtd, int byte,
				int *sectionp, struct mtd_oob_region *oobregion,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	int pos = 0, ret, section = 0;

	memset(oobregion, 0, sizeof(*oobregion));

	while (1) {
		ret = iter(mtd, section, oobregion);
		if (ret)
			return ret;

		if (pos + oobregion->length > byte)
			break;

		pos += oobregion->length;
		section++;
	}

	/*
	 * Adjust region info to make it start at the beginning at the
	 * 'start' ECC byte.
	 */
	oobregion->offset += byte - pos;
	oobregion->length -= byte - pos;
	*sectionp = section;

	return 0;
}


/**
 * mtd_ooblayout_set_bytes - put OOB bytes into the oob buffer
 * @mtd: mtd info structure
 * @buf: source buffer to get OOB bytes from
 * @oobbuf: OOB buffer
 * @start: first OOB byte to set
 * @nbytes: number of OOB bytes to set
 * @iter: section iterator
 *
 * Fill the OOB buffer with data provided in buf. The category (ECC or free)
 * is selected by passing the appropriate iterator.
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ooblayout_set_bytes(struct mtd_info *mtd, const uint8_t *buf,
				uint8_t *oobbuf, int start, int nbytes,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	struct mtd_oob_region oobregion;
	int section, ret;

	ret = mtd_ooblayout_find_region(mtd, start, &section,
					&oobregion, iter);

	while (!ret) {
		int cnt;

		cnt = min_t(int, nbytes, oobregion.length);
		memcpy(oobbuf + oobregion.offset, buf, cnt);
		buf += cnt;
		nbytes -= cnt;

		if (!nbytes)
			break;

		ret = iter(mtd, ++section, &oobregion);
	}

	return ret;
}


/**
 * mtd_ooblayout_get_bytes - Extract OOB bytes from the oob buffer
 * @mtd: mtd info structure
 * @buf: destination buffer to store OOB bytes
 * @oobbuf: OOB buffer
 * @start: first byte to retrieve
 * @nbytes: number of bytes to retrieve
 * @iter: section iterator
 *
 * Extract bytes attached to a specific category (ECC or free)
 * from the OOB buffer and copy them into buf.
 *
 * Returns zero on success, a negative error code otherwise.
 */
static int mtd_ooblayout_get_bytes(struct mtd_info *mtd, uint8_t *buf,
				const uint8_t *oobbuf, int start, int nbytes,
				int (*iter)(struct mtd_info *,
					    int section,
					    struct mtd_oob_region *oobregion))
{
	struct mtd_oob_region oobregion;
	int section, ret;

	ret = mtd_ooblayout_find_region(mtd, start, &section,
					&oobregion, iter);

	while (!ret) {
		int cnt;

		cnt = min_t(int, nbytes, oobregion.length);
		memcpy(buf, oobbuf + oobregion.offset, cnt);
		buf += cnt;
		nbytes -= cnt;

		if (!nbytes)
			break;

		ret = iter(mtd, ++section, &oobregion);
	}

	return ret;
}



/**
 * mtd_ooblayout_get_eccbytes - set data bytes into the oob buffer
 * @mtd: mtd info structure
 * @eccbuf: source buffer to get data bytes from
 * @oobbuf: OOB buffer
 * @start: first ECC byte to set
 * @nbytes: number of ECC bytes to set
 *
 * Works like mtd_ooblayout_get_bytes(), except it acts on free bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_set_databytes(struct mtd_info *mtd, const uint8_t *databuf,
				uint8_t *oobbuf, int start, int nbytes)
{
	return mtd_ooblayout_set_bytes(mtd, databuf, oobbuf, start, nbytes,
				       mtd_ooblayout_free);
}

/**
 * mtd_ooblayout_get_databytes - extract data bytes from the oob buffer
 * @mtd: mtd info structure
 * @databuf: destination buffer to store ECC bytes
 * @oobbuf: OOB buffer
 * @start: first ECC byte to retrieve
 * @nbytes: number of ECC bytes to retrieve
 *
 * Works like mtd_ooblayout_get_bytes(), except it acts on free bytes.
 *
 * Returns zero on success, a negative error code otherwise.
 */
int mtd_ooblayout_get_databytes(struct mtd_info *mtd, uint8_t *databuf,
		const uint8_t *oobbuf, int start, int nbytes)
{
	return mtd_ooblayout_get_bytes(mtd, databuf, oobbuf, start, nbytes,
				       mtd_ooblayout_free);
}

int mtd_block_isbad(struct mtd_info *mtd, off_t ofs)
{
	if (ofs < 0 || ofs > mtd->size)
		return -EINVAL;
	if (!mtd->_block_isbad)
		return 0;
	return mtd->_block_isbad(mtd, ofs);
}

static int mtd_check_oob_ops(struct mtd_info *mtd, off_t offs,
			     struct mtd_oob_ops *ops)
{
	/*
	 * Some users are setting ->datbuf or ->oobbuf to NULL, but are leaving
	 * ->len or ->ooblen uninitialized. Force ->len and ->ooblen to 0 in
	 *  this case.
	 */
	if (!ops->datbuf)
		ops->len = 0;

	if (!ops->oobbuf)
		ops->ooblen = 0;

	if (offs < 0 || offs + ops->len > mtd->size)
		return -EINVAL;

	if (ops->ooblen) {
		size_t maxooblen;

		if (ops->ooboffs >= mtd_oobavail(mtd, ops))
			return -EINVAL;

		maxooblen = ((size_t)(mtd_div_by_ws(mtd->size, mtd) -
				      mtd_div_by_ws(offs, mtd)) *
			     mtd_oobavail(mtd, ops)) - ops->ooboffs;
		if (ops->ooblen > maxooblen)
			return -EINVAL;
	}

	return 0;
}

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	if (instr->addr > mtd->size || instr->len > mtd->size - instr->addr)
		return -EINVAL;
	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;
	instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;
	if (!instr->len) {
		instr->state = MTD_ERASE_DONE;
		return 0;
	}
	return mtd->_erase(mtd, instr);
}

int mtd_read_oob(struct mtd_info *mtd, off_t from, struct mtd_oob_ops *ops)
{
	int ret_code;
	ops->retlen = ops->oobretlen = 0;

	ret_code = mtd_check_oob_ops(mtd, from, ops);
	if (ret_code)
		return ret_code;

	/* Check the validity of a potential fallback on mtd->_read */
	if (!mtd->_read_oob && (!mtd->_read || ops->oobbuf))
		return -EOPNOTSUPP;

	if (mtd->_read_oob)
		ret_code = mtd->_read_oob(mtd, from, ops);
	else
		ret_code = mtd->_read(mtd, from, ops->len, &ops->retlen,
				      ops->datbuf);

	/*
	 * In cases where ops->datbuf != NULL, mtd->_read_oob() has semantics
	 * similar to mtd->_read(), returning a non-negative integer
	 * representing max bitflips. In other cases, mtd->_read_oob() may
	 * return -EUCLEAN. In all cases, perform similar logic to mtd_read().
	 */
	if (unlikely(ret_code < 0))
		return ret_code;
	if (mtd->ecc_strength == 0)
		return 0;	/* device lacks ecc */
	return ret_code >= mtd->bitflip_threshold ? -117 : 0;
}

int mtd_write_oob(struct mtd_info *mtd, off_t to,
				struct mtd_oob_ops *ops)
{
	int ret;

	ops->retlen = ops->oobretlen = 0;

	if (!(mtd->flags & MTD_WRITEABLE))
		return -EROFS;

	ret = mtd_check_oob_ops(mtd, to, ops);
	if (ret)
		return ret;

	/* Check the validity of a potential fallback on mtd->_write */
	if (!mtd->_write_oob && (!mtd->_write || ops->oobbuf))
		return -EOPNOTSUPP;

	if (mtd->_write_oob)
		return mtd->_write_oob(mtd, to, ops);
	else
		return mtd->_write(mtd, to, ops->len, &ops->retlen,
				   ops->datbuf);
}

