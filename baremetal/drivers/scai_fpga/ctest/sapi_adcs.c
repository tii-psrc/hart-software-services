/*
 * adcs.c
 *
 *  Created on: 1 May 2025
 *      Author: Sergio.Sirota
 */

#include "ctest/sapi_adcs.h"
#include "ctest/sapi_gpios.h"
#include "ctest/sapi_hw_platform.h"
#include "ctest/reg_mitm.h"

#include <string.h>

#include "hss_types.h"

#define NAVC_BOARD 1

ADCs_type ADCs[2];

void init_adcs(void)
{
    ADCs_type *atr;

    atr = &ADCs[0];
    atr->address = ADCs_BASE_ADDRESS;
    //The input clock of the module is 100Mhz. The valid clock range is 3.2MHz (~Clk/30) to 8MHz (~Clk/12.5). We will go with 5MHz (Clk/20)
    //By default: neither GPIO nor Toggling enabled
    atr->ctrl = ADC_ENABLE | ADC_START | (20)<<(ADC_S_DIVISOR);
    SaveCtrl(0);
    // HW_set_32bit_reg(atr->address, atr->ctrl);
    scai_gpio_set_reg(atr->address, atr->ctrl);
#if defined(DPU_BOARD)

    set_gpio(D18_CAMS_PWR_TEL_ENA, 1);
    atr = &ADCs[1];
    atr->address = ADCs1_BASE_ADDRESS;
    //The input clock of the module is 100Mhz. The valid clock range is 3.2MHz (~Clk/30) to 8MHz (~Clk/12.5). We will go with 5MHz (Clk/20)
    //By default: neither GPIO nor Toggling enabled
    atr->ctrl = ADC_ENABLE | ADC_START | (100)<<(ADC_S_DIVISOR);
    SaveCtrl(1);
    // HW_set_32bit_reg(atr->address, atr->ctrl);
    scai_gpio_set_reg(atr->address, atr->ctrl);
#endif
}

static int get_hData(int instance)
{
    ADCs_type *atr;
    atr = &ADCs[instance];

    atr->tel[ 0] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL0);
    atr->tel[ 1] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL1);
    atr->tel[ 2] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL2);
    atr->tel[ 3] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL3);
    atr->tel[ 4] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL4);
    atr->tel[ 5] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL5);
    atr->tel[ 6] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL6);
    atr->tel[ 7] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL7);
    atr->tel[ 8] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL10);
    atr->tel[ 9] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL11);
    atr->tel[10] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL12);
    atr->tel[11] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL13);
    atr->tel[12] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL14);
    atr->tel[13] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL15);
    atr->tel[14] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL16);
    atr->tel[15] = 0xFFF & scai_gpio_get_reg(atr->address + ADC_GET_TEL17);

    return 1;
}


/*
 * This routine will trigger an ADC0 reading writing the control register. If some data is in the FIFO, must be read before,
 * */
int one_shot_adc(ADCs_type *atr)
{
    int i = 0;

    //Wait until the interface is idle
    while(!ADC_IDLE(atr->address))
        ;
    atr->ctrl |= ADC_ONE_SHOT;
    // HW_set_32bit_reg(atr->address, atr->ctrl);
    scai_gpio_set_reg(atr->address, atr->ctrl);

    return i;
}

void start_adc(ADCs_type *atr)
{
    atr->ctrl |= ADC_START;
    // HW_set_32bit_reg(atr->address, atr->ctrl);
    scai_gpio_set_reg(atr->address, atr->ctrl);
}

int disable_adc(ADCs_type *atr)
{
    int i=0;

    if(atr->ctrl & ADC_ENABLE)
    {
        //Wait until the interface is idle
        while(!ADC_IDLE(atr->address))
            ;
        atr->ctrl &= ~ADC_ENABLE;
        // HW_set_32bit_reg(atr->address, atr->ctrl);
        scai_gpio_set_reg(atr->address, atr->ctrl);
    }
    return i;
}

void enable_adc(ADCs_type *atr)
{
    if(!(atr->ctrl & ADC_ENABLE))
    {
        atr->ctrl |= ADC_ENABLE;
        // HW_set_32bit_reg(atr->address, atr->ctrl);
        scai_gpio_set_reg(atr->address, atr->ctrl);
    }
}

int stop_adc(ADCs_type *atr)
{
    int i=0;

    if(atr->ctrl & ADC_START)
    {
        //Wait until the interface is idle
        while(!ADC_IDLE(atr->address))
            ;
        atr->ctrl &= ~ADC_START;
        // HW_set_32bit_reg(atr->address, atr->ctrl);
        scai_gpio_set_reg(atr->address, atr->ctrl);
    }
    return i;
}

//Here I will put the high level functions, to get the real telemetry

static uint16_t get_all_adc_data(int instance, uint16_t *data)
{
    int i, max_i=16;
    uint16_t ret = 1;
    ADCs_type *atr;

    atr = &ADCs[instance];
    get_hData(instance);
    if(instance)
        max_i = 8;
    for(i=0; i<max_i; i++)
        data[i] = atr->tel[i];
    return ret;
}

