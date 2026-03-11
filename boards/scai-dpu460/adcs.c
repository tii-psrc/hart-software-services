/*
 * adcs.c
 *
 *  Created on: 1 May 2025
 *      Author: Sergio.Sirota
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "telemetry_service.h"
#include "adcs.h"

#include "hss_debug.h"
#include "hss_clock.h"

#include "uart_helper.h"

#define DPU_BOARD

#define APB_BASE_ADDRESS                0x40000000UL
#define ADCs_BASE_ADDRESS               (APB_BASE_ADDRESS+0x0600L)
#if defined(DPU_BOARD)
#define ADCs1_BASE_ADDRESS              (APB_BASE_ADDRESS+0x0300L)
#endif

#define GPIOs_BASE_ADDRESS              (APB_BASE_ADDRESS+0x0100L) // only for DPU
#define GPIOs0_BASE_ADDRESS             (GPIOs_BASE_ADDRESS)
#define GPIOs1_BASE_ADDRESS             (GPIOs_BASE_ADDRESS + 16)
#define GPIOs2_BASE_ADDRESS             (GPIOs_BASE_ADDRESS + 32)
#define GPIOs3_BASE_ADDRESS             (GPIOs_BASE_ADDRESS + 48)

#if defined(DPU_BOARD)
#define H16_SVTT_ENA 0          // GPIO0, BIT(0) 
#define E16_FVTT_ENA 1          // GPIO0, BIT(1)
#define D18_CAMS_PWR_TEL_ENA 0  // GPIO1, BIT(0)
#endif

#define BIT(n) (1UL << (n))

static void HW_set_32bit_reg(uintptr_t reg, uint32_t value) {
	*(volatile uint32_t*)reg = value;
}

static uint32_t HW_get_32bit_reg(uintptr_t reg) {
	uint32_t value = *(volatile uint32_t*)reg;
	return value;
}

#if defined(DPU_BOARD)
static int gpio_config(unsigned int gpio_num, unsigned int mask, unsigned int mode)
{
	volatile unsigned int *gpio_addr[4] = {
		(unsigned int *)GPIOs0_BASE_ADDRESS,
		(unsigned int *)GPIOs1_BASE_ADDRESS,
		(unsigned int *)GPIOs2_BASE_ADDRESS,
		(unsigned int *)GPIOs3_BASE_ADDRESS
	};
	unsigned int data = 0;

	data = *gpio_addr[gpio_num];
	//log_debug("[pre]\tdata : 0x%08X @0x%p\n", data, (void *)gpio_addr[gpio_num]);

	if (mode == 0) //clear bit
		data &= ~mask;
	else if (mode == 1) //enabled bit
		data |= mask;
	else if (mode == 2) //read bit
		return (data &= mask);
	else
		printf("%s: unknown request (%d)...\n", __func__, mode);

	*gpio_addr[gpio_num] = data;

	data = *gpio_addr[gpio_num];
	//log_debug("[post]\tdata : 0x%08X @0x%p\n", data, (void *)gpio_addr[gpio_num]);

	return 0;
}
#endif

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
	HW_set_32bit_reg(atr->address, atr->ctrl);
#if defined(DPU_BOARD)

	gpio_config(1, BIT(D18_CAMS_PWR_TEL_ENA), 1);
	atr = &ADCs[1];
	atr->address = ADCs1_BASE_ADDRESS;
	//The input clock of the module is 100Mhz. The valid clock range is 3.2MHz (~Clk/30) to 8MHz (~Clk/12.5). We will go with 5MHz (Clk/20)
	//By default: neither GPIO nor Toggling enabled
	atr->ctrl = ADC_ENABLE | ADC_START | (100)<<(ADC_S_DIVISOR);
	SaveCtrl(1);
	HW_set_32bit_reg(atr->address, atr->ctrl);
#endif
}

static int get_hData(int instance)
{
	ADCs_type *atr;
	atr = &ADCs[instance];

	atr->tel[0] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL0);
	atr->tel[1] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL1);
	atr->tel[2] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL2);
	atr->tel[3] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL3);
	atr->tel[4] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL4);
	atr->tel[5] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL5);
	atr->tel[6] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL6);
	atr->tel[7] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL7);
	atr->tel[8] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL10);
	atr->tel[9] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL11);
	atr->tel[10] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL12);
	atr->tel[11] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL13);
	atr->tel[12] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL14);
	atr->tel[13] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL15);
	atr->tel[14] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL16);
	atr->tel[15] = 0xFFF & HW_get_32bit_reg(atr->address + ADC_GET_TEL17);

	return 1;
}


/*
 * This routine will trigger an ADC0 reading writing the control register. If some data is in the FIFO, must be read before,
 * */
int one_shot_adc(ADCs_type *atr)
{
	//Wait until the interface is idle
	while(!ADC_IDLE(atr->address))
		;
	atr->ctrl |= ADC_ONE_SHOT;
	HW_set_32bit_reg(atr->address, atr->ctrl);
	return 0;
}

void start_adc(ADCs_type *atr)
{

	atr->ctrl |= ADC_START;
	HW_set_32bit_reg(atr->address, atr->ctrl);
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
		HW_set_32bit_reg(atr->address, atr->ctrl);
	}
	return i;
}

