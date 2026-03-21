/* SPDX-License-Identifier: GPL-2.0 */
/*
 * scai_fpgaqspi.c
 *
 * SCAI FPGA QSPI controller.
 *
 */
/*
 * Derived from U-Boot drivers/spi/scai_fpgaqspi.c
 */
#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//#include <stdbool.h>

#include "scai_fpgaqspi.h"
#include "spi-mem.h"
#include "nand.h"
#include "spinand.h"

#define SCAI_NAND_FIFO_TIMEOUT 100
#define SCAI_NAND_FIFO_LENGTH  64

// Constants for packing a byte into a 32-bit word for the hardware.
// This is required if the hardware expects the byte in the MSB position.
#define SCAI_QSPI_FIFO_BYTE_SHIFT   24
#define SCAI_QSPI_FIFO_TX_BYTE_MASK 0xFF000000
#define SCAI_QSPI_FIFO_RX_BYTE_MASK 0x000000FF

/* --- SCAI QSPI Controller Register Offsets --- */
#define SCAI_QSPI_REG_DATA          0x00
#define SCAI_QSPI_REG_CTRL1         0x04
#define SCAI_QSPI_REG_CTRL2         0x08
#define SCAI_QSPI_REG_CTRL3         0x0C

#define SCAI_QSPI_REG_STATUS1       0x04
#define SCAI_QSPI_REG_STATUS2       0x08

/* --- SCAI QSPI Controller CTRL1 Register Bits --- */
#define CTRL1_CHIP_ENABLE           BIT(0)
#define CTRL1_NWP                   BIT(1)
#define CTRL1_RESET                 BIT(2)
#define CTRL1_DATA_MODE_WORD        BIT(3) /* 0 = Byte, 1 = Word */
#define CTRL1_LANE_WIDTH_X4         BIT(4) /* 0 = x1, 1 = x4 */
#define CTRL1_START                 BIT(9)
/* Count in bytes or words depending on CTRL1_DATA_MODE_WORD */
#define CTRL1_TX_COUNT(n)           (((n) & 0x7FF) << 10)
/* Count in bytes or words depending on CTRL1_DATA_MODE_WORD */
#define CTRL1_RX_COUNT(n)           (((n) & 0x7FF) << 21)

/* --- SCAI QSPI Controller Status2 Register Bits --- */
#define STATUS2_RX_FIFO_FULL         BIT(0)
#define STATUS2_RX_FIFO_EMPTY		 BIT(1)
#define STATUS2_RX_FIFO_RDCNT_MASK   0x7F
#define STATUS2_RX_FIFO_RDCNT_SHIFT  2
#define STATUS2_RX_FIFO_WrCnt_MASK   0x7F
#define STATUS2_RX_FIFO_WrCnt_SHIFT  9
#define STATUS2_TX_FIFO_FULL         BIT(16)
#define STATUS2_TX_FIFO_EMPTY        BIT(17)
#define STATUS2_TX_FIFO_RDCNT_MASK   0x7F
#define STATUS2_TX_FIFO_RDCNT_SHIFT  18
#define STATUS2_TX_FIFO_WRCNT_MASK   0x7F
#define STATUS2_TX_FIFO_WRCNT_SHIFT  25

/* --- SCAI QSPI Controller STATUS1 Register Bits --- */
#define STATUS1_IDLE                BIT(0)


/*
 * GPIO definitions based on scai_fpga_platform.h from HSS.
 */
#define GPIO_REG_WDATA_OFFSET   0x00
#define GPIO_REG_RDATA_OFFSET   0x04
#define GPIO1_ENA_SS1_MASK      BIT(4)
#define GPIO2_ENA_SS2_MASK      BIT(0)

#define TIMEOUT_MS             (1000 * 500)

#define MAX_DATA_CMD_LEN       0x440

typedef union {
    uint32_t u32;
    struct {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
    } b;
} uint32_t_ex;

