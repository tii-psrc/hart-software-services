/*
 * micron_mt29f_fpga.h
 *
 * Driver for Micron MT29F series NAND flash memory using a memory-mapped
 * QSPI controller on the PolarFire SoC FPGA fabric.
 * This header provides the public API for the driver.
 *
 * Copyright 2024, NAVC
 */

#ifndef MICRON_MT29F_FPGA_H
#define MICRON_MT29F_FPGA_H

#include "scai_fpga_common.h" // For scai_fpga_channel_t
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const uint32_t PAGE_SIZE_BYTES;
extern const uint32_t PAGES_PER_BLOCK;
extern const uint16_t TOTAL_BLOCKS;
extern const uint32_t BLOCK_SIZE_BYTES;
typedef struct {
    uint8_t lock;
    uint8_t config;
    uint8_t status;
    uint8_t die_select;
} mt29f_status_regs_t;

typedef enum {
    SCAI_MT29F_CHIP_0 = 0,
    SCAI_MT29F_CHIP_1 = 1,
    SCAI_MT29F_CHIP_2 = 2,
    SCAI_MT29F_CHIP_3 = 3,
    SCAI_MT29F_CHIP_4 = 4,
    SCAI_MT29F_CHIP_5 = 5,
    SCAI_MT29F_CHIP_6 = 6,
    SCAI_MT29F_CHIP_7 = 7
} scai_mt29f_chip_select_t;

/**
 * @brief Initializes the driver and the MT29F flash device.
 * @param channel   Pointer to the QSPI channel context.
 * @param io_format The desired I/O format (e.g., MSS_QSPI_NORMAL or MSS_QSPI_QUAD_FULL).
 */
void SCAI_MT29_Flash_init(scai_fpga_channel_t* channel, mss_qspi_io_format io_format);

/**
 * @brief Reads the JEDEC ID from the flash device.
 * @param channel   Pointer to the QSPI channel context.
 * @param id_buf A buffer of at least 2 bytes to store the manufacturer and device ID. Must not be NULL.
 */
void SCAI_MT29_Flash_readid(scai_fpga_channel_t* channel, uint8_t* id_buf);

/**
 * @brief Reads a block of data from the flash.
 * @param channel   Pointer to the QSPI channel context.
 * @param buf Pointer to the destination buffer. Must not be NULL.
 * @param addr The logical starting address to read from.
 * @param len The number of bytes to read.
 * @return 0 on success, 1 on failure.
 */
uint8_t SCAI_MT29_Flash_read(scai_fpga_channel_t* channel, uint8_t* buf, uint32_t addr, uint32_t len);

/**
 * @brief Erases the entire flash device.
 * @param channel   Pointer to the QSPI channel context.
 * @return 0 on success, 1 on failure.
 */
uint8_t SCAI_MT29_Flash_erase(scai_fpga_channel_t* channel);

/**
 * @brief Erases a single block of the flash device.
 * @param channel   Pointer to the QSPI channel context.
 * @param block_nb The logical block number to erase.
 * @return 0 on success, 1 on failure.
 */
uint8_t SCAI_MT29_Flash_erase_block(scai_fpga_channel_t* channel, uint16_t block_nb);

/**
 * @brief Programs (writes) data to the flash.
 * @param channel   Pointer to the QSPI channel context.
 * @param buf Pointer to the source data buffer. Must not be NULL.
 * @param addr The logical starting address to write to.
 * @param len The number of bytes to write.
 * @return 0 on success, 1 on failure.
 */
uint8_t SCAI_MT29_Flash_program(scai_fpga_channel_t* channel, const uint8_t* buf, uint32_t addr, uint32_t len);

/**
 * @brief Reads the four main feature/status registers of the flash device.
 * @param channel   Pointer to the QSPI channel context.
 * @param regs_out A pointer to mt29f_status_regs_t struct to store the register values.
 */
uint8_t SCAI_MT29_Flash_read_status_regs(scai_fpga_channel_t* channel, void* regs_out);

uint8_t SCAI_MT29_Flash_get_status(scai_fpga_channel_t* channel);
void SCAI_MT29_Flash_reset(scai_fpga_channel_t* channel);

#ifdef __cplusplus
}
#endif

#endif // MICRON_MT29F_FPGA_H
