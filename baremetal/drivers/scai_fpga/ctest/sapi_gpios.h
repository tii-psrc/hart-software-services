/*
 * gpio.h
 *
 *  Created on: 6 Oct 2024
 *      Author: Sergio.Sirota
 */

#ifndef IOB_GPIOS_H_
#define IOB_GPIOS_H_

#include "ctest/sapi_hw_platform.h"

#include "hss_types.h"

struct pin_st
{
    int instance;
    int mask;
};

typedef struct pin_st PIN;

struct gpio_st
{
    uint32_t address;

    uint32_t out_data, data, mask, toggling;
};

typedef struct gpio_st gpio;

typedef enum
{
        GPIO0 = 0,
        GPIO1,
#if defined(NAVC_BOARD)
        GPIO2,
        GPIO3,
#endif
        MAX_GPIOS
} GPIOS_ENUM;

typedef enum
{
        WDATA = 0,
        WMASK = 4,
        WTOGGLE = 8,
        RPINES = 0,
        RDATA = 4,
        RMASK = 8,
        RTOGGLE = 12
} GPIOS_REGS;

extern volatile gpio GPIOS[MAX_GPIOS];

void init_gpios(void);
void set_gpio(PIN bit, int value);
void set_gpio_direction(PIN bit, int value);
int toggle_gpio(PIN bit);
int pulse_gpio(PIN bit);
int get_gpio(PIN bit);
int get_gpio_direction(PIN bit);
int get_full_data_gpio(PIN bit, int *d, int *pin, int *z);
void set_gpos(int instance, uint32_t mask_bit, uint32_t values);
int enable_toggling(uint8_t ENAnDIS);
// uint16_t gpio_printf(int instance, const char *format, ...);

void test_gpios(void);

/***************************************************************************//**
 * HW_set_32bit_reg is used to write the content of a 32 bits wide peripheral
 * register.
 *
 * @param reg_addr  Address in the processor's memory map of the register to
 *                  write.
 * @param value     Value to be written into the peripheral register.
 *
void
HW_set_32bit_reg
(
    addr_t reg_addr,
    uint32_t value
);

***************************************************************************//**
 * HW_get_32bit_reg is used to read the content of a 32 bits wide peripheral
 * register.
 *
 * @param reg_addr  Address in the processor's memory map of the register to
 *                  read.
 * @return          32 bits value read from the peripheral register.
 *
uint32_t
HW_get_32bit_reg
(
    addr_t reg_addr
);
*/

