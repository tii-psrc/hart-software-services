#ifndef SNVM_UART_H
#define SNVM_UART_H

#include "snvm_config.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"

mss_uart_instance_t *uart_instance(enum SNVM_HSSHartId hartid);
void snvm_uart_init(void);
size_t snvm_uart_rx(mss_uart_instance_t *this_uart,
    uint8_t *buf, size_t size);
size_t snvm_uart_tx(mss_uart_instance_t *this_uart,
    uint8_t *buf, size_t size);

#endif
