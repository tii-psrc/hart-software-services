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

#define TIMEOUT_COUNT 0x01000000
//#define TIMEOUT_COUNT 0x1

int sbi_ecall_telemetry_handler(unsigned long extid,
			     unsigned long funcid,
			     const struct sbi_trap_regs *regs,
			     unsigned long *out_val,
			     struct sbi_trap_info *out_trap)
{
  int result = SBI_EFAIL;
	volatile uint8_t *__reversed_ddr_addr = (volatile uint8_t *)regs->a0;
	bool status = false;
	char buf[1024];
	uint32_t wait_count = 0;

	HSSTicks_t start_time, end_time;
	uint64_t nsecs, msecs;

	start_time = HSS_GetTime();
	wait_count = 0;
	status = set_request_from_sbi_ecall(__reversed_ddr_addr);
	while (status && wait_count < TIMEOUT_COUNT) {
		wfi();
		status = is_request_from_sbi_ecall();
		++wait_count;
	}
	end_time = HSS_GetTime();

	if (status) {
		format_log(HSS_HART_E51, buf, "[%s] no response from telemetry service(%d) ...\r\n", __func__, wait_count);
		*out_val = 0;
		return result;
	}
	*out_val = strlen((char *)__reversed_ddr_addr) + 1;


#if 1
	memset(buf, 0, sizeof(buf));
	nsecs = ((end_time - start_time) + (TICKS_PER_MILLISEC / 2)) / (TICKS_PER_MILLISEC/1000llu);
	msecs = ((end_time - start_time)  + (TICKS_PER_MILLISEC/2)) / TICKS_PER_MILLISEC;

	format_log(HSS_HART_E51, buf, "[%s] time(%llu ns, %llu ms, %llu ticks),  wait_count(0x%08X)\r\n",
			__func__, nsecs, msecs, end_time - start_time, wait_count);
	format_log(HSS_HART_E51, buf, "[%s] str length(%d)\r\n", __func__, *out_val);
	format_log(HSS_HART_E51, (char *)__reversed_ddr_addr , NULL);
	format_log(HSS_HART_E51, buf, "[%s] done. \r\n", __func__);

#endif

	result = SBI_OK;

	return result;
}

