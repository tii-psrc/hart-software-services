/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __HSS_SCAI_FPGAQSPI_SERVICE_H
#define __HSS_SCAI_FPGAQSPI_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

bool FPGA_Flash_init(void);
bool FPGA_Flash_getinfo(uint32_t *writesize, uint32_t *erasesize, uint32_t *nandsize);
bool FPGA_Flash_isbad(void);
bool FPGA_Flash_read(uintptr_t dest, uint32_t off, uint64_t len);
bool FPGA_Flash_write(uintptr_t src, uint32_t off, uint64_t len);
bool FPGA_Flash_erase(uint32_t off, uint64_t len);

#ifdef __cplusplus
}
#endif

#endif
