/*
 * micron_mt29f_fpga.c
 *
 * Driver for Micron MT29F series NAND flash memory using a memory-mapped
 * QSPI controller on the PolarFire SoC FPGA fabric (via FIC3).
 * This driver is designed to be standalone for integration into HSS.
 *
 * Copyright 2024, NAVC
 *
*/

#include "micron_mt29f_fpga.h"
#include "scai_fpga_common.h" // Common low-level interface

#include "hss_types.h"
#include "hss_debug.h"

static bool g_is_mt29f_initialized = false;
// static uint8_t g_active_die = SCAI_MT29F_CHIP_0; // Default to first die

// --- MT29F Flash Command Opcodes ---
typedef enum {
    MT29F_CMD_WRITE_ENABLE               = 0x06,
    MT29F_CMD_GET_FEATURES               = 0x0F,
    MT29F_CMD_SET_FEATURES               = 0x1F,
    MT29F_CMD_PAGE_READ_TO_CACHE         = 0x13,
    MT29F_CMD_READ_FROM_CACHE_X1         = 0x03,
    MT29F_CMD_READ_FROM_CACHE_X4         = 0x6B,
    MT29F_CMD_PROGRAM_LOAD_X1            = 0x02,
    MT29F_CMD_PROGRAM_LOAD_X4            = 0x32,
    MT29F_CMD_PROGRAM_EXECUTE            = 0x10,
    MT29F_CMD_BLOCK_ERASE                = 0xD8,
    MT29F_CMD_READ_ID                    = 0x9F,
    MT29F_CMD_RESET_DEVICE               = 0xFF
} mt29f_command_t;

// --- MT29F Feature/Register Addresses ---
typedef enum {
    MT29F_REG_STATUS                     = 0xC0,
    MT29F_REG_LOCK                       = 0xA0,
    MT29F_REG_CONFIG                     = 0xB0,
    MT29F_REG_DIE_SELECT                 = 0xD0
} mt29f_register_t;

typedef struct {
    uint8_t opcode;
    uint8_t row_addr_2; // MSB of 3-byte row address
    uint8_t row_addr_1;
    uint8_t row_addr_0; // LSB of 3-byte row address
} mt29f_page_read_cmd_t;

typedef struct {
    uint8_t opcode;
    uint8_t col_addr_1;
    uint8_t col_addr_0; // LSB of 2-byte column address
    uint8_t dummy;      // Dummy byte for timing
} mt29f_read_cmd_t;

// Configuration Register Bitfield
typedef union {
    struct {
        uint8_t conti_rd : 1;    // Bit 0: Continuous Read Mode
        uint8_t          : 1;    // Bit 1: Reserved
        uint8_t lot_en   : 1;    // Bit 2: Lock OTP Area
        uint8_t          : 1;    // Bit 3: Reserved
        uint8_t ecc_en   : 1;    // Bit 4: Internal ECC Enable
        uint8_t cfg      : 2;    // Bit 6:5: User-defined
        uint8_t          : 1;    // Bit 7: Reserved
    } bits;
    uint8_t byte;
} mt29f_config_reg_t;

// --- Constants from device datasheet ---
static const uint32_t PAGE_SIZE_BYTES    = 4096;
static const uint32_t PAGES_PER_BLOCK    = 64;
static const uint16_t TOTAL_BLOCKS       = 4096;                                // 2048 blocks/die * 2 die
static const uint32_t BLOCK_SIZE_BYTES   = PAGES_PER_BLOCK * PAGE_SIZE_BYTES;
static const uint8_t  DUMMY_BYTE         = 0xFF;
static const uint8_t  MT29F_ROWSHIFT     = 13;
static const uint16_t MT29F_COLMASK      = 0x1FFF;
static const uint8_t  MT29F_JEDEC_SIZE   = 2;                                   // JEDEC ID is 2 bytes for MT29F
static const uint16_t MT29F_TIMEOUT_ITER = 10000;                               // Timeout iterations for operations
static const uint8_t  MT29F_UNLOCK_ALL   = 0x00;                               // Value to unlock all blocks

// STatus register bits
static const uint8_t MT29F_STATUS_OIP_B  = 0x01;

// --- Static Helper Functions ---

static inline bool is_initialized(void) {
    if (!g_is_mt29f_initialized) {
        return false;
    }
    return true;
}

/**
 * @brief Converts a logical byte address to the physical format for MT29F.
 * @param logical_addr The linear address from the user's perspective.
 * @return The translated physical address used by the flash device.
 */
static inline uint32_t logical_to_physical(uint32_t logical_addr) {
    // This calculation is based on the original soft-core driver's macro.
    return ((logical_addr & 0xFFFFF000UL) << 1) | (logical_addr & 0x00000FFFUL);
}
static void write_enable(uintptr_t base_addr) {
    const uint8_t cmd = MT29F_CMD_WRITE_ENABLE;
    QSPI_FPGA_IF_transfer(base_addr, &cmd, sizeof(cmd), NULL, 0, MSS_QSPI_NORMAL, false);
}

