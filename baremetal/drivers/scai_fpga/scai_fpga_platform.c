#include "scai_fpga_platform.h"

#include "hss_types.h"

// Hardware Base Addresses
const uintptr_t MSS_APB_BASE_ADDRESS    = 0x40000000UL;

const uintptr_t GPIO_BASE_ADDRESS       = MSS_APB_BASE_ADDRESS + 0x0100L;
const uintptr_t QSPI_0_BASE_ADDRESS     = MSS_APB_BASE_ADDRESS + 0x0300L;
const uintptr_t QSPI_1_BASE_ADDRESS     = MSS_APB_BASE_ADDRESS + 0x0400L;
const uintptr_t QSPI_2_BASE_ADDRESS     = MSS_APB_BASE_ADDRESS + 0x0500L;

const uintptr_t W25N01_FPGA_BASE_ADDR   = QSPI_0_BASE_ADDRESS;
const uintptr_t W25N01_DIRECT_BASE_ADDR = 0; // It's not used in direct mode

// static const uintptr_t MT29F_BASE_ADDR         = QSPI_1_BASE_ADDRESS;
const uintptr_t MT29F_CHIP_0_BASE_ADDR  = QSPI_1_BASE_ADDRESS + 0x00; // Die 0
const uintptr_t MT29F_CHIP_1_BASE_ADDR  = QSPI_1_BASE_ADDRESS + 0x10; // Die 1
const uintptr_t MT29F_CHIP_2_BASE_ADDR  = QSPI_1_BASE_ADDRESS + 0x20; // Die 2
const uintptr_t MT29F_CHIP_3_BASE_ADDR  = QSPI_1_BASE_ADDRESS + 0x30; // Die 3
const uintptr_t MT29F_CHIP_4_BASE_ADDR  = QSPI_2_BASE_ADDRESS + 0x00; // Die 4
const uintptr_t MT29F_CHIP_5_BASE_ADDR  = QSPI_2_BASE_ADDRESS + 0x10; // Die 5
const uintptr_t MT29F_CHIP_6_BASE_ADDR  = QSPI_2_BASE_ADDRESS + 0x20; // Die 6
const uintptr_t MT29F_CHIP_7_BASE_ADDR  = QSPI_2_BASE_ADDRESS + 0x30; // Die 7

const uintptr_t MT29F_BASE_ADDRS[] = {
    MT29F_CHIP_0_BASE_ADDR,
    MT29F_CHIP_1_BASE_ADDR,
    MT29F_CHIP_2_BASE_ADDR,
    MT29F_CHIP_3_BASE_ADDR,
    MT29F_CHIP_4_BASE_ADDR,
    MT29F_CHIP_5_BASE_ADDR,
    MT29F_CHIP_6_BASE_ADDR,
    MT29F_CHIP_7_BASE_ADDR
};

// GPIO Pins
const uintptr_t GPIO_0_BASE_ADDRESS = GPIO_BASE_ADDRESS + 0x00;
const uintptr_t GPIO_1_BASE_ADDRESS = GPIO_BASE_ADDRESS + 0x10;
const uintptr_t GPIO_2_BASE_ADDRESS = GPIO_BASE_ADDRESS + 0x20;
const uintptr_t GPIO_3_BASE_ADDRESS = GPIO_BASE_ADDRESS + 0x30;
