/* SPDX-License-Identifier: GPL-2.0 */

#include "hss_types.h"
#include "hss_debug.h"

#include "fpga_qspi_service.h"
#include "scai_fpgaqspi_service.h"

bool HSS_FPGA_QSPIInit(void)
{
	return FPGA_Flash_init();
}

void HSS_FPGA_GetInfo(uint32_t *pBlockSize, uint32_t *pEraseSize,
		uint32_t *pBlockCount)
{
	FPGA_Flash_getinfo(pBlockSize, pEraseSize, pBlockCount);
}

bool HSS_FPGA_QSPIIsbad(void)
{
	return FPGA_Flash_isbad();
}

bool HSS_FPGA_QSPIRead(uintptr_t dest, uint32_t off, uint64_t len)
{
	return FPGA_Flash_read(dest, off, len);
}

bool HSS_FPGA_QSPIWrite(uintptr_t src, uint32_t off, uint64_t len)
{
	return FPGA_Flash_write(src, off, len);
}

bool HSS_FPGA_QSPIErase(uint32_t off, uint64_t len)
{
	return FPGA_Flash_erase(off, len);
}

