/*
 * adcs.h
 *
 *  Created on: 1 May 2025
 *      Author: Sergio.Sirota
 */

#ifndef IOB_ADCS_H_
#define IOB_ADCS_H_

#include "ctest/sapi_hw_platform.h"
#include "hss_types.h"

struct ADC_st
{
    uint32_t address;
    uint32_t ctrl, old_ctrl;
    uint16_t tel[16];
    uint32_t min_t, max_t;
};

#define SaveCtrl(instance)       ADCs[instance].old_ctrl = ADCs[instance].ctrl
#define RestoreCtrl(instance)    ADCs[instance].ctrl = ADCs[instance].old_ctrl

typedef struct ADC_st ADCs_type;


typedef enum
{
    ADC_ENABLE      = 0x00000001,
    ADC_START       = 0x00000002,
    ADC_DIVISOR     = 0x000003FC,
    ADC_G_DOUT      = 0x0000FC00,
    ADC_G_ENA       = 0x003F0000,
    ADC_G_USE_GPIO  = 0x00400000,
    ADC_G_USE_TOG   = 0x00800000,
    ADC_ONE_SHOT    = 0x01000000
}ADCS_MASKS_ENUM;


/*alias ENABLE        : std_logic is control(0);
alias START_OP      : std_logic is control(1);
alias DIVISOR       : std_logic_vector(7 DownTo 0) is control(9 DownTo 2);
alias G_D_OUT       : std_logic_vector(5 DownTo 0)  is control(15 DownTo 10);
alias G_D_ENABLE    : std_logic_vector(5 DownTo 0)  is control(21 DownTo 16);
alias G_USE_GPIO    : std_logic                     is control(22);
alias G_USE_TOGGLE  : std_logic                     is control(23);
alias ST_ONE_SHOT   : std_logic                     is control(24);
*/

typedef enum
{
    ADC_S_ENABLE    = 0,
    ADC_S_START     = 1,
    ADC_S_DIVISOR   = 2,
    ADC_S_G_DOUT    = 10,
    ADC_S_G_ENA     = 16,
    ADC_S_G_USE_GPIO= 22,
    ADC_S_G_USE_TOG = 23,
    ADC_S_ONE_SHOT  = 24
}ADCS_SHIFTS_ENUM;

typedef enum
{
    ADC_GET_TEL0    = 0,
    ADC_GET_TEL1    = 4,
    ADC_GET_TEL2    = 8,
    ADC_GET_TEL3    = 12,
    ADC_GET_TEL4    = 16,
    ADC_GET_TEL5    = 20,
    ADC_GET_TEL6    = 24,
    ADC_GET_TEL7    = 28,
    ADC_GET_STATUS  = 32,
    ADC_GET_TEL10   = 64+0,
    ADC_GET_TEL11   = 64+4,
    ADC_GET_TEL12   = 64+8,
    ADC_GET_TEL13   = 64+12,
    ADC_GET_TEL14   = 64+16,
    ADC_GET_TEL15   = 64+20,
    ADC_GET_TEL16   = 64+24,
    ADC_GET_TEL17   = 64+28
}ADCS_REGS;

typedef enum
{
    ADC_ST_IDLE        = 0x00000001,
    /*  status      <=  x"00"
                    & E_CE & E_CLK & "0" & E_MOSI & E_MISO  & E_CE
                    & "00" & G_D_OUT
                    & ADC_CE & ADC_CLK & "0" & ADC_MOSI & ADC_MISO  & ADC_CE;
    */
}ADCS_ST_ENUM;

#define ADC_IDLE(address) ((ADC_ST_IDLE & scai_get_reg(address + ADC_GET_STATUS))?1:0)

extern ADCs_type ADCs[2];

void init_adcs(void);

/*
 * This routine will trigger only one ADC reading writing the control register.
 * Return how many data where read before the trigger
 * */
int one_shot_adc(ADCs_type *atr);

/*
 * This routine will trigger the constant mode for the ADC: read all time while have space in the hFIFO
 * */
void start_adc(ADCs_type *atr);


/*
 * This routine will disable the interface. If it was enabled, first depletes the hFIFO.
 * Return how many data where read before the trigger
 * */
int disable_adc(ADCs_type *atr);


/*
 * This routine will enablethe interface, if was disabled.
 * */
void enable_adc(ADCs_type *atr);

/*
 * This routine stop the telemetry reading, if it was running. First depletes the hFIFO, and waits up to interface is idle
 * Return how many data where read before the trigger
 * */
int stop_adc(ADCs_type *atr);

void test_adcs(void);

int get_adc_telemetry(uint32_t *d);

int VTTs_are_ok(void);

#endif /* IOB_ADCS_H_ */
