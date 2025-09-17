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
#include "micron_mt29f_fpga.h"
#include "winbond_w25n01gv_fpga.h"
#include "winbond_w25n01gv_direct.h"

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

static const uintptr_t APB_BASE_ADDRESS        = 0x40000000UL;
static const uintptr_t QSPIs0_BASE_ADDRESS     = APB_BASE_ADDRESS + 0x0300L;
static const uintptr_t QSPIs1_BASE_ADDRESS     = APB_BASE_ADDRESS + 0x0400L;

static const uintptr_t MT29F_BASE_ADDR         = QSPIs1_BASE_ADDRESS;
static const uintptr_t W25N01_FPGA_BASE_ADDR   = QSPIs0_BASE_ADDRESS;
static const uintptr_t W25N01_DIRECT_BASE_ADDR = 0; // It's not used in direct mode

// Module State
// Pointers to the currently active driver and its type.
// Default to the direct MSS driver as per the requirement.
static const scai_flash_driver_t* g_active_driver = &w25n01_direct_driver;
static uintptr_t g_active_base_addr = 0;
static scai_flash_type_t g_active_flash_type = SCAI_WINBOND_W25N01_DIRECT;
static bool g_is_initialized[SCAI_MEM_TYPES_QUANTITY] = { false };

// Inline helper function for driver validation
static inline bool is_driver_ready(void) {
    if (!g_active_driver) {
        // mHSS_DEBUG_PRINTF(LOG_ERROR, "No active SCAI flash driver selected.\n");
        return false;
    }
    if (!g_is_initialized[g_active_flash_type]) {
        // mHSS_DEBUG_PRINTF(LOG_ERROR, "Active SCAI flash driver is not initialized.\n");
        return false;
    }
    return true;
}

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
                g_active_base_addr = W25N01_FPGA_BASE_ADDR;
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "Switched to Winbond W25N01 FPGA flash driver.\n");
                break;
            case SCAI_MICRON_MT29F:
                g_active_driver = &micron_mt29f_driver;
                g_active_base_addr = MT29F_BASE_ADDR;
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "Switched to Micron MT29F FPGA flash driver.\n");
                break;
            case SCAI_WINBOND_W25N01_DIRECT:
                g_active_driver = &w25n01_direct_driver;
                g_active_base_addr = W25N01_DIRECT_BASE_ADDR;
                mHSS_DEBUG_PRINTF(LOG_NORMAL, "Switched to Winbond W25N01 Direct flash driver.\n");
                break;
            case SCAI_MICRON_MT25Q:
                mHSS_DEBUG_PRINTF(LOG_ERROR, "Micron MT25Q driver not implemented yet.\n");
                break;
            default: // Should not happen due to check above
                g_active_driver = NULL;
                return SCAI_FLASH_ERROR;
        }
        g_active_flash_type = flash_type;
    }

    // Initialize the driver if it hasn't been initialized yet.
    if (g_active_driver && g_active_driver->init && !g_is_initialized[flash_type]) {
        g_active_driver->init(g_active_base_addr, io_format);
        g_is_initialized[flash_type] = true;
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
// Wrapper Functions
// =============================================================================

// This function, called by HSS, initializes the default driver.
// Subsequent drivers are initialized on-demand by scai_set_flash_chip.
void Flash_init(mss_qspi_io_format io_format) {
    if (!g_active_driver) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_init: No active SCAI flash driver selected.\n");
        return;
    }

    if (io_format < MSS_QSPI_NORMAL || io_format > MSS_QSPI_QUAD_FULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid MSS QSPI I/O format: %u\n", io_format);
        return;
    }

    if (g_active_driver->init && !g_is_initialized[g_active_flash_type]) {
        g_active_driver->init(g_active_base_addr, io_format);
        g_is_initialized[g_active_flash_type] = true;
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

    g_active_driver->read_id(g_active_base_addr, buf);
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

    return g_active_driver->read(g_active_base_addr, buf, addr, len);
}

uint8_t Flash_erase(void) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->erase == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_erase: erase function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->erase(g_active_base_addr);
}

