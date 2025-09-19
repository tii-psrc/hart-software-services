/*
 * winbond_w25n01gv_fpga.c
 *
 * Driver for Winbond W25N01GV series NAND flash memory using a memory-mapped
 * QSPI controller on the PolarFire SoC FPGA fabric (via FIC3).
 * This driver is designed to be standalone for integration into HSS.
 *
 * Copyright 2024, NAVC
 */

#include "winbond_w25n01gv_fpga.h"
#include "scai_fpga_common.h" // Common low-level interface

#include "hss_types.h"

// W25N Flash Command Opcodes
typedef enum {
    W25N_CMD_WRITE_ENABLE               = 0x06,
    W25N_CMD_GET_FEATURES               = 0x0F,
    W25N_CMD_SET_FEATURES               = 0x1F,
    W25N_CMD_PAGE_DATA_READ             = 0x13,
    W25N_CMD_READ_DATA                  = 0x03,
    W25N_CMD_QUAD_READ_DATA             = 0xEB,
    W25N_CMD_PROGRAM_DATA_LOAD          = 0x02,
    W25N_CMD_QUAD_PROG_DATA_LOAD        = 0x32,
    W25N_CMD_PROGRAM_EXECUTE            = 0x10,
    W25N_CMD_BLOCK_ERASE                = 0xD8,
    W25N_CMD_READ_ID                    = 0x9F,
    W25N_CMD_DEVICE_RESET               = 0xFF
} w25n_command_t;

// W25N Feature/Register Addresses
typedef enum {
    W25N_REG_STATUS                     = 0xC0,
    W25N_REG_PROTECTION                 = 0xA0,
    W25N_REG_CONFIG                     = 0xB0
} w25n_register_t;

// Constants from device datasheet
static const uint32_t PAGE_SIZE_BYTES   = 2048;
// static const uint32_t PAGES_PER_BLOCK   = 64;
static const uint16_t TOTAL_BLOCKS      = 1024;
// static const uint32_t BLOCK_SIZE_BYTES  = PAGES_PER_BLOCK * PAGE_SIZE_BYTES;

// Static Helper Functions

/**
 * @brief Converts a logical byte address to the physical format for W25N.
 * @param logical_addr The linear address from the user's perspective.
 * @return The translated physical address used by the flash device.
 */
static inline uint32_t logical_to_physical(uint32_t logical_addr) {
    // This calculation is based on the original soft-core driver's macro.
    return ((logical_addr & 0xFFFFF800UL) << 1) | (logical_addr & 0x000007FFUL);
}

static uint8_t get_feature(scai_fpga_channel_t* channel, w25n_register_t feature_addr) {
    const uint8_t cmd[2] = {W25N_CMD_GET_FEATURES, (uint8_t)feature_addr};
    uint8_t result = 0;    
    scai_fpga_transaction_t params = {
        .tx_buffer = cmd,
        .tx_len    = sizeof(cmd),
        .rx_buffer = &result,
        .rx_len    = sizeof(result)
    };
    scai_fpga_transaction(channel, &params);
    return result;
}

static void set_feature(scai_fpga_channel_t* channel, w25n_register_t feature_addr, uint8_t value) {
    const uint8_t cmd[3] = {W25N_CMD_SET_FEATURES, (uint8_t)feature_addr, value};
    scai_fpga_transaction_t params = {
        .tx_buffer = cmd,
        .tx_len    = sizeof(cmd)
    };
    scai_fpga_transaction(channel, &params);
}

static uint8_t wait_flash_ready(scai_fpga_channel_t* channel) {
    // Operations like erase can take time. This loop polls the status register.
    for (int i = 0; i < 20000; i++) {
        if (!(get_feature(channel, W25N_REG_STATUS) & 0x01)) { // Check BUSY bit
            return 0; // Success
        }
    }
    return 1; // Timeout
}

static void write_enable(scai_fpga_channel_t* channel) {
    const uint8_t cmd = W25N_CMD_WRITE_ENABLE;
    scai_fpga_transaction_t params = {
        .tx_buffer = &cmd,
        .tx_len    = sizeof(cmd)
    };
    scai_fpga_transaction(channel, &params);
}

// =============================================================================
// Public API Implementation
// =============================================================================

void Scai_W25_Fpga_Flash_init(scai_fpga_channel_t* channel, mss_qspi_io_format io_format) {
    QSPI_FPGA_IF_init(channel, io_format);
    // const uint8_t cmd = W25N_CMD_DEVICE_RESET;
    // QSPI_FPGA_IF_transfer(channel, &cmd, 1, NULL, 0, MSS_QSPI_NORMAL, false);
    // wait_flash_ready(channel);
// 
    // // Unlock all blocks and enable buffer mode, replicating soft-core driver logic
    set_feature(channel, W25N_REG_PROTECTION, 0x00);
    // uint8_t config_val = get_feature(channel, W25N_REG_CONFIG);
    // set_feature(channel, W25N_REG_CONFIG, config_val | 0x08); // Set BUF bit for buffer read mode
}

void Scai_W25_Fpga_Flash_readid(scai_fpga_channel_t* channel, uint8_t* id_buf) {
    if (!id_buf) {
        return;
    }

    const uint8_t cmd[2] = {W25N_CMD_READ_ID, 0x00}; // JEDEC ID command requires one dummy byte
    scai_fpga_transaction_t params = {
        .tx_buffer = cmd,
        .tx_len    = sizeof(cmd),
        .rx_buffer = &id_buf,
        .rx_len    = sizeof(id_buf)
    };
    scai_fpga_transaction(channel, &params);
}

