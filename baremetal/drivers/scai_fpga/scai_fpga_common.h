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

#include "hss_types.h"
#include "drivers/mss/mss_qspi/mss_qspi.h" // For mss_qspi_io_format enum

//------------------------------------------------------------------------------
// QSPI Controller Register Definitions
//------------------------------------------------------------------------------
/**
 * @brief CTRL1 Register (Write-Only, Offset +4)
 * Configures and initiates a QSPI transaction.
 */

typedef union {
    struct {
        uint32_t chip_enable    : 1;  // Bit 0:      Chip Enable (1=active) when software driven
        uint32_t nwp            : 1;  // Bit 1:      nWP line value
        uint32_t reset          : 1;  // Bit 2:      nReset line value (1=normal operation)
        uint32_t data_mode      : 1;  // Bit 3:      Data interface (1=Byte, 0=Word)
        uint32_t lane_width     : 1;  // Bit 4:      Interface usage (1=x4, 0=x1)
        uint32_t auto_mask      : 1;  // Bit 5:      Mask operation for auto mode
        uint32_t clk_div        : 3;  // Bits 8:6:   Clock divider
        uint32_t start          : 1;  // Bit 9:      Start transaction
        uint32_t tx_count       : 11; // Bits 20:10: Words/Bytes to Transmit
        uint32_t rx_count       : 11; // Bits 31:21: Words/Bytes to Receive
    } bits;
    uint32_t word;
} QSPI_Ctrl1_Reg_t;

/**
 * @brief CTRL2 Register (Write-Only, Offset +8)
 * Configures automated operations.
 */
typedef union {
    struct {
        uint32_t auto_cmd       : 8;  // Bits 7:0:   Command for auto operation
        uint32_t auto_mask      : 8;  // Bits 15:8:  Mask for auto operation
        uint32_t auto_repeats   : 15; // Bits 30:16: Max retries for auto operation
        uint32_t auto_enable    : 1;  // Bit 31:     Enable auto operation
    } bits;
    uint32_t word;
} QSPI_Ctrl2_Reg_t;

/**
 * @brief CTRL3 Register (Write-Only, Offset +12)
 * Provides direct control over QSPI lines for GPIO/toggling.
 */
typedef union {
    struct {
        uint32_t g_out          : 7;  // Bits 6:0: GPIO output values (Rst,CE,CLK,D[3:0])
        uint32_t g_ena          : 7;  // Bits 13:7: GPIO enable (1=output, 0=input)
        uint32_t g_use_gpio     : 1;  // Bit 14: Use GPIO mode (1=GPIO, 0=QSPI)
        uint32_t g_use_toggle   : 1;  // Bit 15: Use toggling mode
        uint32_t nhold          : 1;  // Bit 16: nHold line value
        uint32_t dummy_cnt      : 5;  // Bits 21:17: Number of dummy cycles
        uint32_t                : 10; // Reserved
    } bits;
    uint32_t word;
} QSPI_Ctrl3_Reg_t;

/**
 * @brief STATUS1 Register (Read-Only, Offset +4)
 * Provides the primary status of the QSPI controller.
 */
typedef union {
    struct {
        uint32_t idle           : 1;  // Bit 0: Controller is idle
        uint32_t rx_fifo_empty  : 1;  // Bit 1: RX FIFO is empty
        uint32_t rx_fifo_full   : 1;  // Bit 2: RX FIFO is full
        uint32_t tx_fifo_empty  : 1;  // Bit 3: TX FIFO is empty
        uint32_t tx_fifo_full   : 1;  // Bit 4: TX FIFO is full
        uint32_t err_overrun    : 1;  // Bit 5: Overrun error
        uint32_t auto_op_ready  : 1;  // Bit 6: Auto operation is ready
        uint32_t auto_op_error  : 1;  // Bit 7: Auto operation error
        uint32_t lane_data      : 4;  // Bits 11:8: Current state of QSPI data out lines
        uint32_t lane_clk       : 1;  // Bit 12: Current state of QSPI CLK line
        uint32_t lane_ce        : 1;  // Bit 13: Current state of QSPI CE line
        uint32_t lane_rst       : 1;  // Bit 14: Current state of QSPI RST line
        uint32_t                : 1;  // Bit 15: Reserved
        uint32_t lane_gpio_out  : 7;  // Bits 22:16: GPIO data out lines
        uint32_t                : 1;  // Bit 23: Reserved
        uint32_t lane_ext_data  : 4;  // Bits 27:24: External data lines
        uint32_t lane_ext_clk   : 1;  // Bit 28: External CLK
        uint32_t lane_ext_rst   : 1;  // Bit 29: External RST
        uint32_t lane_ext_ce    : 1;  // Bit 30: External CE
        uint32_t                : 1;  // Bit 31: Reserved
    } bits;
    uint32_t word;
} QSPI_Status1_Reg_t;

