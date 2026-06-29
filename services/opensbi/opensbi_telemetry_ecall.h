#ifndef OPENSBI_TELEMETRY_ECALL_H
#define OPENSBI_TELEMETRY_ECALL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "opensbi_ecall.h"


int sbi_ecall_telemetry_handler(unsigned long extid, unsigned long funcid,
			     const struct sbi_trap_regs *regs,
			     unsigned long *out_val,
			     struct sbi_trap_info *out_trap);


#ifdef __cplusplus
}
#endif

#endif