static uint32_t __swap32(uint32_t in) {
  uint32_t_ex out = (uint32_t_ex)in;
  uint32_t swapped =
    ((uint32_t)out.b.b0 << 24)
    | ((uint32_t)out.b.b1 << 16)
    | ((uint32_t)out.b.b2 << 8)
    | ((uint32_t)out.b.b3);

  return swapped;
};

static void writel(uint32_t value, uintptr_t reg) {
    // mHSS_DEBUG_PRINTF(LOG_ERROR, "REGW 0x%08x\t=\t0x%08x\n", reg, value);
    *(volatile uint32_t*)reg = value;
}

static uint32_t readl(uintptr_t reg) {
    uint32_t value = *(volatile uint32_t*)reg;
    // mHSS_DEBUG_PRINTF(LOG_ERROR, "REGR 0x%08x\t=\t0x%08x\n", reg, value);
    return value;
}

/**
 * struct scai_fpgaqspi_priv - Private driver data structure
 */
static void scai_fpgaqspi_set_power(struct scai_fpgaqspi_priv *p,
		bool enable)
{
	uint32_t val1 = 0, val2 = 0;

	if (!p->gpio1_regs || !p->gpio2_regs) {
		mHSS_DEBUG_PRINTF(LOG_NORMAL,
				"WARN: SCAI NAND: GPIO registers not mapped\n");
		return;
	}

	/* Read current GPIO state if needed
	 * val1 = readl(p->gpio1_regs + GPIO_REG_RDATA_OFFSET);
	 * val2 = readl(p->gpio2_regs + GPIO_REG_RDATA_OFFSET);
	 */
	if (enable) {
		val1 |= GPIO1_ENA_SS1_MASK;
		val2 |= GPIO2_ENA_SS2_MASK;
	} else {
		val1 &= ~GPIO1_ENA_SS1_MASK;
		val2 &= ~GPIO2_ENA_SS2_MASK;
	}

	writel(val1, p->gpio1_regs + GPIO_REG_WDATA_OFFSET);
	writel(val2, p->gpio2_regs + GPIO_REG_WDATA_OFFSET);
}

static int scai_fpgaqspi_wait_for_ready(struct scai_fpgaqspi_priv *p)
{
	unsigned long count = 0;
	uint32_t status;

	do {
		status = readl(p->regs + SCAI_QSPI_REG_STATUS1);
		if (status & STATUS1_IDLE)
			return 0;

		//udelay(1);
		count++;
	} while (count < TIMEOUT_MS);

	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s: timeout 0x%08X\n", __func__, status);
	return -ETIMEDOUT;
}

static void scai_fpgaqspi_set_operate_mode(struct scai_fpgaqspi_priv *p,
		bool word)
{
	uint32_t ctrl = p->ctrl1_sw_copy;
	ctrl &= ~(CTRL1_LANE_WIDTH_X4 | CTRL1_DATA_MODE_WORD);

	if (word)
		ctrl |= (CTRL1_LANE_WIDTH_X4 | CTRL1_DATA_MODE_WORD);

	p->ctrl1_sw_copy = ctrl;
}

static int scai_fpgaqspi_write_op(struct scai_fpgaqspi_priv *p, bool word)
{
	uint32_t data, status;
	int err = 0;

	if (word) {
		while (p->tx_len) {
			do {
				status = readl(p->regs + SCAI_QSPI_REG_STATUS2);
			} while (status & STATUS2_TX_FIFO_FULL);

			data = *(uint32_t *)p->tx_buf;
			p->tx_buf += 4;
			p->tx_len -= 4;
#if 0
			mHSS_DEBUG_PRINTF(LOG_NORMAL,
					"%s-word: data(0x%08X)\n", __func__, data);
#endif
			writel(data, p->regs + SCAI_QSPI_REG_DATA);
		}
	} else {
		while (p->tx_len--) {
			do {
				status = readl(p->regs + SCAI_QSPI_REG_STATUS2);
			} while (status & STATUS2_TX_FIFO_FULL);

			data =  (uint32_t)((*p->tx_buf <<
						SCAI_QSPI_FIFO_BYTE_SHIFT) & SCAI_QSPI_FIFO_TX_BYTE_MASK);
			data |= ~SCAI_QSPI_FIFO_TX_BYTE_MASK;

#if 0
			mHSS_DEBUG_PRINTF(LOG_NORMAL,
					"%s-byte: data(0x%08X)\n", __func__, data);
#endif
			writel(data, p->regs + SCAI_QSPI_REG_DATA);
			p->tx_buf++;
		}
	}

	return err;
}

