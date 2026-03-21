// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 Free Electrons
 *
 * Authors:
 *	Boris Brezillon <boris.brezillon@free-electrons.com>
 *	Peter Pan <peterpandong@micron.com>
 */
#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//#include <stdbool.h>

#include "bbt.h"
#include "nand.h"

#include "hss_debug.h"

static unsigned long g_cache[4096] = { 0 };
/**
 * nanddev_bbt_init() - Initialize the BBT (Bad Block Table)
 * @nand: NAND device
 *
 * Initialize the in-memory BBT.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_bbt_init(struct nand_device *nand)
{
#if 0
	unsigned int bits_per_block = fls(NAND_BBT_BLOCK_NUM_STATUS);
	unsigned int nblocks = nanddev_neraseblocks(nand);
	unsigned int nwords = DIV_ROUND_UP(nblocks * bits_per_block,
					   BITS_PER_LONG);

	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  nwords(%d)\n",
			__func__, __LINE__, nwords);
#endif
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	nand->bbt.cache = g_cache;//NULL;//kzalloc(nwords, GFP_KERNEL);
	if (!nand->bbt.cache)
		return -ENOMEM;

	return 0;
}

/**
 * nanddev_bbt_update() - Update a BBT
 * @nand: nand device
 *
 * Update the BBT. Currently a NOP function since on-flash bbt is not yet
 * supported.
 *
 * Return: 0 in case of success, a negative error code otherwise.
 */
int nanddev_bbt_update(struct nand_device *nand)
{
	return 0;
}

/**
 * nanddev_bbt_get_block_status() - Return the status of an eraseblock
 * @nand: nand device
 * @entry: the BBT entry
 *
 * Return: a positive number nand_bbt_block_status status or -%ERANGE if @entry
 *	   is bigger than the BBT size.
 */
int nanddev_bbt_get_block_status(const struct nand_device *nand, 
		unsigned int entry)
{
	unsigned int bits_per_block = fls(NAND_BBT_BLOCK_NUM_STATUS);
	unsigned long *pos = nand->bbt.cache +
			     ((entry * bits_per_block) / BITS_PER_LONG);
	unsigned int offs = (entry * bits_per_block) % BITS_PER_LONG;
	unsigned long status;

	if (entry >= nanddev_neraseblocks(nand))
		return -ERANGE;

	status = pos[0] >> offs;
	if (bits_per_block + offs > BITS_PER_LONG)
		status |= pos[1] << (BITS_PER_LONG - offs);

	return status & GENMASK(bits_per_block - 1, 0);
}

/**
 * nanddev_bbt_set_block_status() - Update the status of an eraseblock in the
 *				    in-memory BBT
 * @nand: nand device
 * @entry: the BBT entry to update
 * @status: the new status
 *
 * Update an entry of the in-memory BBT. If you want to push the updated BBT
 * the NAND you should call nanddev_bbt_update().
 *
 * Return: 0 in case of success or -%ERANGE if @entry is bigger than the BBT
 *	   size.
 */
int nanddev_bbt_set_block_status(struct nand_device *nand,
		unsigned int entry, enum nand_bbt_block_status status)
{
	unsigned int bits_per_block = fls(NAND_BBT_BLOCK_NUM_STATUS);
	unsigned long *pos = nand->bbt.cache +
			     ((entry * bits_per_block) / BITS_PER_LONG);
	unsigned int offs = (entry * bits_per_block) % BITS_PER_LONG;
	unsigned long val = status & GENMASK(bits_per_block - 1, 0);

	if (entry >= nanddev_neraseblocks(nand))
		return -ERANGE;

	pos[0] &= ~GENMASK(offs + bits_per_block - 1, offs);
	pos[0] |= val << offs;

	if (bits_per_block + offs > BITS_PER_LONG) {
		unsigned int rbits = bits_per_block + offs - BITS_PER_LONG;

		pos[1] &= ~GENMASK(rbits - 1, 0);
		pos[1] |= val >> (bits_per_block - rbits);
	}

	return 0;
}



