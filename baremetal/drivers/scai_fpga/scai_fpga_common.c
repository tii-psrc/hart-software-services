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
#include "hss_debug.h"

//  Common SCAI FPGA interface state
static mss_qspi_io_format g_io_format = MSS_QSPI_NORMAL;

static const uint16_t SCAI_FPG_IF_TIMEOUT = 10000; // Empirical timeout value

// =============================================================================
// Public API Implementation
// =============================================================================

// Initialize global pointers for optimized access based on the defined base address
void QSPI_FPGA_IF_init(uintptr_t base_addr, mss_qspi_io_format io_format) {
    g_io_format = io_format;

    volatile uint32_t* ctrl1_reg = (uint32_t*)(base_addr + QSPI_CTRL1_REG_OFFSET);
    volatile uint32_t* ctrl2_reg = (uint32_t*)(base_addr + QSPI_CTRL2_REG_OFFSET);
    volatile uint32_t* ctrl3_reg = (uint32_t*)(base_addr + QSPI_CTRL3_REG_OFFSET);

    // 1. Set default control register values
    uint32_t ctrl1_val = Q_CTRL1_SET_nRESET;
    // 2. Configure I/O format
    uint32_t ctrl2_val = 0; // No auto operation
    uint32_t ctrl3_val = Q_CTRL3_SET_nHOLD; // No Toggling, not hold

    // Write initial values to the registers
    *ctrl1_reg = ctrl1_val;
    *ctrl2_reg = ctrl2_val;
    *ctrl3_reg = ctrl3_val;
}

mss_qspi_io_format QSPI_FPGA_IF_get_io_format(void) {
    return g_io_format;
}

bool QSPI_FPGA_IF_wait_controller_idle(uintptr_t base_addr) {
    volatile const uint32_t* status_reg = (uint32_t*)(base_addr + QSPI_STATUS_REG_OFFSET);

//    mHSS_DEBUG_PRINTF(LOG_ERROR, "Address of status reg: 0x%X\n", (unsigned int)base_addr + QSPI_STATUS_REG_OFFSET);
    for (int i = 0; i < SCAI_FPG_IF_TIMEOUT; i++) {
        if (*status_reg & QSPI_STATUS_IDLE_FLAG) {
            return true;
        }
    }
//    mHSS_DEBUG_PRINTF(LOG_ERROR, "> Status reg value: 0x%X\n", *status_reg);
    return false;
}

void QSPI_FPGA_IF_transfer(uintptr_t base_addr, const uint8_t* tx_buffer, uint32_t tx_len, uint8_t* rx_buffer, uint32_t rx_len, mss_qspi_io_format format, bool keep_ce_active) {
    if ((!tx_buffer && tx_len > 0) || (!rx_buffer && rx_len > 0) || (base_addr == 0)) {
        return;
    }

    if (format < MSS_QSPI_NORMAL || format > MSS_QSPI_QUAD_FULL) {
        return;
    }
    
    volatile uint32_t* data_reg = (uint32_t*)(base_addr + QSPI_DATA_REG_OFFSET);
    volatile uint32_t* ctrl1_reg = (uint32_t*)(base_addr + QSPI_CTRL1_REG_OFFSET);
mHSS_DEBUG_PRINTF(LOG_NORMAL, "ctrl1_reg -> 0x%zU\n", ctrl1_reg);
mHSS_DEBUG_PRINTF(LOG_NORMAL, "data_reg -> 0x%zU\n", data_reg);

    uint32_t ctrl1_val = *ctrl1_reg;
mHSS_DEBUG_PRINTF(LOG_NORMAL, "r ctrl1_reg = 0x%08X\n", ctrl1_val);

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

mHSS_DEBUG_PRINTF(LOG_NORMAL, "w ctrl1_reg = 0x%08X\n", ctrl1_val);
    //Read 0x%08X from address 0x%08X\n", value, address);
    *ctrl1_reg = ctrl1_val;

    // Write command/data bytes to the FIFO
    for (uint32_t i = 0; i < tx_len; ++i) {
        // As per original soft-core driver, pack each byte into a 32-bit word.

mHSS_DEBUG_PRINTF(LOG_NORMAL, "w data_reg = 0x%08X\n", ((uint32_t)tx_buffer[i]) << 24);
        *data_reg = ((uint32_t)tx_buffer[i]) << 24;
    }

    QSPI_FPGA_IF_wait_controller_idle(base_addr);

    // Read result bytes from the FIFO
    for (uint32_t i = 0; i < rx_len; ++i) {
        // Assumes the controller places the received byte in the LSB of the read data.
        uint32_t value = *data_reg;
mHSS_DEBUG_PRINTF(LOG_NORMAL, "r data_reg = 0x%08X\n", value);
        // rx_buffer[i] = (uint8_t)(*data_reg);
        rx_buffer[i] = (uint8_t)(value >> 24);
    }

    ctrl1_val = *ctrl1_reg;
mHSS_DEBUG_PRINTF(LOG_NORMAL, "r ctrl1_reg = 0x%08X\n", ctrl1_val);

    // Deactivate CE if it wasn't requested to be kept active
    if (!keep_ce_active) {
        ctrl1_val &= ~QSPI_CTRL1_CE_ACTIVATE;
    }

    // It is good practice to clear START_OP and counts after the transaction.
    ctrl1_val &= ~(QSPI_CTRL1_START_OP 
                  | (0x7FFu << QSPI_CTRL1_RX_COUNT_SHIFT) 
                  | (0x7FFu << QSPI_CTRL1_TX_COUNT_SHIFT));
mHSS_DEBUG_PRINTF(LOG_NORMAL, "w ctrl1_reg = 0x%08X\n", ctrl1_val);
    *ctrl1_reg = ctrl1_val;
}