static int scai_fpgaqspi_read_op(struct scai_fpgaqspi_priv *p, bool word)
{
	uint32_t data, status;

	if (!p->rx_len)
		return -1;

	if (word) {

		while (p->rx_len) {
			do {
				status = readl(p->regs + SCAI_QSPI_REG_STATUS2);
			} while (status & STATUS2_RX_FIFO_EMPTY);

			data = readl(p->regs + SCAI_QSPI_REG_DATA);
			data = __swap32(data);
#if 0
			mHSS_DEBUG_PRINTF(LOG_NORMAL,
					"%s-word: data(0x%08X)\n", __func__, data);
#endif
			*(uint32_t *)p->rx_buf = data;
			p->rx_buf += 4;
			p->rx_len -= 4;
		}
	} else {
		while (p->rx_len--) {
			do {
				status = readl(p->regs + SCAI_QSPI_REG_STATUS2);
			} while (status & STATUS2_RX_FIFO_EMPTY);

			data = readl(p->regs + SCAI_QSPI_REG_DATA);
#if 0
			mHSS_DEBUG_PRINTF(LOG_NORMAL,
					"%s-byte: data(0x%08X)\n", __func__, data);
#endif
			*p->rx_buf++ = (data & SCAI_QSPI_FIFO_RX_BYTE_MASK);
		}
	}

	return 0;
}

static int scai_fpgaqspi_start_transaction(struct scai_fpgaqspi_priv *p,
		uint32_t tx_len, uint32_t rx_len)
{
	uint32_t ctrl = p->ctrl1_sw_copy;
	ctrl &= ~(CTRL1_TX_COUNT(0x7FF) | CTRL1_RX_COUNT(0x7FF));
	ctrl |= CTRL1_TX_COUNT(tx_len) | CTRL1_RX_COUNT(rx_len);
	ctrl |= CTRL1_START | CTRL1_CHIP_ENABLE;

	writel(ctrl, p->regs + SCAI_QSPI_REG_CTRL1);
	p->ctrl1_sw_copy = ctrl;
	return 0;
}

static void scai_fpgaqspi_finish_transaction(struct scai_fpgaqspi_priv *p,
		bool keep_ce)
{
	uint32_t ctrl1 = p->ctrl1_sw_copy;

	ctrl1 &= ~(CTRL1_START | CTRL1_TX_COUNT(0x7FF) | CTRL1_RX_COUNT(0x7FF));

	if (!keep_ce) {
		ctrl1 &= ~CTRL1_CHIP_ENABLE;
	}

	writel(ctrl1, p->regs + SCAI_QSPI_REG_CTRL1);
	p->ctrl1_sw_copy = ctrl1;
}

