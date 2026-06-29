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
bool set_request_from_sbi_ecall(volatile uint8_t *sbi_buf_addr);
bool clear_request_from_sbi_ecall(void);

#ifdef __cplusplus
}
#endif

#endif
