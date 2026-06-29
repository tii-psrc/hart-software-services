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
	char buf[1024];
	volatile uintptr_t dma_addr = (uintptr_t)regs->a0;
	volatile uint8_t *__ddr_buf = (volatile uint8_t *)dma_addr;
	int32_t ipi_status = 0;

	memset(buf, 0, sizeof(buf));
  format_log(HSS_HART_E51, buf, "%s!!! addr(0x%p)\r\n", __func__, dma_addr);
	format_log(HSS_HART_E51, buf, "buf[0] : 0x%02X\r\n", __ddr_buf[0]);
	format_log(HSS_HART_E51, buf, "buf[1] : 0x%02X\r\n", __ddr_buf[1]);
	format_log(HSS_HART_E51, buf, "buf[2] : 0x%02X\r\n", __ddr_buf[2]);
	format_log(HSS_HART_E51, buf, "buf[3] : 0x%02X\r\n", __ddr_buf[3]);
	__ddr_buf[0] = 0x01;
	__ddr_buf[1] = 0x11;
	__ddr_buf[2] = 0x02;
	__ddr_buf[3] = 0x22;
	format_log(HSS_HART_E51, buf, "buf[0] : 0x%02X\r\n", __ddr_buf[0]);
	format_log(HSS_HART_E51, buf, "buf[1] : 0x%02X\r\n", __ddr_buf[1]);
	format_log(HSS_HART_E51, buf, "buf[2] : 0x%02X\r\n", __ddr_buf[2]);
	format_log(HSS_HART_E51, buf, "buf[3] : 0x%02X\r\n", __ddr_buf[3]);


	tm_dma_addr(dma_addr);
	ipi_status = increase_tm_ipi_status();
	do {
		ipi_status = get_tm_ipi_status();
	} while (ipi_status);

	*out_val = strlen((char *)dma_addr);

	format_log(HSS_HART_E51, buf, "\r\nvvv %s(%d) vvv\r\n", __func__, *out_val);
	format_log(HSS_HART_E51, (char *)dma_addr, NULL);
	format_log(HSS_HART_E51, buf, "^^^ %s ^^^\r\n\r\n", __func__);

	result = SBI_OK;
#if 0
#if 1
  //tm_monitoring_print();
  IPI_Send(HSS_HART_E51, IPI_MSG_TELEMETRY, IPI_DebugGetTxId(), PRV_M , NULL, NULL);
  //IPI_MessageUpdateStatus(transaction_id, IPI_IDLE); // free the IPI

  //int source = current_hartid();
  //struct IPI_Outbox_Msg *pMsg = IPI_DirectionToFirstMsgInQueue(source, HSS_HART_E51);
  //size_t i;
#else
  uint32_t index;
  if (IPI_MessageAlloc(&index)) {
    if(IPI_MessageDeliver(index, HSS_HART_E51, IPI_MSG_TELEMETRY, 0, NULL, NULL)) {
      result = SBI_OK;
    } else {
      IPI_MessageFree(index);
    }
  }
#endif
#endif

	return result;
}