static int __do_exec_word_op(struct scai_fpgaqspi_priv *p,
		const struct spi_mem_op *op)
{
	uint32_t total_tx_words, total_rx_words;
	int err = 0;

	if (op->data.buswidth == 4) {
		total_tx_words = 0;
		total_rx_words = (op->data.nbytes + 3) / 4;
		if (op->data.dir == SPI_MEM_DATA_OUT) {
			total_tx_words = (op->data.nbytes + 3) / 4;
			total_rx_words = 0;
		};

		scai_fpgaqspi_set_operate_mode(p, true);
		scai_fpgaqspi_start_transaction(p, total_tx_words, total_rx_words);

		if (op->data.dir == SPI_MEM_DATA_OUT) {
			p->tx_buf = (uint8_t *)op->data.buf.out;
			p->rx_buf = NULL;
			p->rx_len = 0;
			p->tx_len = op->data.nbytes;
			scai_fpgaqspi_write_op(p, true);
		} else if (op->data.dir == SPI_MEM_DATA_IN) {
			p->tx_buf = NULL;
			p->rx_buf = (uint8_t *)op->data.buf.in;
			p->rx_len = op->data.nbytes;
			p->tx_len = 0;
			scai_fpgaqspi_read_op(p, true);
		}
	}

	scai_fpgaqspi_finish_transaction(p, false);

	return err;
}

static int __do_exec_byte_op(struct scai_fpgaqspi_priv *p,
		const struct spi_mem_op *op)
{
	uint32_t address = op->addr.val;
	uint8_t opcode = op->cmd.opcode;
	uint8_t opaddr[32];
	uint32_t total_tx_bytes, total_rx_bytes;
	int err = 0, i;

	total_tx_bytes = op->cmd.nbytes + op->addr.nbytes + op->dummy.nbytes;
	total_rx_bytes = (op->data.buswidth == 1) ? op->data.nbytes : 0;
	if (op->data.dir == SPI_MEM_DATA_OUT) {
		total_tx_bytes += op->data.nbytes;
		total_rx_bytes -= op->data.nbytes;
	};

	scai_fpgaqspi_set_operate_mode(p, false);
	scai_fpgaqspi_start_transaction(p, total_tx_bytes, total_rx_bytes);

	if (op->cmd.opcode) {
		p->tx_buf = &opcode;
		p->rx_buf = NULL;
		p->tx_len = op->cmd.nbytes;
		p->rx_len = 0;
		scai_fpgaqspi_write_op(p, false);
	}

	if (op->addr.nbytes) {
		memset(opaddr, 0, sizeof(opaddr));
		p->tx_buf = &opaddr[0];
		for (i = 0; i < op->addr.nbytes; i++)
			p->tx_buf[i] = address >> (8 * (op->addr.nbytes - i - 1));

		p->rx_buf = NULL;
		p->tx_len = op->addr.nbytes;
		p->rx_len = 0;
		scai_fpgaqspi_write_op(p, false);
	}

	if (op->dummy.nbytes) {
		memset(opaddr, 0, sizeof(opaddr));

		p->tx_buf = &opaddr[0];
		p->rx_buf = NULL;
		p->tx_len = op->dummy.nbytes;
		p->rx_len = 0;
		scai_fpgaqspi_write_op(p, false);
	}

	if (op->data.nbytes && op->data.buswidth == 1) {
		if (op->data.dir == SPI_MEM_DATA_OUT) {
			p->tx_buf = (uint8_t *)op->data.buf.out;
			p->rx_buf = NULL;
			p->rx_len = 0;
			p->tx_len = op->data.nbytes;
			scai_fpgaqspi_write_op(p, false);
		} else if (op->data.dir == SPI_MEM_DATA_IN) {
			p->tx_buf = NULL;
			p->rx_buf = (uint8_t *)op->data.buf.in;
			p->rx_len = op->data.nbytes;
			p->tx_len = 0;
			scai_fpgaqspi_read_op(p, false);
		}
	}

	scai_fpgaqspi_finish_transaction(p, true);

	return err;
}

