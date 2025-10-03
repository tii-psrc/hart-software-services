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

#include "hss_types.h"
#include "winbond_w25n01gv_direct.h"

// Struct with the function pointers for a generic flash driver interface
typedef struct {
    void (*init)(scai_fpga_channel_t* channel, mss_qspi_io_format io_format);
    void (*read_id)(scai_fpga_channel_t* channel, uint8_t* id_buf);
    uint8_t (*read)(scai_fpga_channel_t* channel, uint8_t* rx_buf, uint32_t start_addr, uint32_t size);
    uint8_t (*erase)(scai_fpga_channel_t* channel);
    uint8_t (*erase_block)(scai_fpga_channel_t* channel, uint16_t block_number);
    uint8_t (*program)(scai_fpga_channel_t* channel, const uint8_t* tx_buf, uint32_t start_addr, uint32_t size);
    uint8_t (*read_status_regs)(scai_fpga_channel_t* channel, void* regs_out);
    uint32_t (*scan_for_bad_blocks)(scai_fpga_channel_t* channel, uint16_t* bad_blocks_buf);
    uint8_t (*read_bb_lut)(scai_fpga_channel_t* channel, w25_bb_lut_entry_t* lut_ptr);
    uint8_t (*add_entry_to_bb_lut)(scai_fpga_channel_t* channel, uint16_t lba, uint16_t pba);
} scai_flash_driver_t;

// Enum to identify the different supported flash memory types
typedef enum {
    SCAI_MICRON_MT29F          = 0,
    SCAI_WINBOND_W25N01_FPGA   = 1,
    SCAI_MICRON_MT25Q          = 2,
    SCAI_WINBOND_W25N01_DIRECT = 3,
    SCAI_MICRON_MT29F_CHIP_0   = 4,
    SCAI_MICRON_MT29F_CHIP_1   = 5,
    SCAI_MICRON_MT29F_CHIP_2   = 6,
    SCAI_MICRON_MT29F_CHIP_3   = 7,
    SCAI_MICRON_MT29F_CHIP_4   = 8,
    SCAI_MICRON_MT29F_CHIP_5   = 9,
    SCAI_MICRON_MT29F_CHIP_6   = 10,
    SCAI_MICRON_MT29F_CHIP_7   = 11,
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
uint8_t scai_fpga_page_erase(uint8_t chip, uint16_t page);
uint8_t scai_fpga_page_read(uint8_t chip, uint16_t page);
uint8_t scai_fpga_page_write(uint8_t chip, uint16_t page);
uint8_t scai_fpga_stat(uint8_t chip);
uint8_t scai_fpga_reset(uint8_t chip);
uint8_t scai_fpga_manual_init(uint8_t chip);

uint32_t sergio_jedec_id(scai_flash_type_t flash_type);
uint8_t sergio_manual_init(uint8_t chip);
uint8_t sergio_page_erase(uint8_t chip, uint16_t page);
uint8_t sergio_page_read(uint8_t chip, uint16_t page);
uint8_t sergio_fpga_page_write(uint8_t chip, uint16_t page);

#ifdef __cplusplus
}
#endif
#endif /* MSS_SCAI_FPGA_H_ */