void enable_adc(ADCs_type *atr)
{
	if(!(atr->ctrl & ADC_ENABLE))
	{
		atr->ctrl |= ADC_ENABLE;
		HW_set_32bit_reg(atr->address, atr->ctrl);
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
		HW_set_32bit_reg(atr->address, atr->ctrl);
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
	uint64_t aux = 0;

	//Channel 0: P_1V0.IMON in micro Amperes
	custom_uart_printf(HSS_HART_E51, "\r\nTel 0 : %04d - %d", adc[0], aux);
	aux = 2500000ULL *(uint64_t)adc[0] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	d[0] = (int32_t)aux;

	//Channel 1: P_1V0.Vout in micro Volts
	aux = 2500000ULL *(uint64_t)adc[1] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 1 : %04d - %d", adc[1], aux);
	d[1] = (int32_t)aux;

	//Channel 2: P_1V2.IMON in micro Amperes
	aux = 2500000ULL *(uint64_t)adc[2] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 2 : %04d - %d", adc[2], aux);
	d[2] = (int32_t)aux;
	//Channel 3: P_1V2.Vout in micro Volts
	aux = 2500000ULL *(uint64_t)adc[3] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 3 : %04d - %d", adc[3], aux);
	d[3] = (int32_t)aux;
	//Channel 4: P_1V8.IMON in micro Amperes
	aux = 2500000ULL *(uint64_t)adc[4] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 4 : %04d - %d", adc[4], aux);
	d[4] = (int32_t)aux;
	//Channel 5: P_1V8.Vout in micro Volts
	aux = 2500000ULL *(uint64_t)adc[5] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 5 : %04d - %d", adc[5], aux);
	d[5] = (int32_t)aux;
	//Channel 6: PWR__5V.5V0 in micro Volts
	aux = 5000000ULL *(uint64_t)adc[6] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 6 : %04d - %d", adc[6], aux);
	d[6] = (int32_t)aux;
	custom_uart_printf(HSS_HART_E51, "\r\nTel 7 : %04d", adc[7]);
	//Channel 7: Vref in micro Volts
	d[7] = (int32_t)2500000;
}

static void get_adc1_telemetry(uint16_t *adc, uint32_t *d)
{
	uint64_t aux;

	//Channel 0: P_2V5.IMON in micro Amperes
	aux = 2500000ULL *(uint64_t)adc[0] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 10 : %04d - %d", adc[0], aux);
	d[0] = (int32_t)aux;

	//Channel 1: P_2V5.Vout in micro Volts
	aux = 2500000ULL *(uint64_t)adc[1] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 11 : %04d - %d", adc[1], aux);
	d[1] = (int32_t)aux;

	//Channel 2: P_3V3.IMON in micro Amperes
	aux = 2500000ULL *(uint64_t)adc[2] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 12 : %04d - %d", adc[2], aux);
	d[2] = (int32_t)aux;

	//Channel 3: P_3V3.Vout in micro Volts
	aux = 5000000ULL *(uint64_t)adc[3] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 13 : %04d - %d", adc[3], aux);
	d[3] = (int32_t)aux;

	//Channel 4: FVTT.VTT in micro Volts
	aux = 2500000ULL *(uint64_t)adc[4] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 14 : %04d - %d", adc[4], aux);
	d[4] = (int32_t)aux;

	//Channel 5: SVTT.VTT in micro Volts
	aux = 2500000ULL *(uint64_t)adc[5] / (uint64_t)adc[7]; //10mOhm sensor resistance and gain x100
	custom_uart_printf(HSS_HART_E51, "\r\nTel 15 : %04d - %d", adc[5], aux);
	d[5] = (int32_t)aux;

#if defined(DPU_BOARD)
	//Channel 6: PWR_5V.IMON in micro Amperte

	//Iout = ((2.5V * N6 * Gain) / (N7* Rmon))*1e6 = ((2.5*N6*41500)/(N7*43200))*1e6 = ((2.5*N6*415*)/(N7*432)) = 2401620*N6/N7 =

	aux = (2401620ULL * (uint64_t)adc[6]) / (uint64_t)adc[7];
	custom_uart_printf(HSS_HART_E51, "\r\nTel 16 : %04d - %d", adc[6], aux);
	d[6] = (int32_t)aux;
	custom_uart_printf(HSS_HART_E51, "\r\nTel 17 : %04d", adc[7]);
#else
	//Channel 6: PWR_5V.IMON in micro Amperte

	//Iout = ((2.5V * N6) / (N7*Rmon*G)) - (Ioff/G) = ((2.5*N6)/(N7*13000*52)) - (0.8/52) = ((369822485 * N6)/ N7 - 1538400)/100

#endif
	//Channel 7: Vref in micro Volts
	d[7] = (int32_t)2500000;
}

#if defined(DPU_BOARD)

static uint32_t R2T(uint64_t R)
{
	uint64_t aux;


	aux = ((uint64_t)1000*R-(uint64_t)1000000)/(uint64_t)385;

	return (uint32_t)aux;
}

static void get_adc2_telemetry(uint16_t *adc, uint32_t *d)
{
	uint64_t aux2, N;

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
	//ADCs_type *atr;
	int i;
	uint16_t ret = 0;

	//atr = &ADCs[0];
#if defined(DPU_BOARD)
	gpio_config(0, BIT(H16_SVTT_ENA), 1);
	HSS_SpinDelay_MilliSecs(20u);
	gpio_config(0, BIT(E16_FVTT_ENA), 1);
	HSS_SpinDelay_MilliSecs(20u);
#endif
	for(i=0; i<1000; i++)
	{
		if(get_hData(0))
		{
			{
				ret  = 1;
				break;
			}
		}
	}
	return ret;
}

