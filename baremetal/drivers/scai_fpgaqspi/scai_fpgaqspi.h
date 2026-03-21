/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __HSS_SCAI_FPGAQSPI_H
#define __HSS_SCAI_FPGAQSPI_H

#ifdef __cplusplus
extern "C" {
#endif

#if 0
#include <stdio.h>
#include <stdlib.h>
#include "spi-mem.h"
#include "spinand.h"
#include "nand.h"
#endif

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */
static inline int generic_fls(int x)
{
  int r = 32;

  if (!x)
    return 0;
  if (!(x & 0xffff0000u)) {
    x <<= 16;
    r -= 16;
  }
  if (!(x & 0xff000000u)) {
    x <<= 8;
    r -= 8;
  }
  if (!(x & 0xf0000000u)) {
    x <<= 4;
    r -= 4;
  }
  if (!(x & 0xc0000000u)) {
    x <<= 2;
    r -= 2;
  }
  if (!(x & 0x80000000u)) {
    x <<= 1;
    r -= 1;
  }
  return r;
}

# define fls generic_fls

#define min_t(type, x, y) ({      \
  type __min1 = (x);      \
  type __min2 = (y);      \
  __min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({      \
  type __max1 = (x);      \
  type __max2 = (y);      \
  __max1 > __max2 ? __max1: __max2; })


//#define offsetof(TYPE, MEMBER)  ((size_t)&((TYPE *)0)->MEMBER)
#define do_div(n,base) ({                                      \
        uint32_t __base = (base);                            \
        uint32_t __rem;                                              \
        __rem = ((uint64_t)(n)) % __base;                    \
        (n) = ((uint64_t)(n)) / __base;                              \
        __rem;                                                  \
 })

#define GENMASK(h, l) \
  (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define BITS_PER_LONG 64
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
//#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define BIT(n) (1UL << (n))

#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)

struct spi_mem_op;
struct spinand_info;
struct spinand_device;
struct nand_memory_organization;
struct erase_info;
struct mtd_oob_region;
struct nand_pos;
struct nand_device;
struct mtd_info;
struct nand_ops;

struct scai_fpgaqspi_priv {
	/* Register base addresses */
	uintptr_t regs;          /* QSPI register base */
	uintptr_t gpio1_regs;    /* GPIO1 register base */
	uintptr_t gpio2_regs;    /* GPIO2 register base */

	/* Software-maintained copies */
	uint32_t ctrl1_sw_copy;  /* Cached CTRL1 register value */

	/* Transfer buffers */
	uint8_t *tx_buf;        /* TX buffer pointer */
	uint8_t *rx_buf;        /* RX buffer pointer */
	int tx_len;        /* TX length */
	int rx_len;        /* RX length */
};

int scai_fpgaqspi_probe(struct spinand_device *spinand);
int scai_fpgaqspi_exec_op(struct spinand_device *spinand,
		const struct spi_mem_op *op);
int scai_fpgaqspi_adjust_op_size(struct spinand_device *spinand,
		struct spi_mem_op *op);
bool scai_fpgaqspi_supports_op(struct spinand_device *spinand,
		const struct spi_mem_op *op);


#if 0
bool nanddev_isbad(struct nand_device *nand, const struct nand_pos *pos);
int nanddev_markbad(struct nand_device *nand, const struct nand_pos *pos);
bool nanddev_isreserved(struct nand_device *nand, const struct nand_pos *pos);
int nanddev_mtd_erase(struct mtd_info *mtd, struct erase_info *einfo);
int nanddev_init(struct nand_device *nand, const struct nand_ops *ops);
int mtd_ooblayout_free(struct mtd_info *mtd, int section,
		       struct mtd_oob_region *oobfree);
int mtd_ooblayout_count_freebytes(struct mtd_info *mtd);
#endif







#ifdef __cplusplus
}
#endif

#endif /* __HSS_SCAI_FPGAQSPI_H */
