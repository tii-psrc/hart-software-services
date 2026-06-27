#ifndef HSS_TELEMETRY_SERVICE_H
#define HSS_TELEMETRY_SERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "hss_state_machine.h"
#include "ssmb_ipi.h"
#include "hss_debug.h"

extern struct StateMachine tm_service;

void tm_monitoring_print(int hartid, char *buf);

void tm_dma_addr(volatile uintptr_t dma_addr);
int32_t increase_tm_ipi_status(void);
int32_t get_tm_ipi_status(void);

enum IPIStatusCode HSS_Telemetry_IPIHandler(TxId_t transaction_id, enum HSSHartId source,
    uint32_t immediate_arg, void *p_extended_buffer_in_ddr, void *p_ancilliary_buffer_in_ddr);

#ifdef __cplusplus
}
#endif

#endif
