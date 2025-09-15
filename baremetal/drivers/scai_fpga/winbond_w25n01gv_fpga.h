/*
 * winbond_w25n01gv_fpga.h
 *
 * APIs for the Winbond W25N01GV NAND flash driver using a memory-mapped QSPI
 * controller on the PolarFire SoC FPGA fabric.
 *
 * Copyright 2024, NAVC
 */

#ifndef WINBOND_W25N01GV_FPGA_H
#define WINBOND_W25N01GV_FPGA_H

#include <stdint.h>
#include "drivers/mss/mss_qspi/mss_qspi.h"
#include "winbond_w25n01gv_direct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initializes the driver and the W25N flash device.
 * @param io_format The desired I/O format (e.g., MSS_QSPI_NORMAL or MSS_QSPI_QUAD_FULL).
 */
void Scai_W25_Fpga_Flash_init(mss_qspi_io_format io_format);

/**
 * @brief Reads the JEDEC ID from the flash device.
 * @param id_buf A buffer of at least 3 bytes to store the manufacturer and device ID.
 */
void Scai_W25_Fpga_Flash_readid(uint8_t* id_buf);

/**
 * @brief Reads a block of data from the flash. Handles page boundaries correctly.
 * @param buf Pointer to the destination buffer. Must not be NULL.
 * @param addr The logical starting address to read from.
 * @param len The number of bytes to read.
 * @return 0 on success, 1 on failure (timeout or invalid parameters).
 */
uint8_t Scai_W25_Fpga_Flash_read(uint8_t* buf, uint32_t addr, uint32_t len);

/**
 * @brief Erases the entire flash device by erasing all blocks sequentially.
 * @note This is a long-running operation.
 * @return 0 on success, 1 on failure.
 */
uint8_t Scai_W25_Fpga_Flash_erase(void);

/**
 * @brief Erases a single block of the flash device.
 * @param block_nb The logical block number to erase (0 to 1023).
 * @return 0 on success, 1 on failure.
 */
uint8_t Scai_W25_Fpga_Flash_erase_block(uint16_t block_nb);

/**
 * @brief Programs (writes) data to the flash.
 * @note The target area must be erased first. This function handles page boundaries.
 * @param buf Pointer to the source data buffer. Must not be NULL.
 * @param addr The logical starting address to write to.
 * @param len The number of bytes to write.
 * @return 0 on success, 1 on failure.
 */
uint8_t Scai_W25_Fpga_Flash_program(const uint8_t* buf, uint32_t addr, uint32_t len);

/**
 * @brief Reads the three main feature/status registers of the flash device.
 * @param buf A buffer of at least 3 bytes to store the register values
 * (Protection, Configuration, Status). Must not be NULL.
 */
void Scai_W25_Fpga_Flash_read_status_regs(uint8_t * buf);

/**
 * @brief Scans the device for factory-marked bad blocks.
 * @note This functionality is a placeholder. A robust implementation requires
 * reading the spare area of each page, which is not supported by the QSPI controller.
 * @param buf A buffer to store the identified bad block numbers.
 * @return The number of bad blocks found (always 0 in this implementation).
 */
uint32_t Scai_W25_Fpga_Flash_scan_for_bad_blocks(uint16_t* buf);

/**
 * @brief Reads the Bad Block Management Look-Up Table (LUT).
 * @note This functionality is a placeholder.
 * @param lut_ptr A pointer to an array to store the LUT entries.
 * @return The number of valid entries found (always 0 in this implementation).
 */
uint8_t Scai_W25_Fpga_Flash_read_bb_lut(w25_bb_lut_entry_t* lut_ptr);

#ifdef __cplusplus
}
#endif

#endif /* WINBOND_W25N01GV_FPGA_H */
