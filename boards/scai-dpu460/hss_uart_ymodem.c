/*******************************************************************************
 * Copyright 2019-2025 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * MPFS HSS Embedded Software
 *
 */

#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "uart_helper.h"

#include "ymodem.h"

void HSS_UART_enter_ymodem(void *this_uart)
{
  mHSS_PRINTF("\r\n\r\n");
  mHSS_PRINTF("Please, change baud rate from %d to %d ...)\n",
      MSS_UART_921600_BAUD, MSS_UART_115200_BAUD);
  mHSS_PRINTF("When file transfer is completed, change baud rate from %d to %d ...)\n",
      MSS_UART_115200_BAUD, MSS_UART_921600_BAUD);
  mHSS_PRINTF("\r\n\r\n");

  MSS_UART_init((mss_uart_instance_t *)this_uart, MSS_UART_115200_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
}

void HSS_UART_exit_ymodem(void* this_uart)
{
  MSS_UART_init((mss_uart_instance_t *)this_uart, MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
}
