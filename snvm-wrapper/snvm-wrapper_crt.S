/*
 * The HSS software is released under the following software license:
 *
 * Copyright 2019-2025 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 */


#include "config.h"
#include <sbi/riscv_asm.h>
#include <sbi/riscv_encoding.h>
#include <sbi/sbi_platform.h>
#include <sbi/sbi_scratch.h>
#include <sbi/sbi_trap.h>

/*
 * These must match the definition in include/hss_types.h!
 *
 */
#define mHSS_COMPRESSED_VERSION_FASTLZ 1u
#define mHSS_COMPRESSED_VERSION_DEFLATE  2u

#define OFFSET_OF(TYPE, FIELD) TYPE##_##FIELD##_##OFFSET
#define HSS_CompressedImage_magic_OFFSET		0u
#define HSS_CompressedImage_version_OFFSET		4u
#define HSS_CompressedImage_headerLength_OFFSET		8u
#define HSS_CompressedImage_headerCrc_OFFSET		16u
#define HSS_CompressedImage_compressedCrc_OFFSET	20u
#define HSS_CompressedImage_originalCrc_OFFSET		24u
#define HSS_CompressedImage_compressedImageLen_OFFSET	32u
#define HSS_CompressedImage_originalImageLen_OFFSET	40u
#define HSS_CompressedImage_hash_OFFSET			48u
#define HSS_CompressedImage_ecdsaSig_OFFSET		80u

/*
 */
	.section .entry, "ax", @progbits
	.attribute unaligned_access, 0
	.attribute stack_align, 16
	.align	3
	.globl	_start

_start:
	la	t0, .early_trap_handler
	csrw	CSR_MTVEC, t0
	li	ra, 0
.reset_regs:
	// flush the instruction cache
	fence.i

	// Reset all registers except ra, a0, a1 and a2
	li	gp,	0
	li	sp,	0
	li	tp,	0
	li	t0,	0
	li	t1,	0
	li	t2,	0
	li	s0,	0
	li	s1,	0
	li	a3,	0
	li	a4,	0
	li	a5,	0
	li	a6,	0
	li	a7,	0
	li	s2,	0
	li	s3,	0
	li	s4,	0
	li	s5,	0
	li	s6,	0
	li	s7,	0
	li	s8,	0
	li	s9,	0
	li	s10,	0
	li	s11,	0
	li	t3,	0
	li	t4,	0
	li	t5,	0
	li	t6,	0
	csrw	CSR_MSCRATCH, 0
  call .wait_for_kwangsu

	li t0, 0x20220100
	jr t0

	.align	3
.early_trap_handler:
	j .early_trap_handler

  .globl  wait_for_kwangsu
  .type wait_for_kwangsu, @function
.wait_for_kwangsu:
  wfi
  li t3, 0
  beqz t3, .wait_for_kwangsu
  ret