static uint8_t get_feature(uintptr_t base_addr, mt29f_register_t feature_addr) {
    const uint8_t cmd[] = {MT29F_CMD_GET_FEATURES, (uint8_t)feature_addr};
    uint8_t result = 0;

    QSPI_FPGA_IF_transfer(base_addr, cmd, sizeof(cmd), &result, sizeof(result), MSS_QSPI_NORMAL, false);
    
    return result;
}

static uint8_t wait_flash_ready(uintptr_t base_addr) {
    // Operations like erase can take time. This loop polls the status register.
    for (int i = 0; i < MT29F_TIMEOUT_ITER; i++) {
        if (!(get_feature(base_addr, MT29F_REG_STATUS) & MT29F_STATUS_OIP_B)) { // Check OIP (Operation In Progress) bit
            return 0; // Success
        }
    }
    return 1; // Timeout
}

static void set_feature(uintptr_t base_addr, mt29f_register_t feature_addr, uint8_t value) {
    write_enable(base_addr);
    const uint8_t cmd[] = {MT29F_CMD_SET_FEATURES, (uint8_t)feature_addr, value};
    QSPI_FPGA_IF_transfer(base_addr, cmd, sizeof(cmd), NULL, 0, MSS_QSPI_NORMAL, false);
    wait_flash_ready(base_addr);
}

static void unlock_all_blocks(uintptr_t base_addr) {
    set_feature(base_addr, MT29F_REG_LOCK, MT29F_UNLOCK_ALL);
}

// =============================================================================
// Public API Implementation
// =============================================================================

/**
 * @brief Initializes the MT29F flash FPGA driver.
 * @param io_format The desired QSPI I/O format (e.g., single, dual, quad).
 * 
 * This function must be called before any other flash operations.
 * It initializes the QSPI interface and configures the flash device
 * for optimal performance.
 */
