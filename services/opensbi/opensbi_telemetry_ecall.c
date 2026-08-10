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
#if IS_ENABLED(CONFIG_SERVICE_TELEMETRY)
#include "telemetry_service.h"
#endif

#if IS_ENABLED(CONFIG_SERVICE_WDOG_ENABLE_EXTERNAL)
#include "wdog_external.h"
#endif

enum sbi_tm_ext_cmd {
	SBI_TM_EXT_CONCISE = 0x0,
	SBI_TM_EXT_VERBOSE = 0x1,
	SBI_TM_EXT_STOP_PUBLISHING = 0x2,
};

static int __get_tm_data(uint32_t ext_args, volatile uint8_t *mem, unsigned long *out);
static int __get_tm_data(uint32_t ext_args, volatile uint8_t *mem, unsigned long *out)
{
  int result = SBI_EFAIL;

	bool status = true;
	char buf[1024];

	HSSTicks_t start_time, end_time;
	uint64_t usecs, msecs;

	start_time = HSS_GetTime();
#if IS_ENABLED(CONFIG_SERVICE_TELEMETRY)
	status = set_request_from_sbi_ecall(mem, ext_args);
	do {
		wfi();
		status = is_request_from_sbi_ecall();
		end_time = HSS_GetTime();
		if (end_time - start_time > TICKS_PER_SEC)
			break;
	} while (status);
#endif
	end_time = HSS_GetTime();

	if (status) {
		format_log(HSS_HART_E51, buf,
				"[%s] no response from telemetry service(%llu ticks) ...\r\n",
				__func__, end_time - start_time);
		*out = 0;
		return result;
	}

#if IS_ENABLED(CONFIG_SERVICE_TELEMETRY)
	if (ext_args == SBI_TM_EXT_VERBOSE) {
		*out = strlen((char *)mem) + 1;
	} else if (ext_args == SBI_TM_EXT_CONCISE) {
		*out = sizeof(struct telemetry_data);
	}
#endif

#if 1 // for debug
	memset(buf, 0, sizeof(buf));
	usecs = (end_time - start_time) / (TICKS_PER_MILLISEC/1000llu);
	msecs = (end_time - start_time) / TICKS_PER_MILLISEC;

	format_log(HSS_HART_E51, buf,
			"[%s] time(%llu us, %llu ms, %llu ticks)\r\n",
			__func__, usecs, msecs, end_time - start_time);
	format_log(HSS_HART_E51, buf, "[%s] str length(%d)\r\n", __func__, *out);
#if 0
	if (ext_args == SBI_TM_EXT_VERBOSE)
		format_log(HSS_HART_E51, (char *)mem , NULL);
#endif
	format_log(HSS_HART_E51, buf, "[%s] done. \r\n", __func__);
#endif
	result = SBI_OK;

	return result;
}

static int __stop_services(void);
static int __stop_services(void)
{
  int result = SBI_EFAIL;

#if IS_ENABLED(CONFIG_SERVICE_WDOG_ENABLE_EXTERNAL)
	if (wdog_external_stop())
		result = SBI_OK;
#endif

	return result;
}

int sbi_ecall_telemetry_handler(unsigned long extid,
			     unsigned long funcid,
			     const struct sbi_trap_regs *regs,
			     unsigned long *out_val,
			     struct sbi_trap_info *out_trap)
{
  int result = SBI_EFAIL;
	uint32_t sbi_tm_ext_args = (uint32_t)regs->a0;
	volatile uint8_t *__reversed_ddr_addr = (volatile uint8_t *)regs->a1;
	char buf[1024];

	format_log(HSS_HART_E51, buf, "[%s] args(%d)\r\n", __func__,
			sbi_tm_ext_args);
	switch (sbi_tm_ext_args) {
		case SBI_TM_EXT_CONCISE:
		case SBI_TM_EXT_VERBOSE:
			result = __get_tm_data(sbi_tm_ext_args, __reversed_ddr_addr, out_val);
			break;

		case SBI_TM_EXT_STOP_PUBLISHING:
			result = __stop_services();
			*out_val = 0;
			break;

		default:
			format_log(HSS_HART_E51, buf, "[%s] Unknown args(%d)\r\n", __func__,
					sbi_tm_ext_args);
			break;
	}

	return result;
}

