/*
 * scai_fpga.c
 *
 * Top-level wrapper for selecting and using SCAI flash memory drivers.
 * This acts as the main entry point for HSS.
 *
 */
#include "mpfs_hal/mss_hal.h"
#include "hss_debug.h"
#include "hss_types.h"

#include "scai_fpga.h"
#include "scai_fpga_platform.h"
#include "scai_fpga_gpio.h"
#include "micron_mt29f_fpga.h"
#include "winbond_w25n01gv_fpga.h"
#include "winbond_w25n01gv_direct.h"

#include "ctest/sapi_MT29F.h"
#include "ctest/sapi_qspi_memories.h"
#include "ctest/sapi_qspi.h"
#include "ctest/sapi_adcs.h"
#include "ctest/sapi_gpios.h"

#include <string.h>

// Driver Definitions
// Declarations of the driver structs for each supported flash type.
// Each struct contains function pointers to the specific implementations
static const scai_flash_driver_t winbond_w25n01_fpga_driver = {
    .init                = Scai_W25_Fpga_Flash_init,
    .read_id             = Scai_W25_Fpga_Flash_readid,
    .read                = Scai_W25_Fpga_Flash_read,
    .erase               = Scai_W25_Fpga_Flash_erase,
    .erase_block         = Scai_W25_Fpga_Flash_erase_block,
    .program             = Scai_W25_Fpga_Flash_program,
    .read_status_regs    = Scai_W25_Fpga_Flash_read_status_regs,
    .scan_for_bad_blocks = NULL, // Scai_W25_Fpga_Flash_scan_for_bad_blocks,
    .read_bb_lut         = NULL, // Scai_W25_Fpga_Flash_read_bb_lut,
    .add_entry_to_bb_lut = NULL  // Scai_W25_Fpga_Flash_add_entry_to_bb_lut
};

static const scai_flash_driver_t w25n01_direct_driver = {
    .init                = Scai_W25_Flash_init,
    .read_id             = Scai_W25_Flash_readid,
    .read                = Scai_W25_Flash_read,
    .erase               = Scai_W25_Flash_erase,
    .erase_block         = Scai_W25_Flash_erase_block,
    .program             = Scai_W25_Flash_program,
    .read_status_regs    = Scai_W25_Flash_read_status_regs,
    .scan_for_bad_blocks = Scai_W25_Flash_scan_for_bad_blocks,
    .read_bb_lut         = Scai_W25_Flash_read_bb_lut,
    .add_entry_to_bb_lut = Scai_W25_Flash_add_entry_to_bb_lut
};

static const scai_flash_driver_t micron_mt29f_driver = {
    .init                = SCAI_MT29_Flash_init,
    .read_id             = SCAI_MT29_Flash_readid,
    .read                = SCAI_MT29_Flash_read,
    .erase               = SCAI_MT29_Flash_erase,
    .erase_block         = SCAI_MT29_Flash_erase_block,
    .program             = SCAI_MT29_Flash_program,
    .read_status_regs    = SCAI_MT29_Flash_read_status_regs,
    .scan_for_bad_blocks = NULL, // Not applicable for this flash type
    .read_bb_lut         = NULL, // Not applicable for this flash type
    .add_entry_to_bb_lut = NULL  // Not applicable for this flash type
};

// Module State
// Pointers to the currently active driver and its type.
// Default to the direct MSS driver as per the requirement.

static scai_fpga_channel_t       g_qspi_channels[SCAI_MEM_TYPES_QUANTITY] = { 0 };
static scai_fpga_channel_t       *g_active_channel                        = &g_qspi_channels[SCAI_WINBOND_W25N01_DIRECT];
static const scai_flash_driver_t *g_active_driver                         = &w25n01_direct_driver;
static scai_flash_type_t         g_active_flash_type                      = SCAI_WINBOND_W25N01_DIRECT;
static bool                      g_is_mt29f_enabled                       = false;