static void get_adc0_telemetry(uint16_t *adc, uint32_t *d)
{
    uint64_t aux, vcc, aux1;

    aux1 = ((uint64_t)2500000)*((uint64_t)4095);

    vcc = aux1/((int64_t)adc[7]);   //I get a better value for VCC (3.3V, in microvolts) based on 2.5V voltage reference

    aux1 = 4095;
    //Channel 0: P_1V0.IMON in micro Amperes


    aux = ((int64_t)adc[0])*vcc/aux1; //10mOhm sensor resistance and gain x100
    d[0] = (int32_t)aux;
    //Channel 1: P_1V0.Vout in micro Volts
    aux = ((int64_t)adc[1])*vcc/aux1;
    d[1] = (int32_t)aux;
    //Channel 2: P_1V2.IMON in micro Amperes
    aux = ((int64_t)adc[2])*vcc/aux1;
    d[2] = (int32_t)aux;
    //Channel 3: P_1V2.Vout in micro Volts
    aux = ((int64_t)adc[3])*vcc/aux1;
    d[3] = (int32_t)aux;
    //Channel 4: P_1V8.IMON in micro Amperes
    aux = ((int64_t)adc[4])*vcc/aux1;
    d[4] = (int32_t)aux;
    //Channel 5: P_1V8.Vout in micro Volts
    aux = ((int64_t)adc[5])*vcc/aux1;
    d[5] = (int32_t)aux;
    //Channel 6: PWR__5V.5V0 in micro Volts
    aux = ((int64_t)adc[6])*vcc*2/aux1;
    d[6] = (int32_t)aux;
    //Channel 7: Vref in micro Volts
    aux = ((int64_t)adc[7])*vcc/aux1;
    d[7] = (int32_t)aux;
}

static void get_adc1_telemetry(uint16_t *adc, uint32_t *d)
{
    uint64_t aux, vcc, aux1;

    aux1 = ((uint64_t)2500000)*((uint64_t)4095);

    vcc = aux1/((int64_t)adc[7]);   //I get a better value for VCC (3.3V, in microvolts) based on 2.5V voltage reference

    aux1 = 4095;
    //Channel 0: P_2V5.IMON in micro Amperes
    aux = ((int64_t)adc[0])*vcc/aux1;
    d[0] = (int32_t)aux;
    //Channel 1: P_2V5.Vout in micro Volts
    aux = ((int64_t)adc[1])*vcc/aux1;
    d[1] = (int32_t)aux;
    //Channel 2: P_3V3.IMON in micro Amperes
    aux = ((int64_t)adc[2])*vcc/aux1;
    d[2] = (int32_t)aux;
    //Channel 3: P_3V3.Vout in micro Volts
    aux = ((int64_t)adc[3])*vcc*2/aux1;
    d[3] = (int32_t)aux;
    //Channel 4: FVTT.VTT in micro Volts
    aux = ((int64_t)adc[4])*vcc/aux1;
    d[4] = (int32_t)aux;
    //Channel 5: SVTT.VTT in micro Volts
    aux = ((int64_t)adc[5])*vcc/aux1;
    d[5] = (int32_t)aux;
    //Channel 6: PWR_5V.IMON in micro Volts
    aux = ((int64_t)adc[6])*vcc/aux1;



    aux *= 960648; //to_uA * relation Iout/Imon in TPS7H2201 / Current monitor resistor in DPU = 41500/43200.Multiply by 1e6 to improve the precision
    aux /= 1000000; //

    d[6] = (int32_t)aux;
    //Channel 7: Vref in micro Volts
    aux = ((int64_t)adc[7])*vcc/aux1;
    d[7] = (int32_t)aux;
}

#if defined(DPU_BOARD)

static uint32_t R2T(uint64_t R)
{
    uint64_t aux;


    aux = ((uint64_t)1000*R-(uint64_t)1000000)/(uint64_t)385;

    return (uint32_t)aux;
}
void get_adc2_telemetry(uint16_t *adc, uint32_t *d)
{
    uint64_t aux2, N;
    int ret;

    N = (uint64_t)adc[4];
    aux2 = (uint64_t)2500000/N;    //Vref/Nref

    //Camera2, Thermistor+ : Res = 10(4095-2*N)/(N-4095) = 10N/(4095-N) - 10
    N = (uint64_t)adc[0];
    d[0] = R2T((10*N/(4095-N))-10);

    //Camera2, Thermistor- : Res = (40950/N)-20
    N = (uint64_t)adc[1];
    d[1] = R2T(((uint64_t)40950/N)-20);

    //Camera1, Thermistor+ : Res = 10(4095-2*N)/(N-4095) = 10N/(4095-N) - 10
    N = (uint64_t)adc[2];
    d[2] = R2T((10*N/(4095-N))-10);

    //Camera1, Thermistor- : Res = (40950/N)-20
    N = (uint64_t)adc[3];
    d[3] = R2T(((uint64_t)40950/N)-20);

    N = 2;
    //Camera 2 Voltage
    d[5] = N*((uint64_t)adc[5])*aux2;

    //Camera 1 Voltage
    d[6] = N*((uint64_t)adc[6])*aux2;

    //Vtel Voltage
    d[7] = N*((uint64_t)adc[7])*aux2;

    //Vref
    d[4] = 2500000;

}
#else
static void get_adc2_telemetry(uint16_t *adc, uint32_t *d)
{

}
#endif


int get_adc_telemetry(uint32_t *d)
{
    uint16_t adc[24];

    get_all_adc_data(0, adc);
    get_all_adc_data(1, &adc[16]);
    get_adc0_telemetry(&adc[0], &d[0]);
    get_adc1_telemetry(&adc[8], &d[8]);
    get_adc2_telemetry(&adc[16], &d[16]);
    return 1;
}


int VTTs_are_ok(void)
{
    int i;
    uint16_t ret = 0;
    ADCs_type *atr;

    atr = &ADCs[0];
#if defined(DPU_BOARD)
    set_gpio(H16_SVTT_ENA, 1);
    set_gpio(E16_FVTT_ENA, 1);
#endif
    for(i=0; i<1000; i++)
    {
        if(get_hData(0))
        {
            if((atr->tel[12]>680) && (atr->tel[13]>680))
            {
                ret  = 1;
                break;
            }
        }
    }
    return ret;
}