#if defined(NAVC_BOARD)
    extern const PIN C8_C1_I2C_SDA;
    extern const PIN D8_C1_I2C_SCL;
    extern const PIN B7_C2_I2C_SDA;
    extern const PIN C7_C2_I2C_SCL;

    extern const PIN E8_C3_I2C_SDA;
    extern const PIN F8_C3_I2C_SCL;
    extern const PIN A5_C4_I2C_SDA;
    extern const PIN B6_C4_I2C_SCL;

    extern const PIN F20_ENA_RS2;
    extern const PIN G15_P_3V3_PGOOD;
    extern const PIN G16_P_1V0_IMON;
    extern const PIN G17_P_1V8_PGOOD;

    extern const PIN H16_P_1V0_PGOOD;
    extern const PIN H17_P_1V8_IMON;
    extern const PIN J18_P_2V5_IMON;
    extern const PIN H1_CD3_D0_S_N;
    extern const PIN G1_CD3_D0_S_P;
    extern const PIN H2_CD3_D1_S_N;
    extern const PIN H3_CD3_D1_S_P;
    extern const PIN K1_CD3_D2_S_N;
    extern const PIN J1_CD3_D2_S_P;
    extern const PIN K2_CD3_D3_S_N;
    extern const PIN J3_CD3_D3_S_P;
    extern const PIN U1_CD3_CLK_S_N;
    extern const PIN U2_CD3_CLK_S_P;
    extern const PIN E1_CD4_D0_S_N;
    extern const PIN F2_CD4_D0_S_P;
    extern const PIN G2_CD4_D1_S_N;
    extern const PIN F3_CD4_D1_S_P;
    extern const PIN N11_CD4_D2_S_N;
    extern const PIN M11_CD4_D2_S_P;

    extern const PIN A24_CC3_IO0;
    extern const PIN B24_CC3_IO1;
    extern const PIN B25_CC3_IO2;
    extern const PIN A25_CC3_IO3;

    extern const PIN A27_CC4_IO0;
    extern const PIN B29_CC2_IO1;
    extern const PIN A29_CC2_IO2;
    extern const PIN C27_CC2_IO3;

    extern const PIN B14_FVTT_PGOOD;
    extern const PIN B15_ENA_CAN;
    extern const PIN C13_IO_PGOOD;
    extern const PIN C14_SVTT_ENA;

    extern const PIN C17_ENA_RS1;
    extern const PIN C21_ENA_SS1;
    extern const PIN C26_LED1;
    extern const PIN D13_IO_nPFO;

    extern const PIN D14_SVTT_PGOOD;
    extern const PIN D15_SOC_P_OFF;
    extern const PIN D26_LED2;
    extern const PIN E15_P_1V2_IMON;

    extern const PIN E16_P_1V2_PGOOD;
    extern const PIN E17_P_5V0_nFLT;
    extern const PIN E18_P_5V0_PGOOD;
    extern const PIN F15_P_3V3_IMON;
    extern const PIN H18_E_PWR_IMU_N;
    extern const PIN F22_E_PWR_IMU_R;
    extern const PIN L14_nINT;
    extern const PIN L15_MDC;

    extern const PIN H14_MDIO;
    extern const PIN J14_nRst;
    extern const PIN K17_ENA;
    extern const PIN K18_P_2V5_PGOOD;


    extern const PIN N14_ENA_CLK_50M;
    extern const PIN P1_PGOOD_C3;
    extern const PIN R3_PGOOD_C4;
    extern const PIN R1_E_PWR_C3;

    extern const PIN R2_E_PWR_C4;
    extern const PIN U4_E_PWR_C2;
    extern const PIN V1_E_PWR_C1;
    extern const PIN T4_PGOOD_C2;

    extern const PIN V2_PGOOD_C1;
    extern const PIN A12_IO_WDI;
    extern const PIN A13_IO_nWDO;
    extern const PIN A14_FVTT_ENA;

    extern const PIN A15_SOC_IS_NOM1;
    extern const PIN A23_ENA_SS2;
    extern const PIN D21_CC1_IO0;
    extern const PIN E21_CC1_IO1;

    extern const PIN D25_CC1_IO2;
    extern const PIN D24_CC1_IO3;
    extern const PIN E23_CC2_IO0;
    extern const PIN B22_CC2_IO1;

    extern const PIN C22_CC2_IO2;
    extern const PIN B21_CC2_IO3;
    extern const PIN D16_SOC_IS_NOM0;
    extern const PIN D18_SOC_IS_NOM2;
    extern const PIN L8_CD1_D0_S_N;
    extern const PIN K8_CD1_D0_S_P;
    extern const PIN M7_CD1_D1_S_N;
    extern const PIN L7_CD1_D1_S_P;

    typedef enum
    {
    //GPIO0

        A5_C4_I2C_SDA_Z_MASK            = 1,
        E8_C3_I2C_SDA_Z_MASK            = 2,
        B7_C2_I2C_SDA_Z_MASK            = 4,
        C8_C1_I2C_SDA_Z_MASK            = 8,


        SDDR_PLL_LOCK_MASK              = 0x400000,
        CPU_PLL_LOCK_MASK               = 0x800000,

        FIC3_DLL_LOCK_MASK              = 0x1000000,
        FIC0_DLL_LOCK_MASK              = 0x2000000,
        DDR4_CTRL_RDY_MASK              = 0x4000000,
        A5_C4_I2C_SDA_MASK              = 0x8000000,

        E8_C3_I2C_SDA_MASK              = 0x10000000,
        B7_C2_I2C_SDA_MASK              = 0x20000000,
        C8_C1_I2C_SDA_MASK              = 0x40000000,
        L14_nINT_MASK                   = 0x80000000,



    //GPIO1

        A27_CC4_IO0_MASK                = 1,
        B29_CC2_IO1_MASK                = 2,
        B15_ENA_CAN_MASK                = 4,
        C17_ENA_RS1_MASK                = 8,

        C21_ENA_SS1_MASK                = 0x10,
        C26_LED1_MASK                   = 0x20,
        D15_SOC_P_OF_MASK               = 0x40,
        D26_LED2_MASK                   = 0x80,

        B24_CC3_IO1_MASK                = 0x100,
        A24_CC3_IO0_MASK_MASK           = 0x200,
        F22_E_PWR_IMU_R_MASK            = 0x400,
        H18_E_PWR_IMU_N_MASK            = 0x800,

        R1_E_PWR_C3_MASK                = 0x1000,
        R2_E_PWR_C4_MASK                = 0x2000,
        U4_E_PWR_C2_MASK                = 0x4000,
        V1_E_PWR_C1_MASK                = 0x8000,

        D16_SOC_IS_NOM0_MASK            = 0x10000,
        A15_SOC_IS_NOM1_MASK            = 0x20000,
        D18_SOC_IS_NOM2_MASK            = 0x40000,
        B25_CC3_IO2_MASK                = 0x80000,

        A25_CC3_IO3_MASK                = 0x100000,
        A29_CC2_IO2_MASK                = 0x200000,
        C27_CC2_IO3_MASK                = 0x400000,
        B14_FVTT_PGOOD_MASK             = 0x800000,

        C13_IO_PGOOD_MASK               = 0x1000000,
        D14_SVTT_PGOOD_MASK             = 0x2000000,
        D13_IO_nPFO_MASK                = 0x4000000,
        E15_P_1V2_IMON_MASK             = 0x8000000,

        E16_P_1V2_PGOOD_MASK            = 0x10000000,
        E17_P_5V0_nFLT_MASK             = 0x20000000,
        E18_P_5V0_PGOOD_MASK            = 0x40000000,
        F15_P_3V3_IMON_MASK             = 0x80000000,



        //GPIO2
        A23_ENA_SS2_MASK                = 0x1,
        K17_ENA_MASK                    = 0x2,
        J14_nRst_MASK                   = 0x4,
        L15_MDC_MASK                    = 0x8,

        B22_CC2_IO1_MASK                = 0x10,
        E23_CC2_IO0_MASK                = 0x20,
        E21_CC1_IO1_MASK                = 0x40,
        D21_CC1_IO0_MASK                = 0x80,

        F20_ENA_RS2_MASK                = 0x100,
        D8_C1_I2C_SCL_MASK              = 0x200,
        C7_C2_I2C_SCL_MASK              = 0x400,
        F8_C3_I2C_SCL_MASK              = 0x800,

        B6_C4_I2C_SCL_MASK              = 0x1000,
        A12_IO_WDI_DATA_MASK            = 0x2000,
        FOO_MASK                        = 0x4000,
        J18_P_2V5_IMON_MASK             = 0x8000,

        H17_P_1V8_IMON_MASK             = 0x10000,
        H16_P_1V0_PGOOD_MASK            = 0x20000,
        G17_P_1V8_PGOOD_MASK            = 0x40000,
        G16_P_1V0_IMON_MASK             = 0x80000,

        G15_P_3V3_PGOOD_MASK            = 0x100000,
        A13_IO_nWDO_MASK                = 0x200000,
        V2_PGOOD_C1_MASK                = 0x400000,
        T4_PGOOD_C2_MASK                = 0x800000,

        K18_P_2V5_PGOOD_MASK            = 0x1000000,
        H14_MDIO_MASK                   = 0x2000000,
        B21_CC2_IO3_MASK                = 0x4000000,
        C22_CC2_IO2_MASK                = 0x8000000,

        D24_CC1_IO3_MASK                = 0x10000000,
        D25_CC1_IO2_MASK                = 0x20000000,
        R3_PGOOD_C4_MASK                = 0x40000000,
        P1_PGOOD_C3_MASK                = 0x80000000,


    }RealPinesMasks;