// Inline helper function for driver validation
static inline bool is_driver_ready(void) {
    if (!g_active_driver || !g_active_channel) {
        // mHSS_DEBUG_PRINTF(LOG_ERROR, "No active SCAI flash driver selected.\n");
        return false;
    }

    if (!g_active_channel->is_initialized) {
        // mHSS_DEBUG_PRINTF(LOG_ERROR, "Active SCAI flash driver is not initialized.\n");
        return false;
    }
    return true;
}

// static inline bool find_Mt29_chip_index(uintptr_t base_addr, uint8_t* chip_index_out) {
//     for (uint8_t i = 0; i < sizeof(MT29F_BASE_ADDRS)/sizeof(MT29F_BASE_ADDRS[0]); i++) {
//         if (MT29F_BASE_ADDRS[i] == base_addr) {
//             *chip_index_out = i;
//             return true;
//         }
//     }
//     return false; // Address not found
// }

// =============================================================================
// SCAI Management Functions
// =============================================================================

uint8_t scai_set_flash_chip(scai_flash_type_t flash_type, mss_qspi_io_format io_format) {
    if (flash_type >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Unknown SCAI FPGA flash type: %u\n", flash_type);
        return SCAI_FLASH_ERROR;
    }

    // Only switch if the type is different
    if (g_active_flash_type != flash_type) {
        switch (flash_type) {
            case SCAI_WINBOND_W25N01_FPGA:
                g_active_driver = &winbond_w25n01_fpga_driver; 
                g_qspi_channels[SCAI_WINBOND_W25N01_FPGA].base_addr = W25N01_FPGA_BASE_ADDR;
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "Switched to Winbond W25N01 FPGA flash driver.\n");
                break;
            case SCAI_WINBOND_W25N01_DIRECT:
                g_active_driver = &w25n01_direct_driver;
                g_qspi_channels[SCAI_WINBOND_W25N01_DIRECT].base_addr = W25N01_DIRECT_BASE_ADDR;
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "Switched to Winbond W25N01 Direct flash driver.\n");
                break;
            case SCAI_MICRON_MT29F:
                if (!g_is_mt29f_enabled) {
                    scai_fpga_gpio_enable_mt29f();
                    g_is_mt29f_enabled = true;
                }
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "MT29F chip not specified, defaulting to Chip 0.\n");
                g_active_driver = &micron_mt29f_driver;
                g_qspi_channels[SCAI_MICRON_MT29F].base_addr = MT29F_BASE_ADDRS[0];
                break;
            case SCAI_MICRON_MT29F_CHIP_0:
            case SCAI_MICRON_MT29F_CHIP_1:
            case SCAI_MICRON_MT29F_CHIP_2:
            case SCAI_MICRON_MT29F_CHIP_3:
            case SCAI_MICRON_MT29F_CHIP_4:
            case SCAI_MICRON_MT29F_CHIP_5:
            case SCAI_MICRON_MT29F_CHIP_6:
            case SCAI_MICRON_MT29F_CHIP_7:
                if (!g_is_mt29f_enabled) {
                    scai_fpga_gpio_enable_mt29f();
                    g_is_mt29f_enabled = true;
                }
                g_active_driver = &micron_mt29f_driver;
                g_qspi_channels[flash_type].base_addr = MT29F_BASE_ADDRS[flash_type - SCAI_MICRON_MT29F_CHIP_0];
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "Switched to Micron MT29F FPGA flash driver, Chip %u.\n", flash_type - SCAI_MICRON_MT29F_CHIP_0);
            break;
            case SCAI_MICRON_MT25Q:
                mHSS_DEBUG_PRINTF(LOG_ERROR, "Micron MT25Q driver not implemented yet.\n");
                break;
            default: // Should not happen due to check above
                g_active_driver = NULL;
            return SCAI_FLASH_ERROR;
        }
        g_active_flash_type = flash_type;
        g_active_channel    = &g_qspi_channels[flash_type];

        // Initialize the driver if it hasn't been initialized yet.
        if (g_active_driver 
            && g_active_driver->init 
            && !g_active_channel->is_initialized) 
        {
            g_active_driver->init(g_active_channel, io_format);
            g_active_channel->is_initialized = true;
        }
    }

    return SCAI_FLASH_SUCCESS;
}

