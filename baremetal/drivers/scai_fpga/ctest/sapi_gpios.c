/*
 * gpio.c
 *
 *  Created on: 6 Oct 2024
 *      Author: Sergio.Sirota
 */



#include "ctest/sapi_gpios.h"
#include "ctest/sapi_hw_platform.h"
#include "ctest/reg_mitm.h"

#include <string.h>
#include "hss_types.h"
#include "hss_debug.h"

/******************************************************************************
 * GPIO instance data.
 *****************************************************************************/

volatile gpio GPIOS[MAX_GPIOS];


////////////////////////////////////////////////////////////////////////////////////////////

#if defined(NAVC_BOARD)


/*    const PIN SDDR_PLL_LOCK             = {0, 22};
    const PIN CPU_PLL_LOCK              = {0, 23};
    const PIN FIC3_DLL_LOCK             = {0, 24};
    const PIN FIC0_DLL_LOCK             = {0, 25};
*/
    const PIN DDR4_CTRL_RDY             = {0, 26};
    const PIN A5_C4_I2C_SDA             = {0, 27};
    const PIN E8_C3_I2C_SDA             = {0, 28};
    const PIN B7_C2_I2C_SDA             = {0, 29};
    const PIN C8_C1_I2C_SDA             = {0, 30};
    const PIN L14_nINT                  = {0, 31};
    const PIN A5_C4_I2C_SDA_Z           = {0, 0};
    const PIN E8_C3_I2C_SDA_Z           = {0, 1};
    const PIN B7_C2_I2C_SDA_Z           = {0, 2};
    const PIN C8_C1_I2C_SDA_Z           = {0, 3};


    const PIN A27_CC4_IO0           = {1, 0};
    const PIN B29_CC2_IO1           = {1, 1};
    const PIN B15_ENA_CAN           = {1, 2};
    const PIN C17_ENA_RS1           = {1, 3};   //Stuck High in Libero

    const PIN C21_ENA_SS1           = {1, 4};
    const PIN C26_LED1              = {1, 5};
    const PIN D15_SOC_P_OFF         = {1, 6};
    const PIN D26_LED2              = {1, 7};

    const PIN B24_CC3_IO1           = {1, 8};
    const PIN A24_CC3_IO0           = {1, 9};
    const PIN F22_E_PWR_IMU_R       = {1, 10};
    const PIN H18_E_PWR_IMU_N       = {1, 11};

    const PIN R1_E_PWR_C3           = {1, 12};
    const PIN R2_E_PWR_C4           = {1, 13};
    const PIN U4_E_PWR_C2           = {1, 14};
    const PIN V1_E_PWR_C1           = {1, 15};


    const PIN D16_SOC_IS_NOM0       = {1, 16};
    const PIN A15_SOC_IS_NOM1       = {1, 17};
    const PIN D18_SOC_IS_NOM2       = {1, 18};
    const PIN B25_CC3_IO2           = {1, 19};

    const PIN A25_CC3_IO3           = {1, 20};
    const PIN A29_CC2_IO2           = {1, 21};
    const PIN C27_CC2_IO3           = {1, 22};
    const PIN B14_FVTT_PGOOD        = {1, 23};

    const PIN C13_IO_PGOOD          = {1, 24};
    const PIN D14_SVTT_PGOOD        = {1, 25};
    const PIN D13_IO_nPFO           = {1, 26};
    const PIN E15_P_1V2_IMON        = {1, 27};

    const PIN E16_P_1V2_PGOOD       = {1, 28};
    const PIN E17_P_5V0_nFLT        = {1, 29};
    const PIN E18_P_5V0_PGOOD       = {1, 30};
    const PIN F15_P_3V3_IMON        = {1, 31};

    //-------------------------GPIO2------------------------------

    const PIN A23_ENA_SS2           = {2, 0};
    const PIN K17_ENA               = {2, 1};
    const PIN J14_nRst              = {2, 2};
    const PIN L15_MDC               = {2, 3};

    const PIN B22_CC2_IO1           = {2, 4};
    const PIN E23_CC2_IO0           = {2, 5};
    const PIN E21_CC1_IO1           = {2, 6};
    const PIN D21_CC1_IO0           = {2, 7};

    const PIN F20_ENA_RS2           = {2, 8};//Stuck High in Libero
    const PIN D8_C1_I2C_SCL         = {2, 9};
    const PIN C7_C2_I2C_SCL         = {2, 10};
    const PIN F8_C3_I2C_SCL         = {2, 11};

    const PIN B6_C4_I2C_SCL         = {2, 12};
    const PIN A12_IO_WDI_DATA       = {2, 13};
    const PIN GPIO2_OUT_0           = {2, 14};

    const PIN J18_P_2V5_IMON        = {2, 15};

    const PIN H17_P_1V8_IMON        = {2, 16};
    const PIN H16_P_1V0_PGOOD       = {2, 17};
    const PIN G17_P_1V8_PGOOD       = {2, 18};
    const PIN G16_P_1V0_IMON        = {2, 19};

    const PIN G15_P_3V3_PGOOD       = {2, 20};
    const PIN A13_IO_nWDO           = {2, 21};
    const PIN V2_PGOOD_C1           = {2, 22};
    const PIN T4_PGOOD_C2           = {2, 23};

    const PIN K18_P_2V5_PGOOD       = {2, 24};
    const PIN H14_MDIO              = {2, 25};
    const PIN B21_CC2_IO3           = {2, 26};
    const PIN C22_CC2_IO2           = {2, 27};

    const PIN D24_CC1_IO3           = {2, 28};
    const PIN D25_CC1_IO2           = {2, 29};
    const PIN R3_PGOOD_C4           = {2, 30};
    const PIN P1_PGOOD_C3           = {2, 31};
