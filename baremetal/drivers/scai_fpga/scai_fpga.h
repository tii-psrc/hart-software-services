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
#include "hss_types.h"
#include "winbond_w25n01gv_direct.h"

// Struct with the function pointers for a generic flash driver interface
typedef struct {
    void (*init)(uintptr_t base_addr, mss_qspi_io_format io_format);
    void (*read_id)(uintptr_t base_addr, uint8_t* id_buf);
    uint8_t (*read)(uintptr_t base_addr, uint8_t* rx_buf, uint32_t start_addr, uint32_t size);
    uint8_t (*erase)(uintptr_t base_addr);
    uint8_t (*erase_block)(uintptr_t base_addr, uint16_t block_number);
    uint8_t (*program)(uintptr_t base_addr, const uint8_t* tx_buf, uint32_t start_addr, uint32_t size);
    uint8_t (*read_status_regs)(uintptr_t base_addr, void* regs_out);
    uint32_t (*scan_for_bad_blocks)(uintptr_t base_addr, uint16_t* bad_blocks_buf);
    uint8_t (*read_bb_lut)(uintptr_t base_addr, w25_bb_lut_entry_t* lut_ptr);
    uint8_t (*add_entry_to_bb_lut)(uintptr_t base_addr, uint16_t lba, uint16_t pba);
} scai_flash_driver_t;

// Enum to identify the different supported flash memory types
typedef enum {
    SCAI_MICRON_MT29F          = 0,
    SCAI_WINBOND_W25N01_FPGA   = 1,
    SCAI_MICRON_MT25Q          = 2,
    SCAI_WINBOND_W25N01_DIRECT = 3,
    SCAI_MEM_TYPES_QUANTITY
} scai_flash_type_t;

// Return states
typedef enum {
    SCAI_FLASH_SUCCESS = 0,
    SCAI_FLASH_ERROR   = 1
} scai_flash_status_t;

// Wrapper functions for the currently active flash driver
void Flash_init(mss_qspi_io_format io_format);
void Flash_readid(uint8_t* id_buf);
uint8_t Flash_read(uint8_t* rx_buf, uint32_t start_addr, uint32_t size);
uint8_t Flash_erase(void);
uint8_t Flash_erase_block(uint16_t block_number);
uint8_t Flash_program(const uint8_t* tx_buf, uint32_t start_addr, uint32_t size);
void Flash_read_status_regs(uint8_t* regs_out);
uint32_t Flash_scan_for_bad_blocks(uint16_t* bad_blocks_buf);
uint8_t Flash_read_bb_lut(w25_bb_lut_entry_t* lut_ptr);
uint8_t Flash_add_entry_to_bb_lut(uint16_t lba, uint16_t pba);

// SCAI specific management functions
uint8_t scai_set_flash_chip(scai_flash_type_t flash_type, mss_qspi_io_format io_format);
const scai_flash_driver_t* get_scai_flash_driver(void);
scai_flash_type_t get_scai_flash_type(void);
uint8_t scai_flash_test(scai_flash_type_t flash_type);
uint32_t scai_flash_jedec_id(scai_flash_type_t flash_type);
uint8_t scai_fpga_diagnostics(void);
void scai_fpga_write_reg(uintptr_t address, uint32_t value);
uint32_t scai_fpga_read_reg(uintptr_t address);



#ifdef __cplusplus
}
#endif
#endif /* MSS_SCAI_FPGA_H_ */
