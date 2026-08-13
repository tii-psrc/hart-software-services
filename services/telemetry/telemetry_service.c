#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdbool.h>

#include "telemetry_service.h"
#include "adcs.h"
#if defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
#include "telemetry_publish.h"
#endif

#include "hss_state_machine.h"
#include "hss_clock.h"
#include "uart_helper.h"

static HSSTicks_t tm_ticks = 0;
static bool request_from_cli = false;
static bool request_from_sbi_ecall = false;

static volatile uint8_t *__sbi_ecall_reserved_ddr_buf = NULL;
static uint32_t __sbi_ecall_verbose = 0;

#if defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
static int stop_publish = 0;
static uint32_t publish_count_timer = 0;
static uint32_t publish_count_console = 0;
#endif

#if defined(CONFIG_SERVICE_WDOG_ENABLE_EXTERNAL)
#include "wdog_external.h"
#endif

enum telemetry_status {
	TM_INITIALIZATION,
	TM_MONITORING,
	TM_NUM_STATES = TM_MONITORING+1
};

static void tm_init_handler(struct StateMachine * const pMyMachine);
static void tm_monitoring_handler(struct StateMachine * const pMyMachine);
void format_thermistor(int hartid, char *buf, const char *format, uint32_t d1, uint32_t d2);

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

#if defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
int tm_set_stop_publish(void)
{
	stop_publish = 1;
	publish_count_timer = 0;
	publish_count_console = 0;

	return stop_publish;
}
#endif

bool is_request_from_cli(void)
{
	return request_from_cli;
}

bool set_request_from_cli(void)
{
	request_from_cli = true;

	return request_from_cli;
}

bool clear_request_from_cli(void)
{
	request_from_cli = false;

	return request_from_cli;
}

bool is_request_from_sbi_ecall(void)
{
	return request_from_sbi_ecall;
}

bool set_request_from_sbi_ecall(volatile uint8_t *sbi_buf_addr, uint32_t sbi_verbose)
{
	request_from_sbi_ecall = true;
	__sbi_ecall_reserved_ddr_buf = sbi_buf_addr;
	__sbi_ecall_verbose = sbi_verbose;

	return request_from_sbi_ecall;
}

bool clear_request_from_sbi_ecall(void)
{
	request_from_sbi_ecall = false;
	__sbi_ecall_reserved_ddr_buf = NULL;
	__sbi_ecall_verbose = 0;

	return request_from_sbi_ecall;
}

