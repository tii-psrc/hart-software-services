#ifndef SCAI_FPGA_PLATFORM_H
#define SCAI_FPGA_PLATFORM_H

#include "hss_types.h"

typedef enum {
    Scai_Fpga_Gpio_Reg_Wdata   =  0,
    Scai_Fpga_Gpio_Reg_Wmask   =  4,
    Scai_Fpga_Gpio_Reg_Wtoggle =  8,
    Scai_Fpga_Gpio_Reg_Rpins   =  0,
    Scai_Fpga_Gpio_Reg_Rdata   =  4,
    Scai_Fpga_Gpio_Reg_Rmask   =  8,
    Scai_Fpga_Gpio_Reg_Rtoggle = 12
} Scai_Fpga_Gpio_Reg_Offsets;

extern const uintptr_t MSS_APB_BASE_ADDRESS;
extern const uintptr_t QSPI_0_BASE_ADDRESS;
extern const uintptr_t QSPI_1_BASE_ADDRESS;
extern const uintptr_t QSPI_2_BASE_ADDRESS;

extern const uintptr_t W25N01_FPGA_BASE_ADDR;
extern const uintptr_t W25N01_DIRECT_BASE_ADDR;

// static const uintptr_t MT29F_BASE_ADDR         = QSPI_1_BASE_ADDRESS;
extern const uintptr_t MT29F_CHIP_0_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_1_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_2_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_3_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_4_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_5_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_6_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_7_BASE_ADDR;

extern const uintptr_t MT29F_BASE_ADDRS[];

extern const uintptr_t GPIO_0_BASE_ADDRESS;
extern const uintptr_t GPIO_1_BASE_ADDRESS;
extern const uintptr_t GPIO_2_BASE_ADDRESS;
extern const uintptr_t GPIO_3_BASE_ADDRESS;

// GPIO Pins

typedef union {
    struct {
        uint32_t A5_C4_I2C_SDA_Z : 1;
        uint32_t E8_C3_I2C_SDA_Z : 1;
        uint32_t B7_C2_I2C_SDA_Z : 1;
        uint32_t C8_C1_I2C_SDA_Z : 1;
        uint32_t Reserved1       : 18;
        uint32_t SDDR_PLL_LOCK   : 1;
        uint32_t CPU_PLL_LOCK    : 1;
        uint32_t FIC3_DLL_LOCK   : 1;
        uint32_t FIC0_DLL_LOCK   : 1;
        uint32_t DDR4_CTRL_RDY   : 1;
        uint32_t A5_C4_I2C_SDA   : 1;
        uint32_t E8_C3_I2C_SDA   : 1;
        uint32_t B7_C2_I2C_SDA   : 1;
        uint32_t C8_C1_I2C_SDA   : 1;
        uint32_t L14_nINT        : 1;
    } bits;
    uint32_t word;
} Scai_Fpga_Gpio_Reg_0_t;

typedef union {
    struct {
        uint32_t A27_CC4_IO0     : 1;
        uint32_t B29_CC2_IO1     : 1;
        uint32_t B15_ENA_CAN     : 1;
        uint32_t C17_ENA_RS1     : 1;
        uint32_t C21_ENA_SS1     : 1;   // MT29F Chip Enable
        uint32_t C26_LED1        : 1;
        uint32_t D15_SOC_P_OFF   : 1;
        uint32_t D26_LED2F       : 1;
        uint32_t B24_CC3_IO1F    : 1;
        uint32_t A24_CC3_IO0F    : 1;
        uint32_t F22_E_PWR_IMU_R : 1;
        uint32_t H18_E_PWR_IMU_N : 1;
        uint32_t R1_E_PWR_C3     : 1;
        uint32_t R2_E_PWR_C4     : 1;
        uint32_t U4_E_PWR_C2     : 1;
        uint32_t V1_E_PWR_C1     : 1;
        uint32_t D16_SOC_IS_NOM0 : 1;
        uint32_t A15_SOC_IS_NOM1 : 1;
        uint32_t D18_SOC_IS_NOM2 : 1;
        uint32_t B25_CC3_IO2     : 1;
        uint32_t A25_CC3_IO3     : 1;
        uint32_t A29_CC2_IO2     : 1;
        uint32_t C27_CC2_IO3     : 1;
        uint32_t B14_FVTT_PGOOD  : 1;
        uint32_t C13_IO_PGOOD    : 1;
        uint32_t D14_SVTT_PGOOD  : 1;
        uint32_t D13_IO_nPFO     : 1;
        uint32_t E15_P_1V2_IMON  : 1;
        uint32_t E16_P_1V2_PGOOD : 1;
        uint32_t E17_P_5V0_nFLT  : 1;
        uint32_t E18_P_5V0_PGOOD : 1;
        uint32_t F15_P_3V3_IMON  : 1;
    } bits;
    uint32_t word;
} Scai_Fpga_Gpio_Reg_1_t;

typedef union {
    struct {
        uint32_t A23_ENA_SS2     : 1;   // MT29F Chip Enable
        uint32_t K17_ENA         : 1;
        uint32_t J14_nRst        : 1;
        uint32_t L15_MDC         : 1;
        uint32_t B22_CC2_IO1     : 1;
        uint32_t E23_CC2_IO0     : 1;
        uint32_t E21_CC1_IO1     : 1;
        uint32_t D21_CC1_IO0     : 1;
        uint32_t F20_ENA_RS2     : 1;   // RS-422 Enable
        uint32_t D8_C1_I2C_SCL   : 1;
        uint32_t C7_C2_I2C_SCL   : 1;
        uint32_t F8_C3_I2C_SCL   : 1;
        uint32_t B6_C4_I2C_SCL   : 1;
        uint32_t A12_IO_WDI_DATA : 1;
        uint32_t FOO             : 1;
        uint32_t J18_P_2V5_IMON  : 1;        
        uint32_t H17_P_1V8_IMON  : 1;
        uint32_t H16_P_1V0_PGOOD : 1;
        uint32_t G17_P_1V8_PGOOD : 1;
        uint32_t G16_P_1V0_IMON  : 1;        
        uint32_t G15_P_3V3_PGOOD : 1;
        uint32_t A13_IO_nWDO     : 1;
        uint32_t V2_PGOOD_C1     : 1;
        uint32_t T4_PGOOD_C2     : 1;        
        uint32_t K18_P_2V5_PGOOD : 1;
        uint32_t H14_MDIO        : 1;
        uint32_t B21_CC2_IO3     : 1;
        uint32_t C22_CC2_IO2     : 1;        
        uint32_t D24_CC1_IO3     : 1;
        uint32_t D25_CC1_IO2     : 1;
        uint32_t R3_PGOOD_C4     : 1;
        uint32_t P1_PGOOD_C3     : 1;
    } bits;
    uint32_t word;
} Scai_Fpga_Gpio_Reg_2_t;

#endif /* SCAI_FPGA_PLATFORM_H */