#else
extern     const PIN J16_CFG2;
extern     const PIN R2_ETH_MDINT;
extern     const PIN F15_CTRL_PGOOD0;
extern     const PIN G15_CTRL_nIFLT0;
extern     const PIN N9_ETH_PG;
extern     const PIN J18_CTRL_PGOOD1;
extern     const PIN H18_CTRL_nIFLT1;
extern     const PIN H17_CTRL_PGOOD2;
extern     const PIN G17_CTRL_nIFLT2;
extern     const PIN F17_CTRL_PGOOD3;
extern     const PIN F18_CTRL_nIFLT3;
extern     const PIN F23_IO_nPFO;
extern     const PIN A4_IS_IsNominal0;
extern     const PIN A3_IS_IsNominal1;
extern     const PIN F5_IS_IsNominal2;
extern     const PIN E13_CTRL_PGOOD4;
extern     const PIN H16_SVTT_ENA;
extern     const PIN E16_FVTT_ENA;
extern     const PIN L5_ETH_COMA_MODE;
extern     const PIN M9_ETH_nReset;
extern     const PIN N14_ENA_CLK_50M;
extern     const PIN C3_NAND_PWR_2_ENA;
extern     const PIN D5_B_SNOR_nRESET;
extern     const PIN D13_CTRL_ENABLE;
extern     const PIN A13_CTRL_SOC_P_OFF;
extern     const PIN C14_MI1_GPIO2;
extern     const PIN MI1_I2C_SDA_CTRL;
extern     const PIN A14_MI1_I2C_SCL;
extern     const PIN B14_CAMS_PWR_C1_ENA;
extern     const PIN D15_MI2_GPIO2;
extern     const PIN MI2_I2C_SDA_CTRL;
extern     const PIN B15_CAM2_PWR_ENA;
extern     const PIN F14_CTRL_nIFLT4;
extern     const PIN B12_CTRL_nIFLT5;
extern     const PIN C12_CTRL_PGOOD6;
extern     const PIN C13_CTRL_PGOOD7;
extern     const PIN A12_MI1_GPIO1;
extern     const PIN MI1_I2C_SDA_DIN;
extern     const PIN E18_MI2_GPIO1;
extern     const PIN MI2_I2C_SDA_D;
extern     const PIN A15_MI2_I2C_SCL;
extern     const PIN D21_IO_nWDO;
extern     const PIN eMMC_WR_FIFORFM;
extern     const PIN F20_COMMS_GPIO1;
extern     const PIN eMMC_RD_FIFO_DAVAIL;
extern     const PIN K15_F_SD_CD;
extern     const PIN eMMC_INTERRUPT;
extern     const PIN K18_CFG1;
extern     const PIN D18_CAMS_PWR_TEL_ENA;
extern     const PIN A19_COMMS_GPIO2;
extern     const PIN A18_PWR_ETH_ENA;
extern     const PIN J15_F_SD_ENA;
extern     const PIN D19_SOC_MON_OUT1;
extern     const PIN B20_SOC_MON_OUT2;
extern     const PIN E21_IO_MR;
extern     const PIN D25_IO_WDI;
extern     const PIN A30_NAND_PWR_1_ENA;

