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

typedef enum {
    QSPI_STATE_START,
    QSPI_STATE_FINALIZE,
    QSPI_STATE_PROGRAM_START,
    QSPI_STATE_PROGRAM_START_FINALIZE,
    QSPI_STATE_DATA_LOAD_START,
    QSPI_STATE_DATA_LOAD_FINALIZE
} QSPI_TransactionState;

static const uint32_t SCAI_FPGA_FIFO_TIMEOUT = 100000;
static const uint16_t SCAI_FPGA_FIFO_LENGTH  = 64;

// Constants for packing a byte into a 32-bit word for the hardware.
// This is required if the hardware expects the byte in the MSB position.
static const uint32_t SCAI_FPGA_FIFO_BYTE_SHIFT = 24;
static const uint32_t SCAI_FPGA_FIFO_TX_BYTE_MASK  = 0xFF000000;
static const uint32_t SCAI_FPGA_FIFO_RX_BYTE_MASK  = 0x000000FF;

static void setReg(volatile uintptr_t reg, uint32_t value) {
    mHSS_DEBUG_PRINTF(LOG_ERROR, "REGW 0x%08x\t=\t0x%08x\n", reg, value);
    *(volatile uint32_t*)reg = value;
}

static uint32_t getReg(volatile uintptr_t reg) {
    uint32_t value = *(volatile uint32_t*)reg;
    mHSS_DEBUG_PRINTF(LOG_ERROR, "REGR 0x%08x\t=\t0x%08x\n", reg, value);
    return value;
}

/**
 * @brief Writes a buffer of data to the QSPI controller's TX FIFO with timeout handling.
 * @note  This is a low-level helper function. It assumes a transaction has already been started.
 *
 * @param base_addr          The base address of the QSPI controller.
 * @param tx_buffer          Pointer to the data to be written (can be uint8_t* or uint32_t*).
 * @param tx_len             The number of elements (bytes or words) to write.
 * @param data_size_is_word  Set to 'true' if tx_buffer is an array of 32-bit words.
 *                           Set to 'false' if tx_buffer is an array of 8-bit bytes.
 *
 * @return The number of elements (bytes or words) successfully written to the FIFO.
 * If the returned value is less than tx_len, a timeout has occurred.
 */
static uint32_t qspi_fpga_fifo_write(uintptr_t base_addr,
                                     const void* tx_buffer,
                                     uint32_t tx_len,
                                     bool data_size_is_word)
{
    uintptr_t data_reg     = base_addr + QSPI_DATA_REG_OFFSET;
    uintptr_t status_2_reg = base_addr + QSPI_STATUS2_REG_OFFSET;
    
    QSPI_Status2_Reg_t status2;
    uint32_t elements_written = 0;

    // Cast the generic void pointer to specific types for convenient access.
    const uint8_t*  buf8  = (const uint8_t*) tx_buffer;
    const uint32_t* buf32 = (const uint32_t*)tx_buffer;

    while (elements_written < tx_len) {
        uint32_t timeout_counter = SCAI_FPGA_FIFO_TIMEOUT;
        do {
            status2.word = getReg(status_2_reg);
            if (!status2.bits.tx_fifo_full) {
                break; // Space is available, exit the wait loop.
            }
            timeout_counter--;
        } while (timeout_counter > 0);

        if (timeout_counter == 0) {
            // Timeout triggered
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Tx FIFO timeout\n");
            return elements_written;
        }

        uint32_t free_space_words = SCAI_FPGA_FIFO_LENGTH - status2.bits.tx_fifo_wrcnt;
        uint32_t chunk_size       = tx_len - elements_written;
        if (chunk_size > free_space_words) {
            chunk_size = free_space_words;
        }

        for (uint32_t i = 0; i < chunk_size; ++i) {
            uint32_t data_to_write = 0;

            if (data_size_is_word) {
                data_to_write = buf32[elements_written];
            } else {
                data_to_write = (((uint32_t)buf8[elements_written]) << SCAI_FPGA_FIFO_BYTE_SHIFT) & SCAI_FPGA_FIFO_TX_BYTE_MASK;
                data_to_write |= ~SCAI_FPGA_FIFO_TX_BYTE_MASK; // Fill lsb bits 
            }
            // *data_reg = data_to_write;
            setReg(data_reg, data_to_write);

            elements_written++;
        }
    }

    return elements_written;
}

