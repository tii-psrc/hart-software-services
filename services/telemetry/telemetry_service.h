#ifndef HSS_TELEMETRY_SERVICE_H
#define HSS_TELEMETRY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hss_state_machine.h"
#include "hss_debug.h"

extern struct StateMachine tm_service;

void tm_monitoring_print(int hartid, char *buf);

#ifdef __cplusplus
}
#endif

#endif
