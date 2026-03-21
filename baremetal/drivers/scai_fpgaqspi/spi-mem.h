/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018 Exceet Electronics GmbH
 * Copyright (C) 2018 Bootlin
 *
 * Author:
 *	Peter Pan <peterpandong@micron.com>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 */
/*
 * Derived from U-Boot spi-mem.h
 */

#ifndef __HSS_SPI_MEM_H
#define __HSS_SPI_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#define SPI_MEM_OP_CMD(__opcode, __buswidth)			\
	{							\
		.buswidth = __buswidth,				\
		.opcode = __opcode,				\
		.nbytes = 1,					\
	}

#define SPI_MEM_OP_ADDR(__nbytes, __val, __buswidth)		\
	{							\
		.nbytes = __nbytes,				\
		.val = __val,					\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_ADDR	{ }

#define SPI_MEM_OP_DUMMY(__nbytes, __buswidth)			\
	{							\
		.nbytes = __nbytes,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_DUMMY	{ }

#define SPI_MEM_OP_DATA_IN(__nbytes, __buf, __buswidth)		\
	{							\
		.dir = SPI_MEM_DATA_IN,				\
		.nbytes = __nbytes,				\
		.buf.in = __buf,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_DATA_OUT(__nbytes, __buf, __buswidth)	\
	{							\
		.dir = SPI_MEM_DATA_OUT,			\
		.nbytes = __nbytes,				\
		.buf.out = __buf,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_DATA	{ }

/**
 * enum spi_mem_data_dir - describes the direction of a SPI memory data
 *			   transfer from the controller perspective
 * @SPI_MEM_NO_DATA: no data transferred
 * @SPI_MEM_DATA_IN: data coming from the SPI memory
 * @SPI_MEM_DATA_OUT: data sent the SPI memory
 */
enum spi_mem_data_dir {
	SPI_MEM_NO_DATA,
	SPI_MEM_DATA_IN,
	SPI_MEM_DATA_OUT,
};

/**
 * struct spi_mem_op - describes a SPI memory operation
 * @cmd.nbytes: number of opcode bytes (only 1 or 2 are valid). The opcode is
 *		sent MSB-first.
 * @cmd.buswidth: number of IO lines used to transmit the command
 * @cmd.opcode: operation opcode
 * @cmd.dtr: whether the command opcode should be sent in DTR mode or not
 * @addr.nbytes: number of address bytes to send. Can be zero if the operation
 *		 does not need to send an address
 * @addr.buswidth: number of IO lines used to transmit the address cycles
 * @addr.val: address value. This value is always sent MSB first on the bus.
 *	      Note that only @addr.nbytes are taken into account in this
 *	      address value, so users should make sure the value fits in the
 *	      assigned number of bytes.
 * @addr.dtr: whether the address should be sent in DTR mode or not
 * @dummy.nbytes: number of dummy bytes to send after an opcode or address. Can
 *		  be zero if the operation does not require dummy bytes
 * @dummy.buswidth: number of IO lanes used to transmit the dummy bytes
 * @dummy.dtr: whether the dummy bytes should be sent in DTR mode or not
 * @data.buswidth: number of IO lanes used to send/receive the data
 * @data.dtr: whether the data should be sent in DTR mode or not
 * @data.dtr_bswap16: whether the byte order of 16-bit words is swapped when
		      read or written in DTR mode compared to STR mode.
 * @data.dir: direction of the transfer
 * @data.buf.in: input buffer
 * @data.buf.out: output buffer
 */
struct spi_mem_op {
	struct {
		uint8_t nbytes;
		uint8_t buswidth;
		uint8_t dtr : 1;
		uint16_t opcode;
	} cmd;

	struct {
		uint8_t nbytes;
		uint8_t buswidth;
		uint8_t dtr : 1;
		uint64_t val;
	} addr;

	struct {
		uint8_t nbytes;
		uint8_t buswidth;
		uint8_t dtr : 1;
	} dummy;

	struct {
		uint8_t buswidth;
		uint8_t dtr : 1;
		uint8_t dtr_bswap16 : 1;
		enum spi_mem_data_dir dir;
		unsigned int nbytes;
		/* buf.{in,out} must be DMA-able. */
		union {
			void *in;
			const void *out;
		} buf;
	} data;
};

#define SPI_MEM_OP(__cmd, __addr, __dummy, __data)		\
	{							\
		.cmd = __cmd,					\
		.addr = __addr,					\
		.dummy = __dummy,				\
		.data = __data,					\
	}

#ifdef __cplusplus
}
#endif

#endif /* __HSS_SPI_MEM_H */