uint8_t Scai_W25_Fpga_Flash_read(scai_fpga_channel_t* channel, uint8_t* buf, uint32_t addr, uint32_t len) {
    if (!buf || len == 0) {
        return 1;
    }

    uint8_t cmd[5];
    uint32_t current_addr = addr;
    uint32_t remaining_len = len;
    uint8_t* current_buf = buf;

    while (remaining_len > 0) {
        uint32_t physical_addr = logical_to_physical(current_addr);
        uint32_t page_addr = physical_addr >> 12;
        uint16_t col_addr = physical_addr & 0xFFF;

        cmd[0] = W25N_CMD_PAGE_DATA_READ;
        cmd[1] = (uint8_t)(page_addr >> 8);
        cmd[2] = (uint8_t)(page_addr);

        
        scai_fpga_transaction_t params = {
            .tx_buffer = cmd,
            .tx_len    = sizeof(cmd)
        };
        scai_fpga_transaction(channel, &params);

        if (wait_flash_ready(channel) != 0) {
            return 1;
        }

        uint32_t read_len = (remaining_len > (PAGE_SIZE_BYTES - col_addr)) ? (PAGE_SIZE_BYTES - col_addr) : remaining_len;

        cmd[0] = (QSPI_FPGA_IF_get_io_format() == MSS_QSPI_QUAD_FULL) ? W25N_CMD_QUAD_READ_DATA : W25N_CMD_READ_DATA;
        cmd[1] = (uint8_t)(col_addr >> 8);
        cmd[2] = (uint8_t)(col_addr);
        cmd[3] = 0; // Dummy byte

        scai_fpga_transaction_t params_read = {
            .tx_buffer = cmd,
            .tx_len    = sizeof(cmd),
            .rx_buffer = current_buf,
            .rx_len    = read_len
        };
        scai_fpga_transaction(channel, &params_read);

        current_addr += read_len;
        current_buf += read_len;
        remaining_len -= read_len;
    }
    return 0;
}

uint8_t Scai_W25_Fpga_Flash_erase(scai_fpga_channel_t* channel) {
    for (uint16_t i = 0; i < TOTAL_BLOCKS; ++i) {
        if (Scai_W25_Fpga_Flash_erase_block(channel, i) != 0) {
            return 1;
        }
    }
    return 0;
}

uint8_t Scai_W25_Fpga_Flash_erase_block(scai_fpga_channel_t* channel, uint16_t block_nb) {
    uint8_t cmd[4];

    write_enable(channel);
    
    scai_fpga_transaction_t params = {
        .tx_buffer = cmd,
        .tx_len    = sizeof(cmd)
    };
    scai_fpga_transaction(channel, &params);

    return wait_flash_ready(channel);
}

uint8_t Scai_W25_Fpga_Flash_program(scai_fpga_channel_t* channel, const uint8_t* buf, uint32_t addr, uint32_t len) {
    if (!buf || len == 0) {
        return 1;
    }

    uint8_t cmd[4];
    uint32_t current_addr = addr;
    uint32_t remaining_len = len;
    const uint8_t* current_buf = buf;

    while (remaining_len > 0) {
        uint32_t physical_addr = logical_to_physical(current_addr);
        uint16_t col_addr = physical_addr & 0xFFF;

        write_enable(channel);

        uint32_t write_len = (remaining_len > (PAGE_SIZE_BYTES - col_addr)) ? (PAGE_SIZE_BYTES - col_addr) : remaining_len;

        cmd[0] = (QSPI_FPGA_IF_get_io_format() == MSS_QSPI_QUAD_FULL) ? W25N_CMD_QUAD_PROG_DATA_LOAD : W25N_CMD_PROGRAM_DATA_LOAD;
        cmd[1] = (uint8_t)(col_addr >> 8);
        cmd[2] = (uint8_t)(col_addr);

        scai_fpga_transaction_t params = {
            .tx_buffer = cmd,
            .tx_len    = sizeof(cmd),
            .keep_ce_active = true
        };
        scai_fpga_transaction(channel, &params);
        
        if (wait_flash_ready(channel) != 0) return 1;

        write_enable(channel);
        
        scai_fpga_transaction_t params_read = {
            .tx_buffer = cmd,
            .tx_len    = sizeof(cmd),
            .keep_ce_active = false
        };
        scai_fpga_transaction(channel, &params_read);
        
        if (wait_flash_ready(channel) != 0) {
            return 1;
        }

        current_addr  += write_len;
        current_buf   += write_len;
        remaining_len -= write_len;
    }
    return 0;
}

uint8_t Scai_W25_Fpga_Flash_read_status_regs(scai_fpga_channel_t* channel, void * buf) {
    if (!buf) {
        return 1; // Error
    }

    uint8_t* regs_buf = (uint8_t*)buf;

    regs_buf[0] = get_feature(channel, W25N_REG_PROTECTION);
    regs_buf[1] = get_feature(channel, W25N_REG_CONFIG);
    regs_buf[2] = get_feature(channel, W25N_REG_STATUS);
    return 0; // Success
}

// The following functions for Bad Block Management are placeholders
// as their implementation depends on a specific memory management strategy.

uint8_t Scai_W25_Fpga_Flash_read_bb_lut(scai_fpga_channel_t* channel, w25_bb_lut_entry_t* lut_ptr) {
    // This requires reading a specific area of flash where the LUT is stored.
    // Placeholder - returns error.
    (void)lut_ptr; // Suppress unused parameter warning
    return 1;
}

uint32_t Scai_W25_Fpga_Flash_scan_for_bad_blocks(scai_fpga_channel_t* channel, uint16_t* bad_blocks_buf) {
    // This requires reading the spare area of every page to check the bad block marker.
    // Placeholder - returns 0 bad blocks found.
    (void)bad_blocks_buf; // Suppress unused parameter warning
    return 0;
}
