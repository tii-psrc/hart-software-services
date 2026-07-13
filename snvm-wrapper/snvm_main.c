#include "mpfs_hal/mss_hal.h"
#include "mpfs_hal/common/nwc/mss_nwc_init.h"
#include "mpfs_hal/common/mss_peripherals.h"

#include "snvm_config.h"
#include "snvm_log.h"
#include "snvm_utils.h"
#include "snvm_cli.h"
#include "snvm_uart.h"

typedef enum SNVM_WFI_SM_
{
  SNVM_INIT_THREAD_PR = 0x00, /* !< 0 init pointer    */
  SNVM_CHECK_WFI      = 0x01, /* !< is hart in wfi?   */
  SNVM_SEND_WFI       = 0x02, /* !< separate state to
                                 add a little delay */
  SNVM_CHECK_WAKE     = 0x03, /* !< has hart left wfi */
} SNVM_WFI_SM;


struct  hss_envm_manifest {
  uint32_t size;
  uint32_t crc32;
  uint8_t reserved[504];
};

extern const struct hss_envm_manifest __hss_envm_manifest_start;
extern const uint8_t __envm_start;

typedef void (*envm_entry_t)(void);
envm_entry_t entry = (envm_entry_t)&__envm_start;

void snvm_e51(HLS_DATA* hls);
void snvm_e51(HLS_DATA* hls)
{
  uint8_t * volatile p_hss_envm_manifest_addr = (uint8_t *)&__hss_envm_manifest_start;
  struct hss_envm_manifest * volatile p_hss_envm_manifest = (struct hss_envm_manifest *)p_hss_envm_manifest_addr;
  uint8_t * volatile buf = (uint8_t *)&__envm_start;
  uint32_t crc32_result = 0;

  snvm_cli(7);

  snvm_printf("[%s] p_hss_envm_manifest->size  : 0x%08X\r\n", __func__, p_hss_envm_manifest->size);
  snvm_printf("[%s] p_hss_envm_manifest->crc32 : 0x%08X\r\n", __func__, p_hss_envm_manifest->crc32);
  crc32_result = snvm_crc32(0, buf, p_hss_envm_manifest->size);
  snvm_printf("[%s] crc32_result               : 0x%08X\r\n", __func__, crc32_result);
#ifdef SNVM_DEBUG
  snvm_hexdump("eNVM", buf, p_hss_envm_manifest->size);
#endif

  if (p_hss_envm_manifest->crc32 == crc32_result) {
    snvm_printf("[%s] Integrity check passed ...\r\n", __func__);
    /* Clear pending software interrupt in case there was any.
     * Enable only the software interrupt so that the E51 core can bring this core
     * out of WFI by raising a software interrupt. */
    clear_soft_interrupt();
    set_csr(mie, MIP_MSIP);

    snvm_printf("[%s] Waking-up U54_1 hart, and then enter WFI mode ...\r\n", __func__);
    /* Raise software interrupt to wake hart 1 */
    raise_soft_interrupt(1U);

    /* put this hart into WFI. */
    do
    {
      __asm("wfi");
    } while(0 == (read_csr(mip) & MIP_MSIP));
    snvm_printf("[%s] Wakeup from WFI ...\r\n", __func__);

    /* The hart is out of WFI, clear the SW interrupt. Here onwards Application
     * can enable and use any interrupts as required */
    clear_soft_interrupt();

    snvm_printf("[%s] Jump to eNVM ...\r\n", __func__);
    entry();
  }

  /* CRC for eNVM failed ... */
  snvm_printf("[%s] Integrity check failed ...\r\n", __func__);
  snvm_printf("[%s] Enter sNVM CLI mode ...\r\n", __func__);
  snvm_cli(-1);
}

void snvm_u54_1(HLS_DATA* hls);
void snvm_u54_1(HLS_DATA* hls)
{
  snvm_printf_hart(SNVM_HSS_HART_U54_1,
      "[%s] Enter WFI mode ...\r\n", __func__);
  /* Clear pending software interrupt in case there was any.
     Enable only the software interrupt so that the E51 core can bring this
     core out of WFI by raising a software interrupt. */
  clear_soft_interrupt();
  set_csr(mie, MIP_MSIP);

  /*Put this hart into WFI.*/
  do
  {
    __asm("wfi");
  }while(0 == (read_csr(mip) & MIP_MSIP));
  /* crc32 for envm passed @e51 */
  snvm_printf_hart(SNVM_HSS_HART_U54_1,
      "[%s] Wakeup from WFI ...\r\n", __func__);
  snvm_printf_hart(SNVM_HSS_HART_U54_1,
      "[%s] Waking-up U54_2 hart ...\r\n", __func__);

  /* The hart is out of WFI, clear the SW interrupt. Hear onwards Application
   * can enable and use any interrupts as required */
  clear_soft_interrupt();

  /* Raise software interrupt to wake hart 2 */
  raise_soft_interrupt(2U);

  snvm_printf_hart(SNVM_HSS_HART_U54_1, "[%s] Jump to eNVM ...\r\n", __func__);
  entry();
}

