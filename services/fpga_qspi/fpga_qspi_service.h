/* SPDX-License-Identifier: GPL-2.0 */

#ifndef HSS_FPGA_QSPI_SERVICE_H
#define HSS_FPGA_QSPI_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hss_types.h"
bool HSS_FPGA_QSPIInit(void);
void HSS_FPGA_GetInfo(uint32_t *pBlockSize, uint32_t *pEraseSize, uint32_t *pBlockCount);
bool HSS_FPGA_QSPIIsbad(void);
bool HSS_FPGA_QSPIRead(uintptr_t dest, uint32_t off, uint64_t len);
bool HSS_FPGA_QSPIWrite(uintptr_t src, uint32_t off, uint64_t len);
bool HSS_FPGA_QSPIErase(uint32_t off, uint64_t len);

#ifdef __cplusplus
}
#endif

#endif
