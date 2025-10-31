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
#include "ddr_service.h"
#include "hss_progress.h"

#include "scai_fpga.h"
#include "scai_fpga_platform.h"
#include "scai_fpga_gpio.h"
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
    // Direct driver does not use the channel struct
    if (g_active_flash_type == SCAI_WINBOND_W25N01_DIRECT) {
        return (g_active_driver != NULL);
    }
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
    if (g_active_flash_type == SCAI_WINBOND_W25N01_DIRECT) {
        // Direct driver does not use the channel struct
        if (g_active_driver->init) {
            g_active_driver->init(NULL, io_format);
        }
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

#define MT29F_TEST_BLOCK_SIZE_BYTES (64 * 4096)      // 256 KB

uint8_t scai_flash_test(scai_flash_type_t flash_type) {
    uint32_t MT29F_TEST_TOTAL_BLOCKS     = 4096;
    size_t   MT29F_CHIP_SIZE_BYTES       = ( (uint64_t)MT29F_TEST_TOTAL_BLOCKS * MT29F_TEST_BLOCK_SIZE_BYTES ); // 1 GiB
    uint8_t  real_chip                   = flash_type + SCAI_MICRON_MT29F_CHIP_0;

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "--- Starting 1 GiB Image Write/Verify Test for MT29F type %d ---\n", real_chip);

    if (real_chip > SCAI_MICRON_MT29F_CHIP_7) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "This test is only for MT29F chips (types %d-%d).\n",
            SCAI_MICRON_MT29F_CHIP_0, SCAI_MICRON_MT29F_CHIP_7);
        return SCAI_FLASH_ERROR;
    }

    // --- Step 0: Acquire Buffer ---
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Acquiring 1 GiB buffer pointer from HSS DDR region...\n");
    uint8_t* ddr_base_ptr     = (uint8_t*)HSS_DDRHi_GetStart();
    uint8_t* read_back_buffer = ddr_base_ptr;
    uint8_t* image_buffer     = ddr_base_ptr + MT29F_TEST_BLOCK_SIZE_BYTES; // Leave 256 KB for read-back

    // Ensure that we have enough memory.
    if (MT29F_CHIP_SIZE_BYTES > HSS_DDRHi_GetSize()) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "FATAL: 1 GiB buffer size exceeds available DDR memory! \n0x%llX bytes required, 0x%llX bytes available\n", 
            (unsigned long long) MT29F_CHIP_SIZE_BYTES, (unsigned long long) HSS_DDR_GetSize());
        return SCAI_FLASH_ERROR;
    }
    
    // Temporary buffer for read-back during verification.

    if (scai_set_flash_chip(real_chip, MSS_QSPI_QUAD_FULL) != SCAI_FLASH_SUCCESS) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    // =========================================================================
    // Phase 1: Fill the 1 GiB buffer with test data
    // =========================================================================
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n--- Phase 1: Filling 1 GiB DDR buffer... (this may take a moment)\n");
    for (size_t i = 0; i < MT29F_CHIP_SIZE_BYTES; i++) {
        image_buffer[i] = (uint8_t)i; // Simple sequential pattern
        if ((i & 0xFFFFFF) == 0) {    // Print progress every 16 MB
            HSS_ShowProgress(MT29F_CHIP_SIZE_BYTES, MT29F_CHIP_SIZE_BYTES - i);
        }
    }
    HSS_ShowProgress(MT29F_CHIP_SIZE_BYTES, 0);
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Generated 1 GiB test image in DDR memory.\n");

    // =========================================================================
    // Phase 2: Write the entire image to the chip
    // =========================================================================
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n--- Phase 2: Writing Full 1 GiB Image to Flash...\n");

    for (uint32_t block_idx = 0; block_idx < MT29F_TEST_TOTAL_BLOCKS; block_idx++) {
        if ((block_idx & 0x0F) == 0) { // Print progress every 16 blocks (4 MiB)
            HSS_ShowProgress(MT29F_TEST_TOTAL_BLOCKS, MT29F_TEST_TOTAL_BLOCKS - block_idx);
        }

        uint32_t block_base_addr   = (uint32_t)block_idx * MT29F_TEST_BLOCK_SIZE_BYTES;
        uint8_t* current_chunk_ptr = image_buffer + block_base_addr;

        if (Flash_erase_block(block_idx) != SCAI_FLASH_SUCCESS) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "\n>> FAILED to erase block %u.\n", block_idx);
            return SCAI_FLASH_ERROR;
        }

        if (Flash_program(current_chunk_ptr, block_base_addr, MT29F_TEST_BLOCK_SIZE_BYTES) != SCAI_FLASH_SUCCESS) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "\n>> FAILED to program block %u.\n", block_idx);
            return SCAI_FLASH_ERROR;
        }
    }
    HSS_ShowProgress(MT29F_TEST_TOTAL_BLOCKS, 0);
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n\nCompleted writing 1 GiB test image to flash.\n");

    // =========================================================================
    // Phase 3: Verify the entire image by reading back and comparing
    // =========================================================================
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n\n--- Phase 3: Verifying Full 1 GiB Image...\n");

    for (uint32_t block_idx = 0; block_idx < MT29F_TEST_TOTAL_BLOCKS; block_idx++) {
        if ((block_idx & 0x0F) == 0) { // Print progress every 16 blocks (4 MiB)
            HSS_ShowProgress(MT29F_TEST_TOTAL_BLOCKS, MT29F_TEST_TOTAL_BLOCKS - block_idx);
        }

        uint32_t block_base_addr    = (uint32_t)block_idx * MT29F_TEST_BLOCK_SIZE_BYTES;
        uint8_t* original_chunk_ptr = image_buffer + block_base_addr;
        
        if (Flash_read(read_back_buffer, block_base_addr, MT29F_TEST_BLOCK_SIZE_BYTES) != SCAI_FLASH_SUCCESS) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "\n>> FAILED to read back block %u.\n", block_idx);
            return SCAI_FLASH_ERROR;
        }

        if (memcmp(original_chunk_ptr, read_back_buffer, MT29F_TEST_BLOCK_SIZE_BYTES) != 0) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "\n>> FAILED: Data mismatch found in block %u!\n", block_idx);
            return SCAI_FLASH_ERROR;
        }
    }
    HSS_ShowProgress(MT29F_TEST_TOTAL_BLOCKS, 0);

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n\n--- 1 GiB Full Chip Write/Verify Test PASSED ---\n");
    return SCAI_FLASH_SUCCESS;
}