extern     const PIN PIN_LED1;
extern     const PIN PIN_LED2;



    typedef enum
     {
     //GPIO0
        J16_CFG2_MASK                   = 0x10000,
       R2_ETH_MDINT_MASK                = 0x20000,
       F15_CTRL_PGOOD0_MASK             = 0x40000,
       G15_CTRL_nIFLT0_MASK             = 0x80000,
       N9_ETH_PG_MASK                   = 0x100000,
       J18_CTRL_PGOOD1_MASK             = 0x200000,
       H18_CTRL_nIFLT1_MASK             = 0x400000,
       H17_CTRL_PGOOD2_MASK             = 0x800000,
       G17_CTRL_nIFLT2_MASK             = 0x1000000,
       F17_CTRL_PGOOD3_MASK             = 0x2000000,
       F18_CTRL_nIFLT3_MASK             = 0x4000000,
       F23_IO_nPFO_MASK                 = 0x8000000,
       A4_IS_IsNominal0_MASK            = 0x10000000,
       A3_IS_IsNominal1_MASK            = 0x20000000,
       F5_IS_IsNominal2_MASK            = 0x40000000,
       E13_CTRL_PGOOD4_MASK             = 0x80000000,
       H16_SVTT_ENA_MASK                = 0x1,
       E16_FVTT_ENA_MASK                = 0x2,
       L5_ETH_COMA_MODE_MASK            = 0x4,
       M9_ETH_nReset_MASK               = 0x8,
       N14_ENA_CLK_50M_MASK             = 0x10,
       C3_NAND_PWR_2_ENA_MASK           = 0x20,
       D5_B_SNOR_nRESET_MASK            = 0x40,
       D13_CTRL_ENABLE_MASK             = 0x80,
       A13_CTRL_SOC_P_OFF_MASK          = 0x100,
       C14_MI1_GPIO2_MASK               = 0x200,
       MI1_I2C_SDA_CTRL_MASK            = 0x400,
       A14_MI1_I2C_SCL_MASK             = 0x800,
       B14_CAMS_PWR_C1_ENA_MASK         = 0x1000,
       D15_MI2_GPIO2_MASK               = 0x2000,
       MI2_I2C_SDA_CTRL_MASK            = 0x4000,
       B15_CAM2_PWR_ENA_MASK            = 0x8000,
       //GPIO1
       F14_CTRL_nIFLT4_MASK             = 0x10000,
       B12_CTRL_nIFLT5_MASK             = 0x20000,
       C12_CTRL_PGOOD6_MASK             = 0x40000,
       C13_CTRL_PGOOD7_MASK             = 0x80000,
       A12_MI1_GPIO1_MASK               = 0x100000,
       MI1_I2C_SDA_DIN_MASK             = 0x200000,
       E18_MI2_GPIO1_MASK               = 0x400000,
       MI2_I2C_SDA_DIN_MASK             = 0x800000,
       A15_MI2_I2C_SCL_MASK             = 0x1000000,
       D21_IO_nWDO_MASK                 = 0x2000000,
       eMMC_WR_FIFORFM_MASK             = 0x4000000,
       F20_COMMS_GPIO1_MASK             = 0x8000000,
       eMMC_RD_FIFO_DAVAIL_MASK         = 0x10000000,
       K15_F_SD_CD_MASK                 = 0x20000000,
       eMMC_INTERRUPT_MASK              = 0x40000000,
       K18_CFG1_MASK                    = 0x80000000,
       D18_CAMS_PWR_TEL_ENA_MASK        = 0x1,
       A19_COMMS_GPIO2_MASK             = 0x2,
       A18_PWR_ETH_ENA_MASK             = 0x4,
       J15_F_SD_ENA_MASK                = 0x8,
       D19_SOC_MON_OUT1_MASK            = 0x10,
       B20_SOC_MON_OUT2_MASK            = 0x20,
       E21_IO_MR_MASK                   = 0x40,
       D25_IO_WDI_MASK                  = 0x80,
       A30_NAND_PWR_1_ENA_MASK          = 0x100,

    LED1_MASK                        = D19_SOC_MON_OUT1_MASK,
    LED2_MASK                        = B20_SOC_MON_OUT2_MASK,
}RealPinesMasks;

#endif
void set_led1(uint8_t value);
void set_led2(uint8_t value);


#endif /* IOB_GPIOS_H_ */