/**
 * @brief Reads a buffer of data from the QSPI controller's RX FIFO with timeout handling.
 * @note  This is a low-level helper function. It assumes a transaction has already been started
 *        and the expected number of bytes is known.
 *
 * @param base_addr          The base address of the QSPI controller.
 * @param rx_buffer          Pointer to the buffer where data will be stored.
 * @param rx_len             The number of elements (bytes or words) to read.
 * @param data_size_is_word  Set to 'true' if the function should read 32-bit words.
 *                           Set to 'false' if the function should read 8-bit bytes.
 *
 * @return The number of elements (bytes or words) successfully read from the FIFO.
 * If the returned value is less than rx_len, a timeout has occurred.
 */
static uint32_t qspi_fpga_fifo_read(uintptr_t base_addr,
                                    void* rx_buffer,
                                    uint32_t rx_len,
                                    bool data_size_is_word)
{
    uintptr_t data_reg           = base_addr + QSPI_DATA_REG_OFFSET;
    uintptr_t status_2_reg       = base_addr + QSPI_STATUS2_REG_OFFSET;

    QSPI_Status2_Reg_t status2;
    uint32_t elements_read = 0;

    // Cast the generic void pointer to specific types for convenient access.
    uint8_t*  buf8  = (uint8_t*) rx_buffer;
    uint32_t* buf32 = (uint32_t*)rx_buffer;

    while (elements_read < rx_len) {
        uint32_t timeout_counter = SCAI_FPGA_FIFO_TIMEOUT;
        do {
            status2.word = getReg(status_2_reg);
            if (!status2.bits.rx_fifo_empty) {
                break; // Data is available, exit the wait loop.
            }
            timeout_counter--;
        } while (timeout_counter > 0);

        if (timeout_counter == 0) {
            // Timeout triggered
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Rx FIFO timeout\n");
            return elements_read;
        }

        uint32_t words_available = status2.bits.rx_fifo_rdcnt;
        uint32_t chunk_size = rx_len - elements_read;
        if (chunk_size > words_available) {
            chunk_size = words_available;
        }
        
        for (uint32_t i = 0; i < chunk_size; ++i) {
            uint32_t value = getReg(data_reg);

            if (data_size_is_word) {
                buf32[elements_read] = value;
            } else {
                // As per softcore example, extract the LSB for byte-wise reads.
                buf8[elements_read] = (uint8_t)(value & SCAI_FPGA_FIFO_RX_BYTE_MASK);
            }
            elements_read++;
        }
    }
    
    // Clean up FIFO
    status2.word = getReg(status_2_reg);

    if (!status2.bits.rx_fifo_empty) {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "Rx FIFO cleanup...\n");
        uint32_t words_available = status2.bits.rx_fifo_rdcnt;
        for (uint32_t i = 0; i < words_available; ++i) {
            uint32_t value = getReg(data_reg);
            if (data_size_is_word) {
                uint32_t dummy = value;
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "W DATA_R = 0x%08X\n", dummy);
            } else {
                uint8_t dummy = (uint8_t)(value & SCAI_FPGA_FIFO_RX_BYTE_MASK);
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "B DATA_R = 0x%08X\n", dummy);
            }
        }
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "Rx FIFO cleanup... ended\n");
    }

    return elements_read;
}

static bool qspi_fpga_wait_idle(uintptr_t base_addr) {
    uintptr_t status_1_reg = base_addr + QSPI_STATUS1_REG_OFFSET;
    QSPI_Status1_Reg_t status1;
    uint32_t timeout_counter = SCAI_FPGA_FIFO_TIMEOUT;

    do {
        status1.word = getReg(status_1_reg);
        if (status1.bits.idle) {
            return true; // Operation complete
        }
        timeout_counter--;
    } while (timeout_counter > 0);

    return false; // Timeout
}

/**
 * @brief Manages the state of the CTRL1 register for a QSPI transaction.
 * @param base_addr         The base address of the QSPI controller.
 * @param ctrl_state        A pointer to the software copy of the control register's state.
 * @param transaction_state The desired state transition (START or FINALIZE).
 * @param params            A pointer to a struct with the transaction parameters.
 */