uint32_t scai_flash_jedec_id(scai_flash_type_t flash_type) {
    uint8_t id_buf[4];
    uint8_t real_chip = flash_type + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", flash_type);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_QUAD_FULL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Reading JEDEC ID...\n");
    Flash_readid(id_buf);
    return (id_buf[0] << 24) | (id_buf[1] << 16) | (id_buf[2] << 8) | id_buf[3];
}

uint8_t scai_fpga_diagnostics(void)
{    
    if (scai_set_flash_chip(SCAI_MICRON_MT29F, MSS_QSPI_QUAD_FULL) != 0) {
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

    if (scai_set_flash_chip(real_chip, MSS_QSPI_QUAD_FULL) != 0) {
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

    if (scai_set_flash_chip(real_chip, MSS_QSPI_QUAD_FULL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    if (Flash_read(read_data, page, 32) == 0) {
        for (uint32_t i = 0; i < 16; i++) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "0x%02X\n", read_data[i]);
        }

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

    if (scai_set_flash_chip(real_chip, MSS_QSPI_QUAD_FULL) != 0) {
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

uint8_t scai_fpga_reset(uint8_t chip) {
    uint8_t real_chip = chip + SCAI_MICRON_MT29F_CHIP_0;

    if (real_chip >= SCAI_MEM_TYPES_QUANTITY) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid SCAI flash type: %u\n", chip);
        return 0;
    }

    if (scai_set_flash_chip(real_chip, MSS_QSPI_QUAD_FULL) != 0) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    SCAI_MT29_Flash_reset(g_active_channel);
    return SCAI_FLASH_SUCCESS;
}

/**
 * @brief Writes and verifies a binary image to a specified MT29F flash chip from a DDR memory location.
 *
 * @param image_ptr   Pointer to the start of the image in DDR memory.
 * @param image_size  Size of the image in bytes.
 * @param flash_type  Target MT29F chip for writing (must be within MT29F range).
 * @return SCAI_FLASH_SUCCESS on success, otherwise SCAI_FLASH_ERROR.
 */
uint8_t scai_flash_write_image_from_ddr(scai_flash_type_t flash_type, const uint8_t* image_ptr, size_t image_size) {
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "--- Starting Image Write/Verify for MT29F type %d ---\n", flash_type);
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Image Source: @ %p, Size: %zu bytes\n", (void*)image_ptr, image_size);
    uint32_t MT29F_TEST_TOTAL_BLOCKS     = 4096;
    uint32_t MT29F_BLOCK_SIZE_BYTES      = 64 * 4096; // 256 KB
    size_t   MT29F_CHIP_SIZE_BYTES       = ( (uint64_t)MT29F_TEST_TOTAL_BLOCKS * MT29F_TEST_BLOCK_SIZE_BYTES ); // 1 GiB

    // Validate image parameters
    if (!image_ptr || image_size == 0 || image_size > MT29F_CHIP_SIZE_BYTES) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "FATAL: Invalid image pointer or size.\n");
        return SCAI_FLASH_ERROR;
    }

    if (flash_type < SCAI_MICRON_MT29F_CHIP_0 || flash_type > SCAI_MICRON_MT29F_CHIP_7) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "This function is only for MT29F chips.\n");
        return SCAI_FLASH_ERROR;
    }

    if (scai_set_flash_chip(flash_type, MSS_QSPI_QUAD_FULL) != SCAI_FLASH_SUCCESS) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return SCAI_FLASH_ERROR;
    }

    uint32_t num_blocks_to_process = (image_size + MT29F_BLOCK_SIZE_BYTES - 1) / MT29F_BLOCK_SIZE_BYTES;

    // =========================================================================
    // Erase necessary blocks 
    // =========================================================================
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Erasing required blocks\n");
    for (uint16_t block_idx = 0; block_idx < num_blocks_to_process; block_idx++) {
        HSS_ShowProgress(num_blocks_to_process, num_blocks_to_process - (block_idx + 1));
        if (Flash_erase_block(block_idx) != SCAI_FLASH_SUCCESS) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "\n>> FAILED to erase block %u.\n", block_idx);
            return SCAI_FLASH_ERROR;
        }
    }
    HSS_ShowProgress(num_blocks_to_process, 0);

    // =========================================================================
    // Writing Full Image
    // =========================================================================
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Writing Full Image\n");
    if (Flash_program(image_ptr, 0, image_size) != SCAI_FLASH_SUCCESS) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "\tFAILED to program image.\n");
        return SCAI_FLASH_ERROR;
    }
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Image programming complete.\n");

    // =========================================================================
    // Validation by Read-Back
    // =========================================================================
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Verifying Image on Flash\n");
    uint8_t read_back_buffer[MT29F_BLOCK_SIZE_BYTES];

    for (uint16_t block_idx = 0; block_idx < num_blocks_to_process; block_idx++) {
        HSS_ShowProgress(num_blocks_to_process, num_blocks_to_process - (block_idx + 1));

        uint32_t block_base_addr          = (uint32_t)block_idx * MT29F_BLOCK_SIZE_BYTES;
        const uint8_t* original_chunk_ptr = image_ptr + block_base_addr;
        size_t chunk_size                 = (image_size - block_base_addr < MT29F_BLOCK_SIZE_BYTES) ? (image_size % MT29F_BLOCK_SIZE_BYTES) : MT29F_BLOCK_SIZE_BYTES;
        
        if (Flash_read(read_back_buffer, block_base_addr, chunk_size) != SCAI_FLASH_SUCCESS) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "\n>> FAILED to read back block %u.\n", block_idx);
            return SCAI_FLASH_ERROR;
        }

        if (memcmp(original_chunk_ptr, read_back_buffer, chunk_size) != 0) {
            mHSS_DEBUG_PRINTF(LOG_ERROR, "\n>> FAILED: Data mismatch found in block %u!\n", block_idx);
            return SCAI_FLASH_ERROR;
        }
    }
    HSS_ShowProgress(num_blocks_to_process, 0);

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "\n\n--- Image Write and Verify PASSED ---\n");
    return SCAI_FLASH_SUCCESS;
}

bool scai_select_boot_flash(scai_flash_type_t selectedChip) {
    if (selectedChip >= SCAI_MEM_TYPES_QUANTITY) {
        return false;   
    }
    if (scai_set_flash_chip(selectedChip, MSS_QSPI_QUAD_FULL) != SCAI_FLASH_SUCCESS) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Failed to set flash chip.\n");
        return false;
    }
    return true;
}

