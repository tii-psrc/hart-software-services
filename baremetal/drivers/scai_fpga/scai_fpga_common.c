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
    QSPI_Ctrl1_Reg_t mt29f_ctrl1;
    mt29f_ctrl1.bits.reset       = 1;                                         // Clear RESET
    mt29f_ctrl1.bits.data_mode   = 1;                                         // Byte mode
    mt29f_ctrl1.bits.lane_width  = (io_format == MSS_QSPI_QUAD_FULL) ? 1 : 0; // Interface width
    *ctrl1_reg = mt29f_ctrl1.word;

    // 2. Configure I/O format
    QSPI_Ctrl2_Reg_t mt29f_ctrl2 = {0};                                       // No auto operation
    *ctrl2_reg = mt29f_ctrl2.word;

    QSPI_Ctrl3_Reg_t mt29f_ctrl3 = {0};
    mt29f_ctrl3.bits.nhold = 1;                                               // No Toggling, not hold
    *ctrl3_reg = mt29f_ctrl3.word;
}

mss_qspi_io_format QSPI_FPGA_IF_get_io_format(void) {
    return g_io_format;
}

bool QSPI_FPGA_IF_wait_controller_idle(uintptr_t base_addr) {
    volatile const uint32_t* status_reg = (uint32_t*)(base_addr + QSPI_STATUS_REG_OFFSET);
    QSPI_Status1_Reg_t mt29f_status1;

    for (int i = 0; i < SCAI_FPG_IF_TIMEOUT; i++) {
        mt29f_status1.word = *status_reg;
        if (mt29f_status1.bits.idle) {
            return true;
        }
    }
    return false;
}

void QSPI_FPGA_IF_transfer(uintptr_t base_addr, const uint8_t* tx_buffer, uint32_t tx_len, uint8_t* rx_buffer, uint32_t rx_len, mss_qspi_io_format format, bool keep_ce_active) {
    if ((!tx_buffer && tx_len > 0) || (!rx_buffer && rx_len > 0) || (base_addr == 0)) {
        return;
    }
    
    volatile uint32_t* data_reg = (uint32_t*)(base_addr + QSPI_DATA_REG_OFFSET);
    volatile uint32_t* ctrl1_reg = (uint32_t*)(base_addr + QSPI_CTRL1_REG_OFFSET);

    // Construct CTRL1
    QSPI_Ctrl1_Reg_t mt29f_ctrl1;
    mt29f_ctrl1.bits.reset       = 1;                                      // Clear RESET
    mt29f_ctrl1.bits.data_mode   = 1;                                      // Byte mode
    mt29f_ctrl1.bits.lane_width  = (format == MSS_QSPI_QUAD_FULL) ? 1 : 0; // Interface width
    mt29f_ctrl1.bits.tx_count    = tx_len;                                 // Number of bytes to transmit
    mt29f_ctrl1.bits.rx_count    = rx_len;                                 // Number of bytes to receive
    mt29f_ctrl1.bits.start       = 1;                                      // Start transaction 
    mt29f_ctrl1.bits.chip_enable = 1;                                      // CHIP_ENABLE

    // Write CTRL1
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "CTRL_1 = 0x%08X\n", mt29f_ctrl1.word);
    *ctrl1_reg = mt29f_ctrl1.word;
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "STAT_1 = 0x%08X\n", *ctrl1_reg);


    // Write FIFO
    for (uint32_t i = 0; i < tx_len; ++i) {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "DATA_T = 0x%08X\n", tx_buffer[i]);
        *data_reg = tx_buffer[i];
    }

    bool idle_flag = QSPI_FPGA_IF_wait_controller_idle(base_addr);
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "idle = %d\n", idle_flag);

    // Read FIFO
    for (uint32_t i = 0; i < rx_len; ++i) {
        uint32_t value = *data_reg;
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "DATA_R = 0x%08X\n", value);
        rx_buffer[i] = (uint8_t)(value & 0xFF);
    }

    mt29f_ctrl1.bits.tx_count    = 0;                                 // Number of bytes to transmit
    mt29f_ctrl1.bits.rx_count    = 0;                                 // Number of bytes to receive
    mt29f_ctrl1.bits.start       = 0;                                 // Start transaction 

    if (!keep_ce_active) {
        mt29f_ctrl1.bits.chip_enable = 0;                                   
    }

    // Write CTRL1
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "CTRL_1 = 0x%08X\n", mt29f_ctrl1.word);
    *ctrl1_reg = mt29f_ctrl1.word;
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "STAT_1 = 0x%08X\n", *ctrl1_reg);
}