static void qspi_fpga_update_ctrl1(scai_fpga_channel_t* channel,
                                 QSPI_TransactionState transaction_state,
                                 const scai_fpga_transaction_t* params)
{
    uintptr_t ctrl1_reg = channel->base_addr + QSPI_CTRL1_REG_OFFSET;

    switch (transaction_state) {
        case QSPI_STATE_START:
            channel->ctrl1_state.bits.tx_count     = params->tx_len;
            channel->ctrl1_state.bits.rx_count     = params->rx_len;
            channel->ctrl1_state.bits.start        = 1;
            setReg(ctrl1_reg, channel->ctrl1_state.word);

            // Designed by Sergio that CE bit should be changed in other command
            channel->ctrl1_state.bits.chip_enable  = 1;

            // Write again in case CE changed
            setReg(ctrl1_reg, channel->ctrl1_state.word);

            break;
        case QSPI_STATE_FINALIZE:
            // Modify only the necessary bits from the current software state
            channel->ctrl1_state.bits.start        = 0;
            channel->ctrl1_state.bits.tx_count     = 0;
            channel->ctrl1_state.bits.rx_count     = 0;

            setReg(ctrl1_reg, channel->ctrl1_state.word);

            if (!params->keep_ce_active) {
                // Designed by Sergio that CE bit should be changed in other command
                channel->ctrl1_state.bits.chip_enable = 0;
            }

            // Write again in case CE changed
            setReg(ctrl1_reg, channel->ctrl1_state.word);
            break;
        case QSPI_STATE_PROGRAM_START:
            // As is in Sergio code
            channel->ctrl1_state.bits.chip_enable  = 1;
            setReg(ctrl1_reg, channel->ctrl1_state.word);
            
            channel->ctrl1_state.bits.tx_count     = params->tx_len;
            channel->ctrl1_state.bits.rx_count     = params->rx_len;
            channel->ctrl1_state.bits.start        = 1;
            setReg(ctrl1_reg, channel->ctrl1_state.word);

            break;
        case QSPI_STATE_PROGRAM_START_FINALIZE:
            channel->ctrl1_state.bits.tx_count     = 0;
            channel->ctrl1_state.bits.rx_count     = 0;
            channel->ctrl1_state.bits.start        = 0;
            setReg(ctrl1_reg, channel->ctrl1_state.word);

            break;
        case QSPI_STATE_DATA_LOAD_START:
            channel->ctrl1_state.bits.tx_count     = params->tx_len;
            channel->ctrl1_state.bits.rx_count     = params->rx_len;
            channel->ctrl1_state.bits.start        = 1;
            setReg(ctrl1_reg, channel->ctrl1_state.word);
            break;
        case QSPI_STATE_DATA_LOAD_FINALIZE:
            channel->ctrl1_state.bits.tx_count     = 0;
            channel->ctrl1_state.bits.rx_count     = 0;
            // channel->ctrl1_state.bits.start        = 0; // ??? <===================================
            setReg(ctrl1_reg, channel->ctrl1_state.word);

            channel->ctrl1_state.bits.chip_enable  = 0;
            setReg(ctrl1_reg, channel->ctrl1_state.word);
            break;   
        default:
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid transaction state.\n");
            break;
    }
}

// =============================================================================
// Public API Implementation
// =============================================================================

// Initialize global pointers for optimized access based on the defined base address
void scai_fpga_init(scai_fpga_channel_t* channel) {
    uintptr_t ctrl1_reg = channel->base_addr + QSPI_CTRL1_REG_OFFSET;
    uintptr_t ctrl2_reg = channel->base_addr + QSPI_CTRL2_REG_OFFSET;
    uintptr_t ctrl3_reg = channel->base_addr + QSPI_CTRL3_REG_OFFSET;

    // 1. Set default control register values
    channel->ctrl1_state.word             = 0;
    channel->ctrl1_state.bits.reset       = 1;                                          // Clear RESET
    channel->ctrl1_state.bits.data_mode   = 0;                                          // Byte mode
    channel->ctrl1_state.bits.lane_width  = (channel->format == MSS_QSPI_QUAD_FULL) ? 1 : 0;  // Interface width
    setReg(ctrl1_reg, channel->ctrl1_state.word);

    // 2. Configure I/O format
    channel->ctrl2_state.word             = 0;                                          // No auto operations
    setReg(ctrl2_reg, channel->ctrl2_state.word);

    channel->ctrl3_state.word             = 0;
    channel->ctrl3_state.bits.nhold       = 1;                                          // No Toggling, not hold
    setReg(ctrl3_reg, channel->ctrl3_state.word);
}

