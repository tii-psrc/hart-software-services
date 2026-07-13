#include "snvm_uart.h"

#include "mpfs_hal/common/mss_sysreg.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "mpfs_hal/common/mss_peripherals.h"

#include "snvm_config.h"

mss_uart_instance_t *uart_instance(enum SNVM_HSSHartId hartid)
{
  mss_uart_instance_t *this_uart = NULL;

  switch (hartid) {
    default:
    case SNVM_HSS_HART_E51:
      this_uart = &g_mss_uart2_lo;
      break;

    case SNVM_HSS_HART_U54_1:
    case SNVM_HSS_HART_U54_2:
    case SNVM_HSS_HART_U54_3:
    case SNVM_HSS_HART_U54_4:
      this_uart = &g_mss_uart1_lo;
      break;
  }

  return this_uart;
}

void snvm_uart_init(void)
{
  SYSREG->SUBBLK_CLOCK_CR |= (SUBBLK_CLOCK_CR_MMUART2_MASK);
  SYSREG->SOFT_RESET_CR   &= (uint32_t)(~SUBBLK_CLOCK_CR_MMUART2_MASK);
  SYSREG->SUBBLK_CLOCK_CR |= (SUBBLK_CLOCK_CR_MMUART1_MASK);
  SYSREG->SOFT_RESET_CR   &= (uint32_t)(~SUBBLK_CLOCK_CR_MMUART1_MASK);

  (void) mss_config_clk_rst(MSS_PERIPH_MMUART2, (uint8_t) 0, PERIPHERAL_ON);
  (void) mss_config_clk_rst(MSS_PERIPH_MMUART1, (uint8_t) 0, PERIPHERAL_ON);

  MSS_UART_init(uart_instance(SNVM_HSS_HART_E51),
      MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
  MSS_UART_init(uart_instance(SNVM_HSS_HART_U54_1),
      MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

   
  MSS_UART_polled_tx_string(uart_instance(SNVM_HSS_HART_E51), 
      (const uint8_t *)"\r\nMSS_UART E51 Test @snvm_main()\r\n");
  MSS_UART_polled_tx_string(uart_instance(SNVM_HSS_HART_U54_1),
      (const uint8_t *)"\r\nMSS_UART U54_1 Test @snvm_main()\r\n");
}

size_t snvm_uart_rx(mss_uart_instance_t *this_uart,
    uint8_t *buf, size_t size)
{ 
  return MSS_UART_get_rx(this_uart, buf, size);
}

size_t snvm_uart_tx(mss_uart_instance_t *this_uart,
    uint8_t *buf, size_t size)
{ 
  MSS_UART_polled_tx(this_uart, buf, size);
  return 0;
}
