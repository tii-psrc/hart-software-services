#include "mpfs_hal/mss_hal.h"
#include "mpfs_hal/common/nwc/mss_nwc_init.h"

#include "snvm_log.h"

typedef enum SNVM_WFI_SM_
{
  SNVM_INIT_THREAD_PR = 0x00, /* !< 0 init pointer    */
  SNVM_CHECK_WFI      = 0x01, /* !< is hart in wfi?   */
  SNVM_SEND_WFI       = 0x02, /* !< separate state to
                              add a little delay */
  SNVM_CHECK_WAKE     = 0x03, /* !< has hart left wfi */
} SNVM_WFI_SM;

#define HLS_MAIN_HART_STARTED               0x12344321U
#define HLS_MAIN_HART_FIN_INIT              0x55555555U
#define HLS_OTHER_HART_IN_WFI               0x12345678U
#define HLS_OTHER_HART_PASSED_WFI           0x87654321U

#define SNVM_MPFS_HAL_FIRST_HART  0

void snvm_e51(void);
void snvm_e51(void)
{
	/* Raise software interrupt to wake hart 1 */
	raise_soft_interrupt(1U);

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

	/* The hart is out of WFI, clear the SW interrupt. Here onwards Application
	 * can enable and use any interrupts as required */
	clear_soft_interrupt();
	__enable_irq();

	for (;;)
	{
		static volatile uint64_t counter = 0U;
		/* Added some code as debugger hangs if in loop doing nothing */
		asm ("nop");
		asm ("nop");
		asm ("nop");
		asm ("nop");
		counter = counter + 1U;
		asm ("nop");
		asm ("nop");
		asm ("nop");

	}
	/* never return */
}

void snvm_u54_1(void);
void snvm_u54_1(void)
{
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

	/* The hart is out of WFI, clear the SW interrupt. Hear onwards Application
	 * can enable and use any interrupts as required */
	clear_soft_interrupt();
	__enable_irq();

	/* Raise software interrupt to wake hart 2 */
	raise_soft_interrupt(2U);

	for (;;)
	{
		static volatile uint64_t counter = 0U;
		/* Added some code as debugger hangs if in loop doing nothing */
		asm ("nop");
		asm ("nop");
		asm ("nop");
		asm ("nop");
		counter = counter + 1U;
		asm ("nop");
		asm ("nop");
		asm ("nop");

	}
	/* never return */
}

void snvm_u54_2(void);
void snvm_u54_2(void)
{
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

	/* The hart is out of WFI, clear the SW interrupt. Here onwards Application
	 * can enable and use any interrupts as required */
	clear_soft_interrupt();
	__enable_irq();

	/* Raise software interrupt to wake hart 3 */
	raise_soft_interrupt(3U);

	for (;;)
	{
		static volatile uint64_t counter = 0U;
		/* Added some code as debugger hangs if in loop doing nothing */
		asm ("nop");
		asm ("nop");
		asm ("nop");
		asm ("nop");
		counter = counter + 1U;
		asm ("nop");
		asm ("nop");
		asm ("nop");

	}
	/* never return */
}

void snvm_u54_3(void);
void snvm_u54_3(void)
{
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

	/* The hart is out of WFI, clear the SW interrupt. Here onwards Application
	 * can enable and use any interrupts as required */
	clear_soft_interrupt();
	__enable_irq();

	/* Raise software interrupt to wake hart 4 */
	raise_soft_interrupt(4U);

	for (;;)
	{
		static volatile uint64_t counter = 0U;
		/* Added some code as debugger hangs if in loop doing nothing */
		asm ("nop");
		asm ("nop");
		asm ("nop");
		asm ("nop");
		counter = counter + 1U;
		asm ("nop");
		asm ("nop");
		asm ("nop");

	}
	/* never return */
}