#else
       const PIN J16_CFG2               ={0, 16};
       const PIN R2_ETH_MDINT           ={0, 17};
       const PIN F15_CTRL_PGOOD0        ={0, 18};
       const PIN G15_CTRL_nIFLT0        ={0, 19};
       const PIN N9_ETH_PG              ={0, 20};
       const PIN J18_CTRL_PGOOD1        ={0, 21};
       const PIN H18_CTRL_nIFLT1        ={0, 22};
       const PIN H17_CTRL_PGOOD2        ={0, 23};
       const PIN G17_CTRL_nIFLT2        ={0, 24};
       const PIN F17_CTRL_PGOOD3        ={0, 25};
       const PIN F18_CTRL_nIFLT3        ={0, 26};
       const PIN F23_IO_nPFO            ={0, 27};
       const PIN A4_IS_IsNominal0       ={0, 28};
       const PIN A3_IS_IsNominal1       ={0, 29};
       const PIN F5_IS_IsNominal2       ={0, 30};
       const PIN E13_CTRL_PGOOD4        ={0, 31};
       const PIN H16_SVTT_ENA           ={0, 0};
       const PIN E16_FVTT_ENA           ={0, 1};
       const PIN L5_ETH_COMA_MODE       ={0, 2};
       const PIN M9_ETH_nReset          ={0, 3};
       const PIN N14_ENA_CLK_50M        ={0, 4};
       const PIN C3_NAND_PWR_2_ENA      ={0, 5};
       const PIN D5_B_SNOR_nRESET       ={0, 6};
       const PIN D13_CTRL_ENABLE        ={0, 7};
       const PIN A13_CTRL_SOC_P_OFF     ={0, 8};
       const PIN C14_MI1_GPIO2          ={0, 9};
       const PIN MI1_I2C_SDA_CTRL       ={0, 10};
       const PIN A14_MI1_I2C_SCL        ={0, 11};
       const PIN B14_CAMS_PWR_C1_ENA    ={0, 12};
       const PIN D15_MI2_GPIO2          ={0, 13};
       const PIN MI2_I2C_SDA_CTRL       ={0, 14};
       const PIN B15_CAM2_PWR_ENA       ={0, 15};
       const PIN F14_CTRL_nIFLT4        ={1, 16};
       const PIN B12_CTRL_nIFLT5        ={1, 17};
       const PIN C12_CTRL_PGOOD6        ={1, 18};
       const PIN C13_CTRL_PGOOD7        ={1, 19};
       const PIN A12_MI1_GPIO1          ={1, 20};
       const PIN MI1_I2C_SDA_DIN        ={1, 21};
       const PIN E18_MI2_GPIO1          ={1, 22};
       const PIN MI2_I2C_SDA_DIN        ={1, 23};
       const PIN A15_MI2_I2C_SCL        ={1, 24};
       const PIN D21_IO_nWDO            ={1, 25};
       const PIN eMMC_WR_FIFORFM        ={1, 26};
       const PIN F20_COMMS_GPIO1        ={1, 27};
       const PIN eMMC_RD_FIFO_DAVAIL    ={1, 28};
       const PIN K15_F_SD_CD            ={1, 29};
       const PIN eMMC_INTERRUPT         ={1, 30};
       const PIN K18_CFG1               ={1, 31};
       const PIN D18_CAMS_PWR_TEL_ENA   ={1, 0};
       const PIN A19_COMMS_GPIO2        ={1, 1};
       const PIN A18_PWR_ETH_ENA        ={1, 2};
       const PIN J15_F_SD_ENA           ={1, 3};
       const PIN D19_SOC_MON_OUT1       ={1, 4};
       const PIN B20_SOC_MON_OUT2       ={1, 5};
       const PIN E21_IO_MR              ={1, 6};
       const PIN D25_IO_WDI             ={1, 7};
       const PIN A30_NAND_PWR_1_ENA     ={1, 8};

       const PIN PIN_LED1               =D19_SOC_MON_OUT1;
       const PIN PIN_LED2               =B20_SOC_MON_OUT2;