/**
 * @brief STATUS2 Register (Read-Only, Offset +8)
 * Provides FIFO status and counters.
 */
typedef union {
    struct {
        uint32_t rx_fifo_full   : 1;  // Bit 0: RX FIFO is full
        uint32_t rx_fifo_empty  : 1;  // Bit 1: RX FIFO is empty
        uint32_t rx_fifo_rdcnt  : 7;  // Bits 8:2: RX FIFO read counter
        uint32_t rx_fifo_wrcnt  : 7;  // Bits 15:9: RX FIFO write counter
        uint32_t tx_fifo_full   : 1;  // Bit 16: TX FIFO is full
        uint32_t tx_fifo_empty  : 1;  // Bit 17: TX FIFO is empty
        uint32_t tx_fifo_rdcnt  : 7;  // Bits 24:18: TX FIFO read counter
        uint32_t tx_fifo_wrcnt  : 7;  // Bits 31:25: TX FIFO write counter
    } bits;
    uint32_t word;
} QSPI_Status2_Reg_t;

typedef enum {
    QSPI_DATA_REG_OFFSET                    = 0x00,
    QSPI_CTRL1_REG_OFFSET                   = 0x04,
    QSPI_STATUS1_REG_OFFSET                  = 0x04,
    QSPI_STATUS2_REG_OFFSET                 = 0x08,
    QSPI_CTRL2_REG_OFFSET                   = 0x08,
    QSPI_CTRL3_REG_OFFSET                   = 0x0C
} qspi_reg_offset_t;

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

/**
 * @brief A structure to hold the context for a single QSPI controller instance (channel).
 * This includes its base address and a software copy of its control register state.
 */
typedef struct {
    uintptr_t          base_addr;      // Hardware base address of the QSPI controller
    QSPI_Ctrl1_Reg_t   ctrl1_state;    // Software copy of the CTRL1 register
    QSPI_Ctrl2_Reg_t   ctrl2_state;    // Software copy of the CTRL2 register
    QSPI_Ctrl3_Reg_t   ctrl3_state;    // Software copy of the CTRL3 register
    bool               is_initialized; // True if the controller has been initialized
    mss_qspi_io_format format;         // I/O format (e.g., MSS_QSPI_QUAD_FULL)
} scai_fpga_channel_t;

/**
 * @brief A structure to define all parameters for a single QSPI transaction.
 * This simplifies the API and makes it more extensible.
 */
typedef struct {
    const void* tx_buffer;        // Pointer to the transmit buffer
    uint32_t    tx_len;           // Number of elements (bytes/words) to transmit
    void*       rx_buffer;        // Pointer to the receive buffer
    uint32_t    rx_len;           // Number of elements (bytes/words) to receive
    mss_qspi_io_format format;    // I/O format (e.g., MSS_QSPI_QUAD_FULL)
    bool        keep_ce_active;   // True to keep Chip Enable asserted after transfer
    bool        data_size_is_word;// True if buffers are 32-bit words, false for 8-bit bytes
} scai_fpga_transaction_t;

/**
 * @brief Initializes the common QSPI interface module.
 */
void scai_fpga_init(scai_fpga_channel_t* channel);

/**
 * @brief The primary function for executing a QSPI transaction.
 * This is a versatile low-level helper for most QSPI operations.
 * @param base_addr The base address of the QSPI controller instance.
 * @param params    A pointer to a struct containing all transaction parameters.
 */
void scai_fpga_transaction(scai_fpga_channel_t* channel, const scai_fpga_transaction_t* params);

void inline scai_fpga_set_word_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.data_mode = 1;
}

void inline scai_fpga_set_byte_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.data_mode = 0;
}

void inline scai_fpga_set_qspi_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.lane_width = 1;
}

void inline scai_fpga_set_spi_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.lane_width = 0;
}

bool inline scai_fpga_is_word_mode(scai_fpga_channel_t* channel) {
    return channel->ctrl1_state.bits.data_mode == 1;
}

bool inline scai_fpga_is_quad_mode(scai_fpga_channel_t* channel) {
    return channel->ctrl1_state.bits.lane_width == 1;
}

//==============================================================================)

#endif // SCAI_FPGA_COMMON_H