const scai_flash_driver_t* get_scai_flash_driver(void) {
    return g_active_driver;
}

scai_flash_type_t get_scai_flash_type(void) {
    return g_active_flash_type;
}

// =============================================================================
// Wrapper Functions (HSS API)
// =============================================================================

// This function, called by HSS, initializes the default driver.
// Subsequent drivers are initialized on-demand by scai_set_flash_chip.
void Flash_init(mss_qspi_io_format io_format) {
    if (!g_active_driver || !g_active_channel) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_init: No active SCAI flash driver selected.\n");
        return;
    }

    if (io_format < MSS_QSPI_NORMAL || io_format > MSS_QSPI_QUAD_FULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid MSS QSPI I/O format: %u\n", io_format);
        return;
    }
            
    if (g_active_flash_type == SCAI_MICRON_MT29F || 
        (g_active_flash_type >= SCAI_MICRON_MT29F_CHIP_0 && 
            g_active_flash_type <= SCAI_MICRON_MT29F_CHIP_7)) 
    {
        if (!g_is_mt29f_enabled) {
            scai_fpga_gpio_enable_mt29f();
            g_is_mt29f_enabled = true;
        }
    }

    if (g_active_driver->init && !g_active_channel->is_initialized) {
        g_active_driver->init(g_active_channel, io_format);
    } 
}

void Flash_readid(uint8_t* buf) {
    if (!is_driver_ready()) {
        return;
    }

    if (!buf) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_readid: NULL buffer provided.\n");
        return;
    }

    if (g_active_driver->read_id == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_readid: read_id function not implemented for this driver.\n");
        return;
    }

    g_active_driver->read_id(g_active_channel, buf);
}

uint8_t Flash_read(uint8_t* buf, uint32_t addr, uint32_t len) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (!buf) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_read: NULL buffer provided.\n");
        return SCAI_FLASH_ERROR;
    }

    if (len == 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_read: Zero length read requested.\n");
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->read == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_read: read function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->read(g_active_channel, buf, addr, len);
}

uint8_t Flash_erase(void) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->erase == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_erase: erase function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->erase(g_active_channel);
}

uint8_t Flash_erase_block(uint16_t block_nb) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->erase_block == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_erase_block: erase_block function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->erase_block(g_active_channel, block_nb);
}

uint8_t Flash_program(const uint8_t* buf, uint32_t addr, uint32_t len) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (!buf) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_program: NULL buffer provided.\n");
        return SCAI_FLASH_ERROR;
    }

    if (len == 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_program: Zero length program requested.\n");
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->program == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_program: program function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->program(g_active_channel, buf, addr, len);
}

void Flash_read_status_regs(uint8_t * buf) {
    if (!is_driver_ready()) {
        return;
    }

    if (!buf) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_read_status_regs: NULL buffer provided.\n");
        return;
    }

    if (g_active_driver->read_status_regs == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_read_status_regs: read_status_regs function not implemented for this driver.\n");
        return;
    }

    g_active_driver->read_status_regs(g_active_channel, buf);
}

uint32_t Flash_scan_for_bad_blocks(uint16_t* buf) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (!buf) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_scan_for_bad_blocks: NULL buffer provided.\n");
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->scan_for_bad_blocks == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_scan_for_bad_blocks: scan_for_bad_blocks function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->scan_for_bad_blocks(g_active_channel, buf);
}

uint8_t Flash_read_bb_lut(w25_bb_lut_entry_t* lut_ptr) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (!lut_ptr) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_read_bb_lut: NULL LUT pointer provided.\n");
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->read_bb_lut == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_read_bb_lut: read_bb_lut function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->read_bb_lut(g_active_channel, lut_ptr);
}

