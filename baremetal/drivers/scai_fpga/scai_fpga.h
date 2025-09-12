/*
 * scai_fpga.h
 *
 * Top-level wrapper for selecting and using SCAI flash memory drivers.
 * This acts as the main entry point for HSS.
 *
 */
#ifndef MSS_SCAI_FPGA_H_
#define MSS_SCAI_FPGA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "drivers/mss/mss_qspi/mss_qspi.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declaration for the Bad Block Look-Up Table entry structure.
// winbond_w25n01gv.h and winbond_w25n01gv_direct.h define this structure.
struct w25_bb_lut_entry;
typedef struct w25_bb_lut_entry w25_bb_lut_entry_t;

// Struct with the function pointers for a generic flash driver interface
typedef struct {
    void (*init)(mss_qspi_io_format io_format);
    void (*read_id)(uint8_t* id_buf);
    uint8_t (*read)(uint8_t* rx_buf, uint32_t start_addr, uint32_t size);
    uint8_t (*erase)(void);
    uint8_t (*erase_block)(uint16_t block_number);
    uint8_t (*program)(const uint8_t* tx_buf, uint32_t start_addr, uint32_t size);
    void (*read_status_regs)(uint8_t* regs_buf);
    uint32_t (*scan_for_bad_blocks)(uint16_t* bad_blocks_buf);
    uint8_t (*read_bb_lut)(w25_bb_lut_entry_t* lut_ptr);
    uint8_t (*add_entry_to_bb_lut)(uint16_t lba, uint16_t pba);
} scai_flash_driver_t;

// Enum to identify the different supported flash memory types
typedef enum {
    SCAI_MICRON_MT29F,
    SCAI_WINBOND_W25N01_FPGA,
    SCAI_MICRON_MT25Q,
    SCAI_WINBOND_W25N01_DIRECT,
    SCAI_MEM_TYPES_QUANTITY
} scai_flash_type_t;

// Return states
typedef enum {
    SCAI_FLASH_SUCCESS = 0,
    SCAI_FLASH_ERROR   = 1
} scai_flash_status_t;

// Make global state visible to the inline function
extern const scai_flash_driver_t* g_active_driver;
extern scai_flash_type_t g_active_flash_type;
extern bool g_is_initialized[];

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

// Wrapper functions for the currently active flash driver
void Flash_init(mss_qspi_io_format io_format);
void Flash_readid(uint8_t* id_buf);
uint8_t Flash_read(uint8_t* rx_buf, uint32_t start_addr, uint32_t size);
uint8_t Flash_erase(void);
uint8_t Flash_erase_block(uint16_t block_number);
uint8_t Flash_program(const uint8_t* tx_buf, uint32_t start_addr, uint32_t size);
void Flash_read_status_regs(uint8_t* regs_buf);
uint32_t Flash_scan_for_bad_blocks(uint16_t* bad_blocks_buf);
uint8_t Flash_read_bb_lut(w25_bb_lut_entry_t* lut_ptr);
uint8_t Flash_add_entry_to_bb_lut(uint16_t lba, uint16_t pba);

// SCAI specific management functions
uint8_t scai_set_flash_chip(scai_flash_type_t flash_type, mss_qspi_io_format io_format);
const scai_flash_driver_t* get_scai_flash_driver(void);
scai_flash_type_t get_scai_flash_type(void);
uint8_t scai_flash_test(scai_flash_type_t flash_type);

#ifdef __cplusplus
}
#endif
#endif /* MSS_SCAI_FPGA_H_ */
