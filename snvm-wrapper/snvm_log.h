#ifndef SNVM_LOG_H
#define SNVM_LOG_H

#include <stdarg.h>
#include <stdint.h>

void snvm_printf_hart(int hartid, const char *fmt, ...);
void snvm_printf(const char *fmt, ...);

void snvm_hexdump_hart(int hartid, const char *label, const void *buf,
    uint32_t len);
void snvm_hexdump(const char *label, const void *buf, uint32_t len);

void snvm_log_init(void);

#endif