#endif



static void init_gpio(int instance, uint32_t address, uint32_t out_data)
{
    GPIOS[instance].address  = address;
    GPIOS[instance].out_data = out_data;
    // HW_set_32bit_reg(address+WDATA, out_data);
    scai_set_reg(address+WDATA, out_data);
}


static uint32_t get_init_outs_gpio0(uint32_t *init)
{
#if defined(NAVC_BOARD)
    init[0]     = 0;    //SDAx like input;
#else
    init[0]     = N14_ENA_CLK_50M_MASK;
#endif
    return 0;
}

static uint32_t get_init_outs_gpio1(uint32_t* init)
{
#if defined(NAVC_BOARD)
    init[0]     = C21_ENA_SS1_MASK; //Storage 1 enabled
#else
    init[0]     = LED1_MASK | E21_IO_MR_MASK;
#endif
    return 0;
}

#if defined(NAVC_BOARD)
uint32_t get_init_outs_gpio2(uint32_t *init)
{

    init[0]     = A23_ENA_SS2_MASK  | F20_ENA_RS2_MASK; //Only Storage and RS422 Nominal enabled
    return 0;
}
#endif

void init_gpios(void)
{
    uint32_t mask_d=0;

    get_init_outs_gpio0(&mask_d);
    init_gpio(GPIO0, GPIOs0_BASE_ADDRESS, mask_d);
    get_init_outs_gpio1(&mask_d);
    init_gpio(GPIO1, GPIOs1_BASE_ADDRESS, mask_d);
#if defined(NAVC_BOARD)
    get_init_outs_gpio2(&mask_d);
    init_gpio(GPIO2, GPIOs2_BASE_ADDRESS, mask_d);
#endif
    #if defined IS_MSS
    init_mss_gpios();
#endif
}


void set_gpio(PIN bit, int value)
{
    uint32_t address, data, mask;

    address = GPIOS[bit.instance].address;
    mask    = ((uint32_t)1)<<bit.mask;
    data    = scai_get_reg(address+RDATA);
    if(!value)
        data &= ~mask;
    else
        data |= mask;
    // HW_set_32bit_reg(address+WDATA, data);
    scai_set_reg(address+WDATA, data);
}

int toggle_gpio(PIN bit)
{
    uint32_t address, data, mask;

    address = GPIOS[bit.instance].address;
    mask = ((uint32_t)1)<<bit.mask;
    data = scai_get_reg(address+RDATA);

    if(data & mask)
        data &= ~mask;
    else
        data |= mask;

    // HW_set_32bit_reg(address+WDATA, data);
    scai_set_reg(address+WDATA, data);
    return (data&mask)?1:0;
}

int pulse_gpio(PIN bit)
{
    uint32_t address, data, mask;

    address = GPIOS[bit.instance].address;
    mask = ((uint32_t)1)<<bit.mask;
    data = scai_get_reg(address+RDATA);
    data |= mask;
    // HW_set_32bit_reg(address+WDATA, data);
    scai_set_reg(address+WDATA, data);

    data &= ~mask;
    // HW_set_32bit_reg(address+WDATA, data);
    scai_set_reg(address+WDATA, data);

    return (data&mask)?1:0;
}