uint8_t Flash_add_entry_to_bb_lut(uint16_t lba, uint16_t pba) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->add_entry_to_bb_lut == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_add_entry_to_bb_lut: add_entry_to_bb_lut function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->add_entry_to_bb_lut(g_active_channel, lba, pba);
}

// Example test function
uint8_t scai_flash_test(scai_flash_type_t flash_type) {
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "--- Starting Flash Test for type %d ---\n", flash_type);
    if (flash_type >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", flash_type);
        return SCAI_FLASH_ERROR;
    }
    
    if (scai_set_flash_chip(flash_type, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    uint8_t test_data[256];
    uint8_t read_data[256];
    for (uint32_t i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (uint8_t)i;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Erasing block 10...\n");
    if (Flash_erase_block(10) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash erase_block failed.\n");
        return 4;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Programming block 10...\n");
    if (Flash_program(test_data, 10 * 64 * 2048, sizeof(test_data)) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash program failed.\n");
        return 6;
    }
    
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Dump of first 16 bytes of test_data: ");
    for (uint32_t j = 0; j < 16; j++) {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "0x%02X ", test_data[j]);
    }
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n");


    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Reading block 10...\n");
    memset(read_data, 0, sizeof(read_data));
    if (Flash_read(read_data, 10 * 64 * 2048, sizeof(read_data)) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash read failed.\n");
        return 8;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Dump of first 16 bytes of read_data: ");
    for (uint32_t j = 0; j < 16; j++) {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "0x%02X ", read_data[j]);
    }
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n");

    if (memcmp(test_data, read_data, sizeof(test_data)) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Data mismatch after read/write cycle.\n");
        return 9;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "--- Flash Test Passed ---\n");
    return SCAI_FLASH_SUCCESS;
}

uint32_t scai_flash_jedec_id(scai_flash_type_t flash_type) {
    uint8_t id_buf[4];
    uint8_t real_chip = flash_type + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", flash_type);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Reading JEDEC ID...\n");
    Flash_readid(id_buf);
    return (id_buf[0] << 24) | (id_buf[1] << 16) | (id_buf[2] << 8) | id_buf[3];
}

uint8_t scai_fpga_diagnostics(void)
{    
    if (scai_set_flash_chip(SCAI_MICRON_MT29F, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }
    return SCAI_FLASH_SUCCESS;

}

void scai_fpga_write_reg(uintptr_t address, uint32_t value)
{
    *(volatile uint32_t*)address = value;
    // mHSS_DEBUG_PRINTF(LOG_NORMAL, "Wrote 0x%08X to address 0x%08X\n", value, address);
}

uint32_t scai_fpga_read_reg(uintptr_t address)
{
    uint32_t value = *(volatile uint32_t*)address;
    // mHSS_DEBUG_PRINTF(LOG_NORMAL, "Read 0x%08X from address 0x%08X\n", value, address);
    return value;
}

uint8_t scai_fpga_page_erase(uint8_t chip, uint16_t page) {
    uint8_t real_chip = chip + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", chip);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    return Flash_erase_block(page);
}

uint8_t scai_fpga_page_read(uint8_t chip, uint16_t page) {
    uint8_t read_data[256];
    uint8_t real_chip = chip + SCAI_MICRON_MT29F_CHIP_0;

    memset(read_data, 0, sizeof(read_data));

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", chip);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    if (Flash_read(read_data, page, 32) == 0) {
        return SCAI_FLASH_SUCCESS;
    } else {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash read returned error\n");
        return SCAI_FLASH_ERROR;
    }
}

uint8_t scai_fpga_page_write(uint8_t chip, uint16_t page) {
    uint8_t test_data[128];
    uint8_t real_chip = chip + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", chip);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }
    
    for (uint32_t i = 0; i < sizeof(test_data); i++) {
        test_data[i] = (uint8_t)i;
    }

    if (Flash_erase_block(page) != 0) {
        return SCAI_FLASH_ERROR;
    }

    if (Flash_program(test_data, page, sizeof(test_data)) != 0) {
        return SCAI_FLASH_ERROR;
    }
    return SCAI_FLASH_SUCCESS;
}

uint8_t scai_fpga_stat(uint8_t chip) {
    uint8_t real_chip = chip + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", chip);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    return SCAI_MT29_Flash_get_status(g_active_channel);
}

uint8_t scai_fpga_reset(uint8_t chip) {
    uint8_t real_chip = chip + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", chip);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    SCAI_MT29_Flash_reset(g_active_channel);
    return SCAI_FLASH_SUCCESS;
}

uint8_t scai_fpga_manual_init(uint8_t chip) {
    uint8_t real_chip = chip + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", chip);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_NORMAL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    return SCAI_FLASH_SUCCESS;
}


uint32_t sergio_jedec_id(scai_flash_type_t flash_type) {
    uint8_t tx_d[8];
    uint8_t id_buf[4];
    uint16_t id = 0;

    tx_d[0] = 0x9F;
    tx_d[1] = 0xFF;

    int result = generic_tx_rx_8bits(flash_type, 0, tx_d, 2, id_buf, 2, QSPI_ACTIVATE_CE);
    id = (id_buf[0] << 8) | (id_buf[1] << 0);
    mHSS_DEBUG_PRINTF(LOG_ERROR, "ID: jedec  = 0x%04X\n", id);
    mHSS_DEBUG_PRINTF(LOG_ERROR, "ID: result = %d\n", result);
    
    return (uint32_t)id;
}

uint8_t sergio_manual_init(uint8_t chip) {
    init_gpios();
    init_adcs();
    sapi_init_qspis();
    init_mqspis();
    sapi_init_mt29fs();
    return SCAI_FLASH_SUCCESS;
}

uint8_t sergio_page_erase(uint8_t chip, uint16_t page) {    
    if (MQSPIs[chip].mem_type != MICRON_MT29F) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Erase: Invalid memory type %d\n", MQSPIs[chip].mem_type);
        return SCAI_FLASH_ERROR;
    }

    int result = block_erase_mt29f(chip, page);
    mHSS_DEBUG_PRINTF(LOG_ERROR, "Erase: result = %d\n", result);
    
    return SCAI_FLASH_SUCCESS;
}