void SCAI_MT29_Flash_init(uintptr_t base_addr, mss_qspi_io_format io_format) {
    if (is_initialized()) {
        // Already initialized, no action needed
        mHSS_DEBUG_PRINTF(LOG_NORMAL, "MT29F already initialized.\n");
        return;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Initializing MT29F...\n");
    
    QSPI_FPGA_IF_init(base_addr, io_format);

    mt29f_config_reg_t config_reg;
    config_reg.byte = get_feature(base_addr, MT29F_REG_CONFIG);
    config_reg.bits.conti_rd = 1;
    set_feature(base_addr, MT29F_REG_CONFIG, config_reg.byte);

    // Unlock all blocks
    unlock_all_blocks(base_addr);

    g_is_mt29f_initialized = true;
    mHSS_DEBUG_PRINTF(LOG_NORMAL, "Micron MT29F configured.\n\n");

}

void SCAI_MT29_Flash_readid(uintptr_t base_addr, uint8_t* id_buf) {
    if (!id_buf || !is_initialized()) {
        return;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "READID()\n");

    const uint8_t cmd[] = {
        MT29F_CMD_READ_ID, 
        DUMMY_BYTE // JEDEC ID command requires one dummy byte
    };
    QSPI_FPGA_IF_transfer(base_addr, (uint8_t*)cmd, sizeof(cmd), id_buf, MT29F_JEDEC_SIZE, MSS_QSPI_NORMAL, false);
}

uint8_t SCAI_MT29_Flash_read(uintptr_t base_addr, uint8_t* buf, uint32_t addr, uint32_t len) {
    if (!buf || len == 0 || !is_initialized()) {
        return 1;
    }

    uint32_t current_addr  = addr;
    uint32_t remaining_len = len;
    uint8_t* current_buf   = buf;

    while (remaining_len > 0) {
        uint32_t physical_addr = logical_to_physical(current_addr);
        uint32_t row_addr = physical_addr >> MT29F_ROWSHIFT;
        uint16_t col_addr = physical_addr & MT29F_COLMASK;

        mt29f_page_read_cmd_t page_read_cmd = {
            .opcode     = MT29F_CMD_PAGE_READ_TO_CACHE,
            .row_addr_2 = (uint8_t)(row_addr >> 16),
            .row_addr_1 = (uint8_t)(row_addr >> 8),
            .row_addr_0 = (uint8_t)(row_addr)
        };
        QSPI_FPGA_IF_transfer(base_addr, (uint8_t*)&page_read_cmd, sizeof(page_read_cmd), NULL, 0, MSS_QSPI_NORMAL, false);

        if (wait_flash_ready(base_addr) != 0) {
            return 1;
        }

        uint32_t read_len = (remaining_len > (PAGE_SIZE_BYTES - col_addr)) ? (PAGE_SIZE_BYTES - col_addr) : remaining_len;

        mt29f_read_cmd_t read_cmd = {
            .opcode     = (QSPI_FPGA_IF_get_io_format() == MSS_QSPI_QUAD_FULL) ? MT29F_CMD_READ_FROM_CACHE_X4 : MT29F_CMD_READ_FROM_CACHE_X1,
            .col_addr_1 = (uint8_t)(col_addr >> 8),
            .col_addr_0 = (uint8_t)(col_addr),
            .dummy      = DUMMY_BYTE
        };
        QSPI_FPGA_IF_transfer(base_addr, (uint8_t*)&read_cmd, sizeof(read_cmd), current_buf, read_len, QSPI_FPGA_IF_get_io_format(), false);

        current_addr  += read_len;
        current_buf   += read_len;
        remaining_len -= read_len;
    }
    return 0;
}

uint8_t SCAI_MT29_Flash_erase(uintptr_t base_addr) {
    if (!is_initialized()) {
        return 1;
    }

    for (uint16_t i = 0; i < TOTAL_BLOCKS; ++i) {
        if (SCAI_MT29_Flash_erase_block(base_addr, i) != 0) {
            return 1;
        }
    }
    return 0;
}

uint8_t SCAI_MT29_Flash_erase_block(uintptr_t base_addr, uint16_t block_nb) {
    if (!is_initialized() || block_nb >= TOTAL_BLOCKS) {
        return 1;
    }

    uint32_t logical_addr  = (uint32_t)block_nb * BLOCK_SIZE_BYTES;
    uint32_t physical_addr = logical_to_physical(logical_addr);
    uint32_t row_addr      = physical_addr >> MT29F_ROWSHIFT;

    write_enable(base_addr);

    mt29f_page_read_cmd_t erase_cmd = {
        .opcode     = MT29F_CMD_BLOCK_ERASE,
        .row_addr_2 = (uint8_t)(row_addr >> 16),
        .row_addr_1 = (uint8_t)(row_addr >> 8),
        .row_addr_0 = (uint8_t)(row_addr)
    };
    QSPI_FPGA_IF_transfer(base_addr, (uint8_t*)&erase_cmd, sizeof(erase_cmd), NULL, 0, MSS_QSPI_NORMAL, false);

    return wait_flash_ready(base_addr);
}

uint8_t SCAI_MT29_Flash_program(uintptr_t base_addr, const uint8_t* buf, uint32_t addr, uint32_t len) {
    if (!buf || (len == 0) || !is_initialized()) {
        return 1;
    }

    if (len > TOTAL_BLOCKS * BLOCK_SIZE_BYTES) {
        return 1; // Length exceeds total flash size
    }

    uint32_t current_addr = addr;
    uint32_t remaining_len = len;
    const uint8_t* current_buf = buf;

    while (remaining_len > 0) {
        uint32_t physical_addr = logical_to_physical(current_addr);
        uint32_t page_addr     = physical_addr >> MT29F_ROWSHIFT;
        uint16_t col_addr      = physical_addr & MT29F_COLMASK;

        write_enable(base_addr);

        uint32_t write_len = (remaining_len > (PAGE_SIZE_BYTES - col_addr)) ? (PAGE_SIZE_BYTES - col_addr) : remaining_len;

        mt29f_read_cmd_t prog_load_cmd = {
            .opcode     = (QSPI_FPGA_IF_get_io_format() == MSS_QSPI_QUAD_FULL) ? MT29F_CMD_PROGRAM_LOAD_X4 : MT29F_CMD_PROGRAM_LOAD_X1,
            .col_addr_1 = (uint8_t)(col_addr >> 8),
            .col_addr_0 = (uint8_t)(col_addr),
            .dummy      = DUMMY_BYTE
        };
        QSPI_FPGA_IF_transfer(base_addr, (uint8_t*)&prog_load_cmd, sizeof(prog_load_cmd), (uint8_t*)current_buf, write_len, QSPI_FPGA_IF_get_io_format(), false);

        if (wait_flash_ready(base_addr) != 0) {
            return 1;
        }

        write_enable(base_addr);

        mt29f_page_read_cmd_t prog_exec_cmd = {
            .opcode     = MT29F_CMD_PROGRAM_EXECUTE,
            .row_addr_2 = (uint8_t)(page_addr >> 16),
            .row_addr_1 = (uint8_t)(page_addr >> 8),
            .row_addr_0 = (uint8_t)(page_addr)
        };
        QSPI_FPGA_IF_transfer(base_addr, (uint8_t*)&prog_exec_cmd, sizeof(prog_exec_cmd), NULL, 0, MSS_QSPI_NORMAL, false);
        
        if (wait_flash_ready(base_addr) != 0) {
            return 1;
        }

        current_addr  += write_len;
        current_buf   += write_len;
        remaining_len -= write_len;
    }
    return 0;
}

uint8_t SCAI_MT29_Flash_read_status_regs(uintptr_t base_addr, void* regs_out) {
    if (!regs_out || !is_initialized()) {
        return 1; // Error
    }

    mt29f_status_regs_t* regs = (mt29f_status_regs_t*)regs_out;

    regs->lock       = get_feature(base_addr, MT29F_REG_LOCK);
    regs->config     = get_feature(base_addr, MT29F_REG_CONFIG);
    regs->status     = get_feature(base_addr, MT29F_REG_STATUS);
    regs->die_select = get_feature(base_addr, MT29F_REG_DIE_SELECT);
    
    return 0; // Success
}