#if 0
static void dump_mem_op_info(const struct spi_mem_op *op)
{
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n");
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "==========================\n");
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->cmd.opcode(0x%04X)\n",
			__func__, __LINE__, op->cmd.opcode);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->cmd.nbytes(0x%02X)\n",
			__func__, __LINE__, op->cmd.nbytes);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->cmd.buswidth(0x%02X)\n",
			__func__, __LINE__, op->cmd.buswidth);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->cmd.dtr(0x%02X)\n",
			__func__, __LINE__, op->cmd.dtr);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n");

	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->addr.val(0x%016llX)\n",
			__func__, __LINE__, op->addr.val);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->addr.nbytes(0x%02X)\n",
			__func__, __LINE__, op->addr.nbytes);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->addr.buswidth(0x%02X)\n",
			__func__, __LINE__, op->addr.buswidth);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->addr.dtr(0x%02X)\n",
			__func__, __LINE__, op->addr.dtr);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n");

	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->dummy.buswidth(0x%02X)\n",
			__func__, __LINE__, op->dummy.buswidth);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->dummy.dtr(0x%02X)\n",
			__func__, __LINE__, op->dummy.dtr);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->dummy.nbytes(0x%02X)\n",
			__func__, __LINE__, op->dummy.nbytes);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n");

	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->data.buswidth(0x%02X)\n",
			__func__, __LINE__, op->data.buswidth);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->data.dtr(0x%02X)\n",
			__func__, __LINE__, op->data.dtr);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->data.nbytes(0x%08X)\n",
			__func__, __LINE__, op->data.nbytes);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "==========================\n");
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n");
}
#endif

int scai_fpgaqspi_exec_op(struct spinand_device *spinand,
		const struct spi_mem_op *op)
{
	struct scai_fpgaqspi_priv *p = (struct scai_fpgaqspi_priv *)spinand->priv;
	int err = 0;

#if 0
	dump_mem_op_info(op);
#endif

	err = scai_fpgaqspi_wait_for_ready(p);
	if (err)
		return err;

	err = __do_exec_byte_op(p, op);
	err = __do_exec_word_op(p, op);

	return 0;
}

int scai_fpgaqspi_adjust_op_size(struct spinand_device *spinand,
		struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_DATA_OUT &&
			op->data.nbytes > MAX_DATA_CMD_LEN)
	{
		op->data.nbytes = MAX_DATA_CMD_LEN;
#if 0
		mHSS_DEBUG_PRINTF(LOG_NORMAL, "%s(%d):  op->data.nbytes(0x%08X)\n",
				__func__, __LINE__, op->data.nbytes);
#endif
	}

	return 0;
}

bool scai_fpgaqspi_supports_op(struct spinand_device *spinand,
		const struct spi_mem_op *op)
{
#if 0
	if (!spi_mem_default_supports_op(slave, op))
		return false;
#endif

	if ((op->data.buswidth == 2 || op->data.buswidth == 4) &&
	    (op->cmd.buswidth == 1 && (op->addr.buswidth <= 1)) &&
	    op->data.dir == SPI_MEM_DATA_OUT)
		return false;

	return true;
}

int scai_fpgaqspi_probe(struct spinand_device *spinand)
{
	struct scai_fpgaqspi_priv *p = (struct scai_fpgaqspi_priv *)spinand->priv;
	uint32_t control;

	mHSS_DEBUG_PRINTF(LOG_NORMAL, "SCAI FPGA QSPI REG mapped to VA: %p\n",
			p->regs);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "GPIO1 mapped to VA: %p, Value: 0x%08X\n",
			p->gpio1_regs, readl(p->gpio1_regs + GPIO_REG_RDATA_OFFSET));
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "GPIO2 mapped to VA: %p, Value: 0x%08X\n",
			p->gpio2_regs, readl(p->gpio2_regs + GPIO_REG_RDATA_OFFSET));

	scai_fpgaqspi_set_power(p, true);
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "Enabled FPGA mapped QSPI power via GPIOs\n");

	control = CTRL1_RESET;
	p->ctrl1_sw_copy = control;
	writel(control, p->regs + SCAI_QSPI_REG_CTRL1);

	writel(0, p->regs + SCAI_QSPI_REG_CTRL2);
	writel(BIT(16), p->regs + SCAI_QSPI_REG_CTRL3);

	return 0;
}
