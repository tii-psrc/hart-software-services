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

#include "scai_fpga_platform.h"
#include "hss_types.h"
#include "drivers/mss/mss_qspi/mss_qspi.h" // For mss_qspi_io_format enum

//------------------------------------------------------------------------------
// Public API
//------------------------------------------------------------------------------

/**
 * @brief A structure to hold the context for a single QSPI controller instance (channel).
 * This includes its base address and a software copy of its control register state.
 */
typedef struct {
    uintptr_t              base_addr;      // Hardware base address of the QSPI controller
    Scai_Qspi_Ctrl_1_Reg_t ctrl1_state;    // Software copy of the CTRL1 register
    Scai_Qspi_Ctrl_2_Reg_t ctrl2_state;    // Software copy of the CTRL2 register
    Scai_Qspi_Ctrl_3_Reg_t ctrl3_state;    // Software copy of the CTRL3 register
    bool                   is_initialized; // True if the controller has been initialized
    mss_qspi_io_format     format;         // I/O format (e.g., MSS_QSPI_QUAD_FULL)
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

inline void scai_fpga_set_word_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.Scai_Qspi_Ctrl_1_Data_Mode = 1;
}

inline void scai_fpga_set_byte_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.Scai_Qspi_Ctrl_1_Data_Mode = 0;
}

inline void scai_fpga_set_qspi_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.Scai_Qspi_Ctrl_1_Lane_Width = 1;
}

inline void scai_fpga_set_spi_mode(scai_fpga_channel_t* channel) {
    channel->ctrl1_state.bits.Scai_Qspi_Ctrl_1_Lane_Width = 0;
}

inline bool scai_fpga_is_word_mode(scai_fpga_channel_t* channel) {
    return channel->ctrl1_state.bits.Scai_Qspi_Ctrl_1_Data_Mode == 1;
}

inline bool scai_fpga_is_quad_mode(scai_fpga_channel_t* channel) {
    return channel->ctrl1_state.bits.Scai_Qspi_Ctrl_1_Lane_Width == 1;
}
void scai_fpga_enable_ce(scai_fpga_channel_t* channel);
void scai_fpga_disable_ce(scai_fpga_channel_t* channel);
void scai_fpga_disable_write_protect(scai_fpga_channel_t* channel);
void scai_fpga_enable_write_protect(scai_fpga_channel_t* channel);

//==============================================================================)

#endif // SCAI_FPGA_COMMON_H