uint8_t Flash_erase_block(uint16_t block_nb) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->erase_block == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_erase_block: erase_block function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->erase_block(g_active_base_addr, block_nb);
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

    return g_active_driver->program(g_active_base_addr, buf, addr, len);
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

    g_active_driver->read_status_regs(g_active_base_addr, buf);
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

    return g_active_driver->scan_for_bad_blocks(g_active_base_addr, buf);
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

    return g_active_driver->read_bb_lut(g_active_base_addr, lut_ptr);
}

uint8_t Flash_add_entry_to_bb_lut(uint16_t lba, uint16_t pba) {
    if (!is_driver_ready()) {
        return SCAI_FLASH_ERROR;
    }

    if (g_active_driver->add_entry_to_bb_lut == NULL) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash_add_entry_to_bb_lut: add_entry_to_bb_lut function not implemented for this driver.\n");
        return SCAI_FLASH_ERROR;
    }

    return g_active_driver->add_entry_to_bb_lut(g_active_base_addr, lba, pba);
}

// Example test function
uint8_t scai_flash_test(scai_flash_type_t flash_type) {
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "--- Starting Flash Test for type %d ---\n", flash_type);
    if (flash_type >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", flash_type);
        return SCAI_FLASH_ERROR;
    }
    
    if (scai_set_flash_chip(flash_type, MSS_QSPI_QUAD_FULL) != 0) {
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

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Reading block 10...\n");
    memset(read_data, 0, sizeof(read_data));
    if (Flash_read(read_data, 10 * 64 * 2048, sizeof(read_data)) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash read failed.\n");
        return 8;
    }

    if (memcmp(test_data, read_data, sizeof(test_data)) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Data mismatch after read/write cycle.\n");
        return 9;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "--- Flash Test Passed ---\n");
    return SCAI_FLASH_SUCCESS;
}

uint32_t scai_flash_jedec_id(scai_flash_type_t flash_type) {
    uint8_t id_buf[4];
    if (flash_type >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", flash_type);
        return 0;
    }
    if (scai_set_flash_chip(flash_type, MSS_QSPI_QUAD_FULL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return 0;
    }
    Flash_readid(id_buf);
    return (id_buf[0] << 24) | (id_buf[1] << 16) | (id_buf[2] << 8) | id_buf[3];
}

uint8_t scai_fpga_diagnostics(void)
{
    uint8_t read_data[256];
    
    if (scai_set_flash_chip(SCAI_MICRON_MT29F, MSS_QSPI_QUAD_FULL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Reading block 10...\n");
    memset(read_data, 0, sizeof(read_data));
    if (Flash_read(read_data, 10 * 64 * 2048, sizeof(read_data)) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Flash read failed.\n");
        return 8;
    }
    for (size_t i = 0; i < 100; i++) {
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "0x%02X ", read_data[i]);
        if ((i & 0x0F) == 0x0F) {
            mHSS_PUTS("\n");
        }
    }
    return SCAI_FLASH_SUCCESS;

}

void scai_fpga_write_reg(uintptr_t address, uint32_t value)
{
    *(volatile uint32_t*)address = value;
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Wrote 0x%08X to address 0x%08X\n", value, address);
}

uint32_t scai_fpga_read_reg(uintptr_t address)
{
    uint32_t value = *(volatile uint32_t*)address;
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Read 0x%08X from address 0x%08X\n", value, address);
    return value;
}

/*
qspi readreg 0x40000404
qspi writereg 0x40000404 0x4
qspi writereg 0x40000408 0x0
qspi writereg 0x4000040C 0x10000
qspi readreg 0x40000404

// Unlock all blocks
qspi writereg 0x40000400 0x1F000000
qspi writereg 0x40000400 0xA0000000
qspi writereg 0x40000400 0x00000000
qspi writereg 0x40000404 0xE01
qspi readreg 0x40000404

// Continuous Read
qspi writereg 0x40000400 0x1F000000
qspi writereg 0x40000400 0xB0000000
qspi writereg 0x40000400 0x01000000
qspi writereg 0x40000404 0xE01
qspi readreg 0x40000404
*/