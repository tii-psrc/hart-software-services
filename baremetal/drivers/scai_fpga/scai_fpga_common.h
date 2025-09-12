/*
 * scai_fpga_common.h
 *
 * Common low-level interface for the memory-mapped QSPI controller in the
 * FPGA fabric. This module abstracts direct hardware access for various
 * flash memory drivers.
 *
 */

#ifndef SCAI_FPGA_COMMON_H
#define SCAI_FPGA_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "drivers/mss/mss_qspi/mss_qspi.h" // For mss_qspi_io_format enum

//------------------------------------------------------------------------------
// QSPI Controller Configuration
//------------------------------------------------------------------------------
// Base address of your QSPI controller on the FIC3 bus.
static const uintptr_t QSPI_FPGA_BASE_ADDR = 0x40000000UL;

//------------------------------------------------------------------------------
// QSPI Controller Register Definitions
//------------------------------------------------------------------------------

typedef enum {
    QSPI_DATA_REG_OFFSET  = 0x00,
    QSPI_CTRL1_REG_OFFSET = 0x04,
    QSPI_STATUS_REG_OFFSET  = 0x04
} qspi_reg_offset_t;

// Bitmasks for the Control Register (CTRL1)
typedef enum {
    QSPI_CTRL1_CE_ACTIVATE  = (1u << 0),
    QSPI_CTRL1_BYTE_MODE    = (1u << 3), // 0 = Word, 1 = Byte
    QSPI_CTRL1_QUAD_MODE    = (1u << 4), // 0 = x1, 1 = x4
    QSPI_CTRL1_START_OP     = (1u << 9)
} qspi_ctrl1_mask_t;

// Bit shifts for the Control Register (CTRL1)
typedef enum {
    QSPI_CTRL1_TX_COUNT_SHIFT = 10,
    QSPI_CTRL1_RX_COUNT_SHIFT = 21
} qspi_ctrl1_shift_t;

// Bitmask for the Status Register
static const uint32_t QSPI_STATUS_IDLE_FLAG = (1u << 0);

//------------------------------------------------------------------------------
// Public Function Prototypes
//------------------------------------------------------------------------------

/**
 * @brief Initializes the common QSPI interface module.
 * @param io_format The desired I/O format (e.g., MSS_QSPI_NORMAL or MSS_QSPI_QUAD_FULL).
 */
void QSPI_FPGA_IF_init(mss_qspi_io_format io_format);

/**
 * @brief Gets the current I/O format.
 * @return The configured mss_qspi_io_format.
 */
mss_qspi_io_format QSPI_FPGA_IF_get_io_format(void);

/**
 * @brief Waits for the QSPI controller's state machine to become idle.
 * @return true if idle is reached within the timeout, false otherwise.
 */
bool QSPI_FPGA_IF_wait_controller_idle(void);

/**
 * @brief A versatile low-level helper for most QSPI transactions.
 * @param tx_buffer Pointer to the transmit buffer.
 * @param tx_len Number of bytes to transmit.
 * @param rx_buffer Pointer to the receive buffer.
 * @param rx_len Number of bytes to receive.
 * @param format The I/O format for this specific transfer.
 * @param keep_ce_active Set to true to keep Chip Enable asserted after the transfer.
 */
void QSPI_FPGA_IF_transfer(const uint8_t* tx_buffer, uint32_t tx_len, uint8_t* rx_buffer, uint32_t rx_len, mss_qspi_io_format format, bool keep_ce_active);

#endif // SCAI_FPGA_COMMON_H
