#ifndef HSS_TELEMETRY_SERVICE_H
#define HSS_TELEMETRY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hss_state_machine.h"
#include "hss_debug.h"

extern struct StateMachine tm_service;

bool is_request_from_cli(void);
bool set_request_from_cli(void);
bool clear_request_from_cli(void);

bool is_request_from_sbi_ecall(void);
bool set_request_from_sbi_ecall(volatile uint8_t *sbi_buf_addr, uint32_t sbi_verbose);
bool clear_request_from_sbi_ecall(void);


struct telemetry_data {
  int32_t temp_K;
  int32_t temp_C;
  int32_t input_mV;
  int32_t input_mA;
  int32_t rail_1_0_mV;
  int32_t rail_1_0_mA;
  int32_t rail_1_0_from_pf_mV;
  int32_t rail_1_2_mV;
  int32_t rail_1_2_mA;
  int32_t rail_1_8_mV;
  int32_t rail_1_8_mA;
  int32_t rail_1_8_from_pf_mV;
  int32_t rail_2_5_mV;
  int32_t rail_2_5_mA;
  int32_t rail_2_5_from_pf_mV;
  int32_t rail_3_3_mV;
  int32_t rail_3_3_mA;
  int32_t sddr_mV;
  int32_t fddr_mV;
  int32_t adc1_mV;
  int32_t adc2_mV;
  int32_t camera1_thermistor_mOhm_plus;
  int32_t camera1_thermistor_mOhm_minus;
  int32_t camera2_thermistor_mOhm_plus;
  int32_t camera2_thermistor_mOhm_minus;
  int32_t camera1_mV;
  int32_t camera2_mV;
  int32_t cams_telem_mV;
  int32_t adc_tel_mV;
  int32_t sanity_check_1_0_mV;
  int32_t sanity_check_1_0_mA;
  int32_t sanity_check_1_2_mV;
  int32_t sanity_check_1_2_mA;
  int32_t sanity_check_1_8_mV;
  int32_t sanity_check_1_8_mA;
  int32_t sanity_check_2_5_mV;
  int32_t sanity_check_2_5_mA;
  int32_t sanity_check_3_3_mV;
  int32_t sanity_check_3_3_mA;
  int32_t sanity_check_sddr_vtt;
  int32_t sanity_check_fddr_vtt;
};

#ifdef __cplusplus
}
#endif

#endif
