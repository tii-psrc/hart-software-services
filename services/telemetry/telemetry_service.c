#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdbool.h>

#include "telemetry_service.h"
#include "adcs.h"

#include "hss_state_machine.h"
#include "hss_clock.h"
#include "uart_helper.h"

static HSSTicks_t tm_ticks = 0;

enum telemetry_status {
	TM_INITIALIZATION,
	TM_MONITORING,
	TM_NUM_STATES = TM_MONITORING+1
};

static void tm_init_handler(struct StateMachine * const pMyMachine);
static void tm_monitoring_handler(struct StateMachine * const pMyMachine);

static const struct StateDesc tm_state_descs[] = {
	{ (const stateType_t)TM_INITIALIZATION, (const char *)"init",       NULL, NULL, &tm_init_handler       },
	{ (const stateType_t)TM_MONITORING,     (const char *)"monitoring", NULL, NULL, &tm_monitoring_handler },
};

struct StateMachine tm_service = {
    .state             = (stateType_t)TM_INITIALIZATION,
    .prevState         = (stateType_t)SM_INVALID_STATE,
    .numStates         = (const uint32_t)TM_NUM_STATES,
    .pMachineName      = (const char *)"tm_service",
    .startTime         = 0u,
    .lastExecutionTime = 0u,
    .executionCount    = 0u,
    .pStateDescs       = tm_state_descs,
    .debugFlag         = true,
    .priority          = 0u,
    .pInstanceData     = NULL
};

static void tm_init_handler(struct StateMachine * const pMyMachine)
{
	init_adcs();

	tm_ticks = HSS_GetTime();

	pMyMachine->state++;
}

static void format_telemetry(uint32_t data, char *ptr)
{
	uint32_t i, j;

	i = data/1000000;
	j = (data - i*1000000)/1000;

	sbi_snprintf(ptr, sizeof(ptr), "%d.%03d", i, j);
}

static void format_tel_line(const char *format, uint32_t d1, uint32_t d2)
{
	char ptr1[16], ptr2[16], ptr3[16];
	uint32_t power;

	power = (d1/100)*(d2/100)/100;
	format_telemetry(d1, ptr1);
	format_telemetry(d2, ptr2);
	format_telemetry(power, ptr3);
	custom_uart_printf(HSS_HART_E51, format, ptr1, ptr2, ptr3);
}

static void format_tel_line1(const char *format, uint32_t d1)
{
	char ptr1[16];

	format_telemetry(d1, ptr1);
	custom_uart_printf(HSS_HART_E51, format, ptr1);
}

void tm_monitoring_print(void)
{
	uint32_t adc[24];

	custom_uart_printf(HSS_HART_E51, "\r\n");
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "\r\nTelemetry\r\n");

	get_adc_telemetry(adc);
	custom_uart_printf(HSS_HART_E51, "\r\n");

	format_tel_line ("Input voltage     => Voltage : %sV  - Current: %sA - Power: %s\r\n", adc[6], adc[14]);
	format_tel_line ("1.0V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n", adc[1], adc[0]);
	format_tel_line ("1.2V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n", adc[3], adc[2]);
	format_tel_line ("1.8V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n", adc[5], adc[4]);
	format_tel_line ("2.5V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n", adc[9], adc[8]);
	format_tel_line ("3.3V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n", adc[11], adc[10]);
	format_tel_line1("System DDR VTT    => Voltage : %sV\r\n", adc[13]);
	format_tel_line1("Fabric DDR VTT    => Voltage : %sV\r\n", adc[12]);

	format_tel_line1("Reference Voltage => ADC1    : %sV\r\n", adc[7]);
	format_tel_line1("Reference Voltage => ADC2    : %sV\r\n", adc[15]);

#if defined(DPU_BOARD)
	format_thermistor ("Camera1 Thermistor=>From V+ : %sOhm - From V-: %sOhm \r\n", adc[16], adc[17]);
	format_thermistor ("Camera2 Thermistor=>From V+ : %sOhm - From V-: %sOhm \r\n", adc[18], adc[19]);
	format_tel_line1("Voltage Camera1   => Voltage : %sV\r\n", adc[22]);
	format_tel_line1("Voltage Camera2   => Voltage : %sV\r\n", adc[21]);
	format_tel_line1("Voltage Cams Telem=> Voltage : %sV\r\n", adc[23]);
	format_tel_line1("Reference Voltage => ADC_TEL : %sV\r\n", adc[20]);

	custom_uart_printf(HSS_HART_E51, "\r\nSanity check (Digital Status Signals):");
	format_sanity("\r\n    1.0V:          => PGOOD   : %s - IMON: %s", F15_CTRL_PGOOD0, G15_CTRL_nIFLT0);
	format_sanity("\r\n    1.2V:          => PGOOD   : %s - IMON: %s", J18_CTRL_PGOOD1, H18_CTRL_nIFLT1);
	format_sanity("\r\n    1.8V:          => PGOOD   : %s - IMON: %s", E13_CTRL_PGOOD4, F14_CTRL_nIFLT4);
	format_sanity("\r\n    2.5V:          => PGOOD   : %s - IMON: %s", F17_CTRL_PGOOD3, F18_CTRL_nIFLT3);
	format_sanity("\r\n    3.3V:          => PGOOD   : %s - IMON: %s", H17_CTRL_PGOOD2, G17_CTRL_nIFLT2);
	format_sanity_vtt("Sytem DDR VTT", H16_SVTT_ENA, C12_CTRL_PGOOD6);
	format_sanity_vtt("Fabric DDR VTT", E16_FVTT_ENA, C13_CTRL_PGOOD7);
#endif

	custom_uart_printf(HSS_HART_E51, "\r\n");
}

static void tm_monitoring_handler(struct StateMachine * const pMyMachine)
{
	(void)pMyMachine;
	
	HSSTicks_t ticks = HSS_GetTime();
	size_t msecs = ((ticks - tm_ticks) + (TICKS_PER_MILLISEC / 2)) / TICKS_PER_MILLISEC;

	if (CONFIG_SERVICE_TELEMETRY_DEBUG_TIMEOUT_SEC && msecs > CONFIG_SERVICE_TELEMETRY_DEBUG_TIMEOUT_SEC*1000) {
		tm_monitoring_print();
		tm_ticks = HSS_GetTime();
	}
}
