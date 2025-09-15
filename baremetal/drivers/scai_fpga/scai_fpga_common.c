/*
 * scai_fpga_common.c
 *
 * Common low-level interface for the memory-mapped QSPI controller in the FPGA fabric.
 * This file contains shared helper functions for different flash memory drivers.
 * Designed to be standalone for HSS integration.
 *
 */

#include "scai_fpga_common.h"

#include "hss_types.h"

// --- Static Global Pointers for Optimized Register Access ---
static volatile uint32_t* g_qspi_data_reg;
static volatile uint32_t* g_qspi_ctrl1_reg;
static volatile const uint32_t* g_qspi_status_reg;
static mss_qspi_io_format g_io_format;
static bool g_is_common_initialized = false;

// Static Helper Functions
static inline bool is_initialized(void) {
    if (!g_is_common_initialized) {
        return false;
    }
    return true;
}

// =============================================================================
// Public API Implementation
// =============================================================================

// Initialize global pointers for optimized access based on the defined base address
void QSPI_FPGA_IF_init(mss_qspi_io_format io_format) {
    if (is_initialized()) {
        // Already initialized, no action needed
        return;
    }

    g_qspi_data_reg    = (uint32_t *)(QSPI_FPGA_BASE_ADDR + QSPI_DATA_REG_OFFSET);
    g_qspi_ctrl1_reg   = (uint32_t *)(QSPI_FPGA_BASE_ADDR + QSPI_CTRL1_REG_OFFSET);
    g_qspi_status_reg  = (const uint32_t *)(QSPI_FPGA_BASE_ADDR + QSPI_STATUS_REG_OFFSET);
    g_io_format = io_format;
    g_is_common_initialized = true;
}

mss_qspi_io_format QSPI_FPGA_IF_get_io_format(void) {
    if (!is_initialized()) {
        return MSS_QSPI_NORMAL;
    }

    return g_io_format;
}

bool QSPI_FPGA_IF_wait_controller_idle(void) {
    if (!is_initialized()) {
        return false;
    }

    // This timeout value is empirical and may need adjustment.
    for (int i = 0; i < 1000; i++) {
        if (*g_qspi_status_reg & QSPI_STATUS_IDLE_FLAG) {
            return true;
        }
    }
    return false;
}

void QSPI_FPGA_IF_transfer(const uint8_t* tx_buffer, uint32_t tx_len, uint8_t* rx_buffer, uint32_t rx_len, mss_qspi_io_format format, bool keep_ce_active) {
    // Basic safety checks
    if (!is_initialized()) {
        return;
    }

    if ((!tx_buffer && tx_len > 0) || (!rx_buffer && rx_len > 0)) {
        return;
    }

    if (format < MSS_QSPI_NORMAL || format > MSS_QSPI_QUAD_FULL) {
        return;
    }

    uint32_t ctrl1_val = *g_qspi_ctrl1_reg;

    // Clear tx/rx counts, format, and WnB (byte mode) bits for a clean slate
    ctrl1_val &= ~((0x7FFu << QSPI_CTRL1_RX_COUNT_SHIFT) 
               | (0x7FFu << QSPI_CTRL1_TX_COUNT_SHIFT) 
               | QSPI_CTRL1_QUAD_MODE 
               | QSPI_CTRL1_BYTE_MODE);

    if (format == MSS_QSPI_QUAD_FULL) {
        ctrl1_val |= QSPI_CTRL1_QUAD_MODE;
    }

    ctrl1_val |= (tx_len << QSPI_CTRL1_TX_COUNT_SHIFT) 
               | (rx_len << QSPI_CTRL1_RX_COUNT_SHIFT);
    ctrl1_val |= QSPI_CTRL1_START_OP;

    // Always assert CE at the start of a transfer
    ctrl1_val |= QSPI_CTRL1_CE_ACTIVATE;

    *g_qspi_ctrl1_reg = ctrl1_val;

    // Write command/data bytes to the FIFO
    for (uint32_t i = 0; i < tx_len; ++i) {
        // As per original soft-core driver, pack each byte into a 32-bit word.
        *g_qspi_data_reg = ((uint32_t)tx_buffer[i]) << 24;
    }

    QSPI_FPGA_IF_wait_controller_idle();

    // Read result bytes from the FIFO
    for (uint32_t i = 0; i < rx_len; ++i) {
        // Assumes the controller places the received byte in the LSB of the read data.
        rx_buffer[i] = (uint8_t)(*g_qspi_data_reg);
    }

    ctrl1_val = *g_qspi_ctrl1_reg;

    // Deactivate CE if it wasn't requested to be kept active
    if (!keep_ce_active) {
        ctrl1_val &= ~QSPI_CTRL1_CE_ACTIVATE;
    }

    // It is good practice to clear START_OP and counts after the transaction.
    ctrl1_val &= ~(QSPI_CTRL1_START_OP 
                  | (0x7FFu << QSPI_CTRL1_RX_COUNT_SHIFT) 
                  | (0x7FFu << QSPI_CTRL1_TX_COUNT_SHIFT));
    *g_qspi_ctrl1_reg = ctrl1_val;
}