void scai_fpga_transaction(scai_fpga_channel_t* channel, const scai_fpga_transaction_t* params) {
    if (!params || (params->tx_len > 0 && !params->tx_buffer) || (params->rx_len > 0 && !params->rx_buffer)) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "scai_fpga_transaction: Invalid arguments provided.\n");
        return;
    }

    // 1. Configure and Start Transaction
    qspi_fpga_update_ctrl1(channel, QSPI_STATE_START, params);

    // 2. Handle Data Phase (FIFO Operations)
    if (params->tx_len > 0) {
        uint32_t sent = qspi_fpga_fifo_write(channel->base_addr, params->tx_buffer, params->tx_len, params->data_size_is_word);
        if (sent < params->tx_len) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: failed to send all data. Sent %u of %u.\n", sent, params->tx_len);
        }
    }

    if (params->rx_len > 0) {
        uint32_t received = qspi_fpga_fifo_read(channel->base_addr, params->rx_buffer, params->rx_len, params->data_size_is_word);
        if (received < params->rx_len) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: failed to receive all data. Received %u of %u.\n", received, params->rx_len);
        }
    }

    // 3. Wait while QSPI controller become idle
    if (!qspi_fpga_wait_idle(channel->base_addr)) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: controller did not become idle.\n");
    }

    // 4. Clean Up and Finalize State
    qspi_fpga_update_ctrl1(channel, QSPI_STATE_FINALIZE, params);

}

void scai_fpga_program(scai_fpga_channel_t* channel, const scai_fpga_transaction_t* params) {
    if (!params || (params->tx_len > 0 && !params->tx_buffer) || (params->rx_len > 0 && !params->rx_buffer)) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "scai_fpga_transaction: Invalid arguments provided.\n");
        return;
    }

    // 1. Configure and Start Transaction
    qspi_fpga_update_ctrl1(channel, QSPI_STATE_PROGRAM_START, params);

    // 2. Handle Data Phase (FIFO Operations)
    if (params->tx_len > 0) {
        uint32_t sent = qspi_fpga_fifo_write(channel->base_addr, params->tx_buffer, params->tx_len, params->data_size_is_word);
        if (sent < params->tx_len) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: failed to send all data. Sent %u of %u.\n", sent, params->tx_len);
        }
    }

    if (params->rx_len > 0) {
        uint32_t received = qspi_fpga_fifo_read(channel->base_addr, params->rx_buffer, params->rx_len, params->data_size_is_word);
        if (received < params->rx_len) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: failed to receive all data. Received %u of %u.\n", received, params->rx_len);
        }
    }

    // 3. Wait while QSPI controller become idle
    if (!qspi_fpga_wait_idle(channel->base_addr)) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: controller did not become idle.\n");
    }

    // 4. Clean Up and Finalize State
    qspi_fpga_update_ctrl1(channel, QSPI_STATE_PROGRAM_START_FINALIZE, params);

}


void scai_fpga_load(scai_fpga_channel_t* channel, const scai_fpga_transaction_t* params) {
    if (!params || (params->tx_len > 0 && !params->tx_buffer) || (params->rx_len > 0 && !params->rx_buffer)) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "scai_fpga_transaction: Invalid arguments provided.\n");
        return;
    }

    // 1. Configure and Start Transaction
    qspi_fpga_update_ctrl1(channel, QSPI_STATE_DATA_LOAD_START, params);

    // 2. Handle Data Phase (FIFO Operations)
    if (params->tx_len > 0) {
        uint32_t sent = qspi_fpga_fifo_write(channel->base_addr, params->tx_buffer, params->tx_len, params->data_size_is_word);
        if (sent < params->tx_len) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: failed to send all data. Sent %u of %u.\n", sent, params->tx_len);
        }
    }

    if (params->rx_len > 0) {
        uint32_t received = qspi_fpga_fifo_read(channel->base_addr, params->rx_buffer, params->rx_len, params->data_size_is_word);
        if (received < params->rx_len) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: failed to receive all data. Received %u of %u.\n", received, params->rx_len);
        }
    }

    // 3. Wait while QSPI controller become idle
    if (!qspi_fpga_wait_idle(channel->base_addr)) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Transaction error: controller did not become idle.\n");
    }

    // 4. Clean Up and Finalize State
    qspi_fpga_update_ctrl1(channel, QSPI_STATE_DATA_LOAD_FINALIZE, params);

}

void scai_fpga_disable_write_protect(scai_fpga_channel_t* channel) {
    uintptr_t ctrl1_reg = channel->base_addr + QSPI_CTRL1_REG_OFFSET;
    channel->ctrl1_state.bits.nwp = 1;
    setReg(ctrl1_reg, channel->ctrl1_state.word);
}

void scai_fpga_enable_write_protect(scai_fpga_channel_t* channel) {
    uintptr_t ctrl1_reg = channel->base_addr + QSPI_CTRL1_REG_OFFSET;
    channel->ctrl1_state.bits.nwp = 0;
    setReg(ctrl1_reg, channel->ctrl1_state.word);
}
