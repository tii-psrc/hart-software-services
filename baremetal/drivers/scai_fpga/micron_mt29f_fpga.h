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

#include "drivers/mss/mss_qspi/mss_qspi.h" // For mss_qspi_io_format enum
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t lock;
    uint8_t config;
    uint8_t status;
    uint8_t die_select;
} mt29f_status_regs_t;

/**
 * @brief Initializes the driver and the MT29F flash device.
 * @param io_format The desired I/O format (e.g., MSS_QSPI_NORMAL or MSS_QSPI_QUAD_FULL).
 */
void SCAI_MT29_Flash_init(uintptr_t base_addr, mss_qspi_io_format io_format);

/**
 * @brief Reads the JEDEC ID from the flash device.
 * @param id_buf A buffer of at least 2 bytes to store the manufacturer and device ID. Must not be NULL.
 */
void SCAI_MT29_Flash_readid(uintptr_t base_addr, uint8_t* id_buf);

/**
 * @brief Reads a block of data from the flash. Handles page boundaries correctly.
 * @param buf Pointer to the destination buffer. Must not be NULL.
 * @param addr The logical starting address to read from.
 * @param len The number of bytes to read.
 * @return 0 on success, 1 on failure (timeout or invalid parameters).
 */
uint8_t SCAI_MT29_Flash_read(uintptr_t base_addr, uint8_t* buf, uint32_t addr, uint32_t len);

/**
 * @brief Erases the entire flash device by erasing all blocks sequentially.
 * @note This is a long-running operation.
 * @return 0 on success, 1 on failure.
 */
uint8_t SCAI_MT29_Flash_erase(uintptr_t base_addr);

/**
 * @brief Erases a single block of the flash device.
 * @param block_nb The logical block number to erase (0 to 4095).
 * @return 0 on success, 1 on failure.
 */
uint8_t SCAI_MT29_Flash_erase_block(uintptr_t base_addr, uint16_t block_nb);

/**
 * @brief Programs (writes) data to the flash.
 * @note The target area must be erased first. This function handles page boundaries.
 * @param buf Pointer to the source data buffer. Must not be NULL.
 * @param addr The logical starting address to write to.
 * @param len The number of bytes to write.
 * @return 0 on success, 1 on failure.
 */
uint8_t SCAI_MT29_Flash_program(uintptr_t base_addr, const uint8_t* buf, uint32_t addr, uint32_t len);

/**
 * @brief Reads the four main feature/status registers of the flash device.
 * @param buf A buffer of at least 4 bytes to store the register values
 * (Lock, Config, Status, Die Select). Must not be NULL.
 */
uint8_t SCAI_MT29_Flash_read_status_regs(uintptr_t base_addr, void * regs_out);

#ifdef __cplusplus
}
#endif

#endif // MICRON_MT29F_FPGA_H