void snvm_u54_2(HLS_DATA* hls);
void snvm_u54_2(HLS_DATA* hls)
{
  snvm_printf_hart(SNVM_HSS_HART_U54_2,
      "[%s] Enter WFI mode ...\r\n", __func__);
  /* Clear pending software interrupt in case there was any.
   * Enable only the software interrupt so that the E51 core can bring this core
   * out of WFI by raising a software interrupt. */
  clear_soft_interrupt();
  set_csr(mie, MIP_MSIP);

  /* put this hart into WFI. */
  do
  {
    __asm("wfi");
  } while(0 == (read_csr(mip) & MIP_MSIP));
  /* crc32 for envm passed @e51 */
  snvm_printf_hart(SNVM_HSS_HART_U54_2,
      "[%s] Wakeup from WFI ...\r\n", __func__);
  snvm_printf_hart(SNVM_HSS_HART_U54_2,
      "[%s] Waking-up U54_3 hart ...\r\n", __func__);

  /* The hart is out of WFI, clear the SW interrupt. Here onwards Application
   * can enable and use any interrupts as required */
  clear_soft_interrupt();

  /* Raise software interrupt to wake hart 3 */
  raise_soft_interrupt(3U);

  snvm_printf_hart(SNVM_HSS_HART_U54_2, "[%s] Jump to eNVM ...\r\n", __func__);
  entry();
}

void snvm_u54_3(HLS_DATA* hls);
void snvm_u54_3(HLS_DATA* hls)
{
  snvm_printf_hart(SNVM_HSS_HART_U54_3,
      "[%s] Enter WFI mode ...\r\n", __func__);
  /* Clear pending software interrupt in case there was any.
   * Enable only the software interrupt so that the E51 core can bring this core
   * out of WFI by raising a software interrupt. */
  clear_soft_interrupt();
  set_csr(mie, MIP_MSIP);

  /* put this hart into WFI. */
  do
  {
    __asm("wfi");
  } while(0 == (read_csr(mip) & MIP_MSIP));
  /* crc32 for envm passed @e51 */
  snvm_printf_hart(SNVM_HSS_HART_U54_3,
      "[%s] Wakeup from WFI ...\r\n", __func__);
  snvm_printf_hart(SNVM_HSS_HART_U54_3,
      "[%s] Waking-up U54_4 hart ...\r\n", __func__);

  /* The hart is out of WFI, clear the SW interrupt. Here onwards Application
   * can enable and use any interrupts as required */
  clear_soft_interrupt();

  /* Raise software interrupt to wake hart 4 */
  raise_soft_interrupt(4U);

  snvm_printf_hart(SNVM_HSS_HART_U54_3, "[%s] Jump to eNVM ...\r\n", __func__);
  entry();
}

void snvm_u54_4(HLS_DATA* hls);
void snvm_u54_4(HLS_DATA* hls)
{
  snvm_printf_hart(SNVM_HSS_HART_U54_4,
      "[%s] Enter WFI mode ...\r\n", __func__);
  /* Clear pending software interrupt in case there was any.
   * Enable only the software interrupt so that the E51 core can bring this core
   * out of WFI by raising a software interrupt. */
  clear_soft_interrupt();
  set_csr(mie, MIP_MSIP);

  /* put this hart into WFI. */
  do
  {
    __asm("wfi");
  } while(0 == (read_csr(mip) & MIP_MSIP));
  /* crc32 for envm passed @e51 */
  snvm_printf_hart(SNVM_HSS_HART_U54_4,
      "[%s] Wakeup from WFI ...\r\n", __func__);
  snvm_printf_hart(SNVM_HSS_HART_U54_4,
      "[%s] Waking-up E51 hart ...\r\n", __func__);

  /* The hart is out of WFI, clear the SW interrupt. Here onwards Application
   * can enable and use any interrupts as required */
  clear_soft_interrupt();

  /* Raise software interrupt to wake hart 0 */
  raise_soft_interrupt(0U);

  snvm_printf_hart(SNVM_HSS_HART_U54_4, "[%s] Jump to eNVM ...\r\n", __func__);
  entry();
}