void snvm_u54_4(void);
void snvm_u54_4(void)
{
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

	/* The hart is out of WFI, clear the SW interrupt. Here onwards Application
	 * can enable and use any interrupts as required */
	clear_soft_interrupt();
	__enable_irq();

	/* Raise software interrupt to wake hart 4 */
	raise_soft_interrupt(0U);

	for (;;)
	{
		static volatile uint64_t counter = 0U;
		/* Added some code as debugger hangs if in loop doing nothing */
		asm ("nop");
		asm ("nop");
		asm ("nop");
		asm ("nop");
		counter = counter + 1U;
		asm ("nop");
		asm ("nop");
		asm ("nop");

	}
	/* never return */
}

__attribute__((noinline)) int snvm_other_main(HLS_DATA* hls);
__attribute__((noinline)) int snvm_other_main(HLS_DATA* hls)
{
  extern char __app_stack_top_h0;
  extern char __app_stack_top_h1;
  extern char __app_stack_top_h2;
  extern char __app_stack_top_h3;
  extern char __app_stack_top_h4;

  const uint64_t app_stack_top_h0 = (const uint64_t)&__app_stack_top_h0 - (HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h1 = (const uint64_t)&__app_stack_top_h1 - (HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h2 = (const uint64_t)&__app_stack_top_h2 - (HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h3 = (const uint64_t)&__app_stack_top_h3 - (HLS_DEBUG_AREA_SIZE);
  const uint64_t app_stack_top_h4 = (const uint64_t)&__app_stack_top_h4 - (HLS_DEBUG_AREA_SIZE);

  const uint64_t app_hart_common_start = (const uint64_t)&__app_hart_common_start;
  hls->shared_mem = (uint64_t *)app_hart_common_start;
  hls->shared_mem_marker = SHARED_MEM_INITALISED_MARKER;
  hls->shared_mem_status = SHARED_MEM_DEFAULT_STATUS;

  volatile uint64_t dummy;

  switch(hls->my_hart_id)
  {

    case 0U:
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h0));
      snvm_e51();
      break;

    case 1U:
      //(void)init_pmp((uint8_t)1);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h1));
      snvm_u54_1();
      break;

    case 2U:
      //(void)init_pmp((uint8_t)2);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h2));
      snvm_u54_2();
      break;

    case 3U:
      //(void)init_pmp((uint8_t)3);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h3));
      snvm_u54_3();
      break;

    case 4U:
      //(void)init_pmp((uint8_t)4);
      __asm volatile ("add sp, x0, %1" : "=r"(dummy) : "r"(app_stack_top_h4));
      snvm_u54_4();
      break;

    default:
      /* no more harts */
      break;
  }

  /* should never get here */
  while(true)
  {
    static volatile uint64_t counter = 0U;
    /* Added some code as debugger hangs if in loop doing nothing */
    counter = counter + 1U;
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

    hls_e51->my_hart_id = hartid;
    hls_e51->in_wfi_indicator = HLS_MAIN_HART_STARTED;

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
          hls_u54 = (HLS_DATA*)(stack_top - HLS_DEBUG_AREA_SIZE);
          snvm_printf("[hart #%d] hsl(0x%08lx)\r\n",
              u54_hart_id, (unsigned long)(uintptr_t)hls_u54);
          sm_check_thread = SNVM_CHECK_WFI;
          wait_count = 0U;
          break;

        case SNVM_CHECK_WFI:
          if (hls_u54->in_wfi_indicator == HLS_OTHER_HART_IN_WFI)
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
          if (hls_u54->in_wfi_indicator == HLS_OTHER_HART_PASSED_WFI)
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
              if(hls_u54->in_wfi_indicator == HLS_OTHER_HART_IN_WFI )
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
    hls_e51->in_wfi_indicator = HLS_MAIN_HART_FIN_INIT;
    (void)snvm_other_main(hls_e51);
  }


  while(true)
    {
       static volatile uint64_t counter = 0U;
       /* Added some code as debugger hangs if in loop doing nothing */
       asm ("nop");
       asm ("nop");
       asm ("nop");
       asm ("nop");
       counter = counter + 1U;
       asm ("nop");
       asm ("nop");
       asm ("nop");
    }

    return (0);
}
