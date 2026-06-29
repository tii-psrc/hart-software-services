#include "config.h"
#include "hss_types.h"
#include "opensbi_service.h"

#if !IS_ENABLED(CONFIG_OPENSBI)
#  error OPENSBI needed for this module
#endif

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
#include <sbi/sbi_trap.h>
#include <sbi/sbi_version.h>
#include <sbi/riscv_asm.h>
#include <sbi/riscv_atomic.h>
#include <sbi/riscv_barrier.h>
#include "mss_peripherals.h"

#include "opensbi_ecall.h"
#include "opensbi_telemetry_ecall.h"

#include "uart_helper.h"
#include "telemetry_service.h"

int sbi_ecall_telemetry_handler(unsigned long extid,
			     unsigned long funcid,
			     const struct sbi_trap_regs *regs,
			     unsigned long *out_val,
			     struct sbi_trap_info *out_trap)
{
  int result = SBI_EFAIL;
	volatile uint8_t *__reversed_ddr_addr = (volatile uint8_t *)regs->a0;
	int status = 0;
	char buf[1024];

	status = set_request_from_sbi_ecall(__reversed_ddr_addr);
	while (status) {
		status = is_request_from_sbi_ecall();
	}
	*out_val = strlen((char *)__reversed_ddr_addr) + 1;

#if 1
	memset(buf, 0, sizeof(buf));
	format_log(HSS_HART_E51, buf, "\r\nvvv %s(%d) vvv\r\n", __func__, *out_val); 
	format_log(HSS_HART_E51, (char *)__reversed_ddr_addr , NULL);
	format_log(HSS_HART_E51, buf, "^^^ %s ^^^\r\n\r\n", __func__);
#endif

	result = SBI_OK;

	return result;
}