uint8_t sergio_page_read(uint8_t chip, uint16_t page) {
    uint32_t buffer[0x400];
    uint32_t quantity = 16;

    if (MQSPIs[chip].mem_type != MICRON_MT29F) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Erase: Invalid memory type %d\n", MQSPIs[chip].mem_type);
        return SCAI_FLASH_ERROR;
    }

    int result = mem_read_mt29f_x4(chip, page, buffer, quantity);
    mHSS_DEBUG_PRINTF(LOG_ERROR, "Read: result = %d\n", result);

    for (uint32_t i = 0; i < quantity; i++) {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "[d] 0x%08X\n", buffer[i]);
    }
    
    return SCAI_FLASH_SUCCESS;
}

uint8_t sergio_fpga_page_write(uint8_t chip, uint16_t page) {
    uint32_t buffer[0x400];
    uint32_t len = 16;

    if (MQSPIs[chip].mem_type != MICRON_MT29F) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Erase: Invalid memory type %d\n", MQSPIs[chip].mem_type);
        return SCAI_FLASH_ERROR;
    }

    uint8_t* p_tmp_buf = (uint8_t *)&buffer[0];

    for (uint32_t i = 0; i < len * 4; i++) {
        p_tmp_buf[i] = i;
    }

    int result = mem_write_mt29f_x4(chip, page, buffer, len);
    mHSS_DEBUG_PRINTF(LOG_ERROR, "Write: result = %d\n", result);
    
    return SCAI_FLASH_SUCCESS;
}