static void tm_init_handler(struct StateMachine * const pMyMachine)
{
	init_adcs();
#if defined(CONFIG_BOARD_SCAI_DPU460) && defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
	stop_publish = 0;
	publish_count_timer = 0;
	publish_count_console = 0;
	/*
	 * MMUART Controller #0 : LVDS1
	 * MMUART Controller #1 : U54_1 Hart(U-Boot/Linux)
	 * MMUART Controller #2 : E51 Hart(HSS Bootloader)
	 * MMUART Controller #3 : LVDS2
	 */
	do_tm_publish_init(HSS_UART_GetInstance(HSS_HART_U54_2)); // MMUART Controller #0
	do_tm_publish_init(HSS_UART_GetInstance(HSS_HART_U54_3)); // MMUART Controller #3
#endif

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

static void format_tel_line(int hartid, char *buf, const char *format, uint32_t d1, uint32_t d2)
{
	char ptr1[16], ptr2[16], ptr3[16];
	uint32_t power;

	power = (d1/100)*(d2/100)/100;
	format_telemetry(d1, ptr1);
	format_telemetry(d2, ptr2);
	format_telemetry(power, ptr3);
	format_log(hartid, buf, format, ptr1, ptr2, ptr3);
}

static void format_tel_line1(int hartid, char *buf, const char *format, uint32_t d1)
{
	char ptr1[16];

	format_telemetry(d1, ptr1);
	format_log(hartid, buf, format, ptr1);
}

#if defined(CONFIG_BOARD_SCAI_DPU460)
void format_thermistor(int hartid, char *buf, const char *format, uint32_t d1, uint32_t d2)
{
	char ptr1[16], ptr2[16];
	format_telemetry(d1, ptr1);
	format_telemetry(d2, ptr2);
	format_log(hartid, buf, format, ptr1, ptr2);
}
#endif

static void tm_monitoring_print(int hartid, char *buf, struct telemetry_data *tm_data)
{
	uint32_t adc[24];
#if defined(CONFIG_BOARD_SCAI_DPU460)
	uint16_t pf_tel[4];
	int16_t  in_celsius;
#endif

	format_log(hartid, buf, "Telemetry\r\n");

#if defined(CONFIG_BOARD_SCAI_DPU460)
	pf_init(pf_tel);
	in_celsius = (pf_tel[3]>>4) - 273;
#endif

	get_adc_telemetry(hartid, buf, adc);
	format_log(hartid, buf, "\r\n");

#if defined(CONFIG_BOARD_SCAI_DPU460)
	format_log(hartid, buf, "SOC Temp.       => Temp    : %d.%dK %dC\r\n", pf_tel[3]>>4, pf_tel[3]& 0xF, in_celsius);
	if (tm_data) {
		tm_data->temp_K = (pf_tel[3]>>4);
		tm_data->temp_C = in_celsius;
	}
#endif
	format_tel_line (hartid, buf, "Input voltage     => Voltage : %sV  - Current: %sA - Power: %s\r\n",
			adc[6], adc[14]);
	if (tm_data) {
		tm_data->input_mV = adc[6]/1000;
		tm_data->input_mA = adc[14]/1000;
	}
	format_tel_line (hartid, buf, "1.0V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n",
			adc[1], adc[0]);
	if (tm_data) {
		tm_data->rail_1_0_mV = adc[1]/1000;
		tm_data->rail_1_0_mA = adc[0]/1000;
	}
#if defined(CONFIG_BOARD_SCAI_DPU460)
	format_log(hartid, buf, "1.0V rail from PF => Voltage : %d.%03dV\r\n", (pf_tel[0]>>3)/1000, (pf_tel[0]>>3)%1000);
	if (tm_data) {
		tm_data->rail_1_0_from_pf_mV = (pf_tel[0]>>3);
	}
#endif

	format_tel_line (hartid, buf, "1.2V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n",
			adc[3], adc[2]);
	if (tm_data) {
		tm_data->rail_1_2_mV = adc[3]/1000;
		tm_data->rail_1_2_mA = adc[2]/1000;
	}

	format_tel_line (hartid, buf, "1.8V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n",
			adc[5], adc[4]);
	if (tm_data) {
		tm_data->rail_1_8_mV = adc[5]/1000;
		tm_data->rail_1_8_mA = adc[4]/1000;
	}
#if defined(CONFIG_BOARD_SCAI_DPU460)
	format_log(hartid, buf, "1.8V rail from PF => Voltage : %d.%03dV\r\n", (pf_tel[1]>>3)/1000, (pf_tel[1]>>3)%1000);
	if (tm_data) {
		tm_data->rail_1_8_from_pf_mV = (pf_tel[1]>>3);
	}
#endif

	format_tel_line (hartid, buf, "2.5V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n",
			adc[9], adc[8]);
	if (tm_data) {
		tm_data->rail_2_5_mV = adc[9]/1000;
		tm_data->rail_2_5_mA = adc[8]/1000;
	}
#if defined(CONFIG_BOARD_SCAI_DPU460)
	format_log(hartid, buf, "2.5V rail from PF => Voltage : %d.%03dV\r\n", (pf_tel[2]>>3)/1000, (pf_tel[2]>>3)%1000);
	if (tm_data) {
		tm_data->rail_2_5_from_pf_mV = (pf_tel[2]>>3);
	}
#endif

	format_tel_line (hartid, buf, "3.3V rail         => Voltage : %sV  - Current: %sA - Power: %s\r\n",
			adc[11], adc[10]);
	if (tm_data) {
		tm_data->rail_3_3_mV = adc[11]/1000;
		tm_data->rail_3_3_mA = adc[10]/1000;
	}
	format_tel_line1(hartid, buf, "System DDR VTT    => Voltage : %sV\r\n", adc[13]);
	format_tel_line1(hartid, buf, "Fabric DDR VTT    => Voltage : %sV\r\n", adc[12]);
	if (tm_data) {
		tm_data->sddr_mV = adc[13]/1000;
		tm_data->fddr_mV = adc[12]/1000;
	}

	format_tel_line1(hartid, buf, "Reference Voltage => ADC1    : %sV\r\n", adc[7]);
	format_tel_line1(hartid, buf, "Reference Voltage => ADC2    : %sV\r\n", adc[15]);
	if (tm_data) {
		tm_data->adc1_mV = adc[7]/1000;
		tm_data->adc2_mV = adc[15]/1000;
	}

#if defined(CONFIG_BOARD_SCAI_DPU460)
	format_thermistor (hartid, buf, "Camera1 Thermistor=>From V+ : %sOhm - From V-: %sOhm \r\n",
			adc[16], adc[17]);
	format_thermistor (hartid, buf, "Camera2 Thermistor=>From V+ : %sOhm - From V-: %sOhm \r\n",
			adc[18], adc[19]);
	if (tm_data) {
		tm_data->camera1_thermistor_mOhm_plus = adc[16]/1000;
		tm_data->camera1_thermistor_mOhm_minus = adc[17]/1000;
		tm_data->camera2_thermistor_mOhm_plus = adc[16]/1000;
		tm_data->camera2_thermistor_mOhm_minus = adc[17]/1000;
	}

	format_tel_line1(hartid, buf, "Voltage Camera1   => Voltage : %sV\r\n", adc[22]);
	format_tel_line1(hartid, buf, "Voltage Camera2   => Voltage : %sV\r\n", adc[21]);
	format_tel_line1(hartid, buf, "Voltage Cams Telem=> Voltage : %sV\r\n", adc[23]);
	format_tel_line1(hartid, buf, "Reference Voltage => ADC_TEL : %sV\r\n", adc[20]);
	if (tm_data) {
		tm_data->camera1_mV = adc[22]/1000;
		tm_data->camera2_mV = adc[21]/1000;
		tm_data->cams_telem_mV = adc[23]/1000;
		tm_data->adc_tel_mV = adc[20]/1000;
	}

	format_log(hartid, buf, "\r\nSanity check (Digital Status Signals):");
	do_format_sanity(hartid, buf, tm_data);
	do_format_sanity_vtt(hartid, buf, tm_data);
#endif

	format_log(hartid, buf, "\r\n");
}

static void print_tm_data(int hartid, char *buf, struct telemetry_data *tm_data)
{
	format_log(hartid, buf, "tm_data->temp_K                        : %d\r\n",
			tm_data->temp_K);
	format_log(hartid, buf, "tm_data->temp_C                        : %d\r\n",
			tm_data->temp_C);
	format_log(hartid, buf, "tm_data->input_mV                      : %d\r\n",
			tm_data->input_mV);
	format_log(hartid, buf, "tm_data->input_mA                      : %d\r\n",
			tm_data->input_mA);
	format_log(hartid, buf, "tm_data->rail_1_0_mV                   : %d\r\n",
			tm_data->rail_1_0_mV);
	format_log(hartid, buf, "tm_data->rail_1_0_mA                   : %d\r\n",
			tm_data->rail_1_0_mA);
	format_log(hartid, buf, "tm_data->rail_1_0_from_pf_mV           : %d\r\n",
			tm_data->rail_1_0_from_pf_mV);
	format_log(hartid, buf, "tm_data->rail_1_2_mV                   : %d\r\n",
			tm_data->rail_1_2_mV);
	format_log(hartid, buf, "tm_data->rail_1_2_mA                   : %d\r\n",
			tm_data->rail_1_2_mA);
	format_log(hartid, buf, "tm_data->rail_1_8_mV                   : %d\r\n",
			tm_data->rail_1_8_mV);
	format_log(hartid, buf, "tm_data->rail_1_8_mA                   : %d\r\n",
			tm_data->rail_1_8_mA);
	format_log(hartid, buf, "tm_data->rail_1_8_from_pf_mV           : %d\r\n",
			tm_data->rail_1_8_from_pf_mV);
	format_log(hartid, buf, "tm_data->rail_2_5_mV                   : %d\r\n",
			tm_data->rail_2_5_mV);
	format_log(hartid, buf, "tm_data->rail_2_5_mA                   : %d\r\n",
			tm_data->rail_2_5_mA);
	format_log(hartid, buf, "tm_data->rail_2_5_from_pf_mV           : %d\r\n",
			tm_data->rail_2_5_from_pf_mV);
	format_log(hartid, buf, "tm_data->rail_3_3_mV                   : %d\r\n",
			tm_data->rail_3_3_mV);
	format_log(hartid, buf, "tm_data->rail_3_3_mA                   : %d\r\n",
			tm_data->rail_3_3_mA);
	format_log(hartid, buf, "tm_data->sddr_mV                       : %d\r\n",
			tm_data->sddr_mV);
	format_log(hartid, buf, "tm_data->fddr_mV                       : %d\r\n",
			tm_data->fddr_mV);
	format_log(hartid, buf, "tm_data->adc1_mV                       : %d\r\n",
			tm_data->adc1_mV);
	format_log(hartid, buf, "tm_data->adc2_mV                       : %d\r\n",
			tm_data->adc2_mV);
	format_log(hartid, buf, "tm_data->camera1_thermistor_mOhm_plus  : %d\r\n",
			tm_data->camera1_thermistor_mOhm_plus);
	format_log(hartid, buf, "tm_data->camera1_thermistor_mOhm_minus : %d\r\n",
			tm_data->camera1_thermistor_mOhm_minus);
	format_log(hartid, buf, "tm_data->camera2_thermistor_mOhm_plus  : %d\r\n",
			tm_data->camera2_thermistor_mOhm_plus);
	format_log(hartid, buf, "tm_data->camera2_thermistor_mOhm_minus : %d\r\n",
			tm_data->camera2_thermistor_mOhm_minus);
	format_log(hartid, buf, "tm_data->camera1_mV                    : %d\r\n",
			tm_data->camera1_mV);
	format_log(hartid, buf, "tm_data->camera2_mV                    : %d\r\n",
			tm_data->camera2_mV);
	format_log(hartid, buf, "tm_data->cams_telem_mV                 : %d\r\n",
			tm_data->cams_telem_mV);
	format_log(hartid, buf, "tm_data->adc_tel_mV                    : %d\r\n",
			tm_data->adc_tel_mV);
	format_log(hartid, buf, "tm_data->sanity_check_1_0_mV           : %d\r\n",
			tm_data->sanity_check_1_0_mV);
	format_log(hartid, buf, "tm_data->sanity_check_1_0_mA           : %d\r\n",
			tm_data->sanity_check_1_0_mA);
	format_log(hartid, buf, "tm_data->sanity_check_1_2_mV           : %d\r\n",
			tm_data->sanity_check_1_2_mV);
	format_log(hartid, buf, "tm_data->sanity_check_1_2_mA           : %d\r\n",
			tm_data->sanity_check_1_2_mA);
	format_log(hartid, buf, "tm_data->sanity_check_1_8_mV           : %d\r\n",
			tm_data->sanity_check_1_8_mV);
	format_log(hartid, buf, "tm_data->sanity_check_1_8_mA           : %d\r\n",
			tm_data->sanity_check_1_8_mA);
	format_log(hartid, buf, "tm_data->sanity_check_2_5_mV           : %d\r\n",
			tm_data->sanity_check_2_5_mV);
	format_log(hartid, buf, "tm_data->sanity_check_2_5_mA           : %d\r\n",
			tm_data->sanity_check_2_5_mA);
	format_log(hartid, buf, "tm_data->sanity_check_3_3_mV           : %d\r\n",
			tm_data->sanity_check_3_3_mV);
	format_log(hartid, buf, "tm_data->sanity_check_3_3_mA           : %d\r\n",
			tm_data->sanity_check_3_3_mA);
	format_log(hartid, buf, "tm_data->sanity_check_sddr_vtt         : %d\r\n",
			tm_data->sanity_check_sddr_vtt);
	format_log(hartid, buf, "tm_data->sanity_check_fddr_vtt         : %d\r\n",
			tm_data->sanity_check_fddr_vtt);
}

static void tm_monitoring_handler(struct StateMachine * const pMyMachine)
{
	(void)pMyMachine;

	char buf[4096];
	struct telemetry_data tm_data;

	HSSTicks_t ticks = HSS_GetTime();
	size_t msecs = ((ticks - tm_ticks) + (TICKS_PER_MILLISEC / 2)) / TICKS_PER_MILLISEC;

	if (CONFIG_SERVICE_TELEMETRY_DEBUG_TIMEOUT_SEC && msecs >
			CONFIG_SERVICE_TELEMETRY_DEBUG_TIMEOUT_SEC*1000)
	{
		mHSS_DEBUG_PRINTF(LOG_NORMAL, "[%s] Timer execution.\r\n", __func__);
		memset(buf, 0, sizeof(buf));
		tm_monitoring_print(HSS_HART_E51, buf, NULL);

#if defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
#if 0
		if (!stop_publish)
			do_tm_publish((uint8_t)255);
#endif
#endif

		tm_ticks = HSS_GetTime();
	}

	if (is_request_from_cli()) {
		mHSS_DEBUG_PRINTF(LOG_NORMAL, "[%s] CLI execution.\r\n", __func__);
		memset(buf, 0, sizeof(buf));
		memset((void *)&tm_data, 0, sizeof(struct telemetry_data));

		tm_monitoring_print(HSS_HART_E51, buf, &tm_data);
		print_tm_data(HSS_HART_E51, buf, &tm_data);

#if defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
		if (!stop_publish)
			do_tm_publish((uint8_t)255);
#endif

		clear_request_from_cli();
	}

	if (is_request_from_sbi_ecall()) {
		mHSS_DEBUG_PRINTF(LOG_NORMAL, "[%s] SBI ECALL execution.\r\n", __func__);
		memset(buf, 0, sizeof(buf));
		memset((void *)&tm_data, 0, sizeof(struct telemetry_data));

		if (__sbi_ecall_verbose) {
			tm_monitoring_print(HSS_HART_ALL + 1, buf, NULL);
			memcpy((void *)__sbi_ecall_reserved_ddr_buf, (void *)buf, strlen(buf) + 1);
		} else {
			tm_monitoring_print(HSS_HART_E51, buf, &tm_data);
			print_tm_data(HSS_HART_E51, buf, &tm_data);
			memcpy((void *)__sbi_ecall_reserved_ddr_buf, (void *)&tm_data, sizeof(struct telemetry_data));
		}
		clear_request_from_sbi_ecall();
	}
}