int get_gpio(PIN bit)
{
    uint32_t data, mask;

    data = scai_get_reg(GPIOS[bit.instance].address+RDATA);
    mask = 1<<bit.mask;

    return (data&mask)?1:0;
}

int get_gpio_direction(PIN bit)
{
    uint32_t data, mask;

    data = scai_get_reg(GPIOS[bit.instance].address+RMASK);
    mask = 1<<bit.mask;

    return (data&mask)?1:0;
}

int get_full_data_gpio(PIN bit, int *d, int *pin, int *z)
{
    uint32_t data[3], mask;


    mask = 1<<bit.mask;
    data[0] = scai_get_reg(GPIOS[bit.instance].address+RDATA);
    d[0] = (data[0]&mask)?1:0;
    data[1] = scai_get_reg(GPIOS[bit.instance].address+RPINES);
    pin[0] = (data[1]&mask)?1:0;
    data[2] = scai_get_reg(GPIOS[bit.instance].address+RMASK);
    z[0] = (data[2]&mask)?1:0;
    return 1;
}


void set_gpos(int instance, uint32_t mask_bit, uint32_t values)
{
    uint32_t address, data;

    address = GPIOS[instance].address;

    data = ~mask_bit & scai_get_reg(address+RDATA); //data has in 0 the data to be update
    data |= (mask_bit & values);   //1 when the bit is not masked and the value is 1
                                    //0 Otherwise
    // HW_set_32bit_reg(address+WDATA, data);
    scai_set_reg(address+WDATA, data);
}

// static char printf_buffer[256];

// uint16_t gpio_printf(int instance, const char *format, ...)
// {
//     va_list args;
//     char *ptr = printf_buffer;
//     uint32_t j;
// 
//     // init arguments
//     va_start(args, format);
// 
//     vsprintf(printf_buffer, format, args);
// 
//     va_end(args);
//     while(*ptr)
//     {
//         j=*ptr++;
//         set_gpos(instance, 0x000000FF, j);
//     }
//     return 1;
// }

void test_gpios(void)
{
    static int outs[3]={0xBEBECAFEU, 0x01234567U, 0x89ABCDEF};


    set_gpos(0, scai_get_reg(GPIOs0_BASE_ADDRESS+RMASK), outs[0]);
    set_gpos(1, scai_get_reg(GPIOs1_BASE_ADDRESS+RMASK), outs[1]);
    set_gpos(2, scai_get_reg(GPIOs2_BASE_ADDRESS+RMASK), outs[2]);
    outs[0]+=(17*0x10000);
    outs[1]+=(19);
    outs[2]+=(23*0x100);
}

void set_led1(uint8_t value)
{
#if defined(NAVC_BOARD)
    set_gpio(C26_LED1, value);
#else
    set_gpio(PIN_LED1, value);
#endif
}

void set_led2(uint8_t value)
{
#if defined(NAVC_BOARD)
    set_gpio(D26_LED2, value);
#else
    set_gpio(PIN_LED2, value);
#endif
}


void set_gpio_direction(PIN bit, int value)
{
    uint32_t address, data, mask;

    address = GPIOS[bit.instance].address;
    mask    = ((uint32_t)1)<<bit.mask;
    data    = scai_get_reg(address+RMASK);
    if(!value)
        data &= ~mask;
    else
        data |= mask;
    // HW_set_32bit_reg(address+WMASK, data);
    scai_set_reg(address+WMASK, data);
}

// static void RdWrGPIOS(void)
// {
//     uint32_t volatile din, dout;
//     uint32_t i, j=0;
// 
//     for(i=0, dout = 0x12345678;i<0x1000000;i++, dout++)
//     {
//         // HW_set_32bit_reg(GPIOs0_BASE_ADDRESS+WDATA, dout);
//         scai_set_reg(GPIOs0_BASE_ADDRESS+WDATA, dout);
//         din =  scai_get_reg(GPIOs0_BASE_ADDRESS+RDATA);
//         if(dout != din)
//             j++;
//         if(!(i&0xFFF))
//             mHSS_DEBUG_PRINTF(LOG_ERROR, ".\n");
//     }
//     if(j)
// //        uart_printf(U_DBG1,"\r\nDetected %d errors", j);
//         mHSS_DEBUG_PRINTF(LOG_ERROR, "Detected %d errors\n", j);
//     else
//         mHSS_DEBUG_PRINTF(LOG_ERROR, "No Error Detected\n");
// }