__attribute__((noinline)) int snvm_other_main(HLS_DATA* hls);
__attribute__((noinline)) int snvm_other_main(HLS_DATA* hls)
{
  extern char __app_stack_top_h0;
  extern char __app_stack_top_h1;
  extern char __app_stack_top_h2;
  extern char __app_stack_top_h3;
  extern char __app_stack_top_h4;

  const uint64_t app_stack_top_h0 = (const uint64_t)&__app_stack_top_h0 - (SNVM_HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h1 = (const uint64_t)&__app_stack_top_h1 - (SNVM_HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h2 = (const uint64_t)&__app_stack_top_h2 - (SNVM_HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h3 = (const uint64_t)&__app_stack_top_h3 - (SNVM_HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h4 = (const uint64_t)&__app_stack_top_h4 - (SNVM_HLS_DEBUG_AREA_SIZE);

  const uint64_t app_hart_common_start = (const uint64_t)&__app_hart_common_start;
  hls->shared_mem = (uint64_t *)app_hart_common_start;
  hls->shared_mem_marker = SNVM_SHARED_MEM_INITALISED_MARKER;
  hls->shared_mem_status = SNVM_SHARED_MEM_DEFAULT_STATUS;

  volatile uint64_t dummy;

  switch(hls->my_hart_id)
  {

    case 0U:
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h0));
      snvm_e51(hls);
      break;

    case 1U:
      //(void)init_pmp((uint8_t)1);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h1));
      snvm_u54_1(hls);
      break;

    case 2U:
      //(void)init_pmp((uint8_t)2);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h2));
      snvm_u54_2(hls);
      break;

    case 3U:
      //(void)init_pmp((uint8_t)3);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h3));
      snvm_u54_3(hls);
      break;

    case 4U:
      //(void)init_pmp((uint8_t)4);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h4));
      snvm_u54_4(hls);
      break;

    default:
      /* no more harts */
      break;
  }

  return 0;
}

__attribute__((noinline)) int snvm_main(HLS_DATA* hls_e51);
__attribute__((noinline)) int snvm_main(HLS_DATA* hls_e51)
{
  volatile uint32_t hartid = read_csr(mhartid);

  volatile uint32_t u54_hart_id;
  uint8_t * volatile stack_top = 0U;

  volatile uint32_t wait_count = 0U;
  HLS_DATA * volatile hls_u54 = NULL;


  if (hartid == SNVM_MPFS_HAL_FIRST_HART) {
    (void)mss_nwc_init();
    (void)snvm_uart_init();

    snvm_printf("[hart #%d] hsl(0x%08lx)\r\n",
        hartid, (unsigned long)(uintptr_t)hls_e51);

    hls_e51->my_hart_id = hartid;
    hls_e51->in_wfi_indicator = SNVM_HLS_MAIN_HART_STARTED;

    SNVM_WFI_SM sm_check_thread = SNVM_INIT_THREAD_PR;
    u54_hart_id = SNVM_MPFS_HAL_FIRST_HART + 1U;
    while(u54_hart_id <= MPFS_HAL_LAST_HART)
    {
      switch (sm_check_thread)
      {
        default:
        case SNVM_INIT_THREAD_PR:

          switch (u54_hart_id)
          {
            case 1:
              stack_top = (uint8_t*)&__stack_top_h1$;
              break;
            case 2:
              stack_top = (uint8_t*)&__stack_top_h2$;
              break;
            case 3:
              stack_top = (uint8_t*)&__stack_top_h3$;
              break;
            case 4:
              stack_top = (uint8_t*)&__stack_top_h4$;
              break;
          }
          hls_u54 = (HLS_DATA*)(stack_top - SNVM_HLS_DEBUG_AREA_SIZE);
          snvm_printf("[hart #%d] hsl(0x%08lx)\r\n",
              u54_hart_id, (unsigned long)(uintptr_t)hls_u54);
          sm_check_thread = SNVM_CHECK_WFI;
          wait_count = 0U;
          break;

        case SNVM_CHECK_WFI:
          if (hls_u54->in_wfi_indicator == SNVM_HLS_OTHER_HART_IN_WFI)
          {
            /* Separate state- to add a little delay */
            sm_check_thread = SNVM_SEND_WFI;
          }
          break;

        case SNVM_SEND_WFI:
          hls_u54->my_hart_id = u54_hart_id; /* record hartid locally */
          raise_soft_interrupt(u54_hart_id);
          sm_check_thread = SNVM_CHECK_WAKE;
          wait_count = 0UL;
          break;

        case SNVM_CHECK_WAKE:
          if (hls_u54->in_wfi_indicator == SNVM_HLS_OTHER_HART_PASSED_WFI)
          {
            sm_check_thread = SNVM_INIT_THREAD_PR;
            u54_hart_id++;
            wait_count = 0UL;
          }
          else
          {
            wait_count++;
            if(wait_count > 0x10U)
            {
              if(hls_u54->in_wfi_indicator == SNVM_HLS_OTHER_HART_IN_WFI )
              {
                hls_u54->my_hart_id = u54_hart_id; /* record hartid locally */
                raise_soft_interrupt(u54_hart_id);
                wait_count = 0UL;
              }
            }
          }
          break;
      }
    }
    hls_e51->in_wfi_indicator = SNVM_HLS_MAIN_HART_FIN_INIT;
    (void)snvm_other_main(hls_e51);
  }

  return (0);
}
