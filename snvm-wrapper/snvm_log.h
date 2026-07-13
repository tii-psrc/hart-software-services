#ifndef SNVM_LOG_H
#define SNVM_LOG_H

#include <stdarg.h>
#include <stdint.h>

void snvm_putc_hart(int hartid, char c);
void snvm_putc(char c);

void snvm_puts_hart(int hartid, const char *s);
void snvm_puts(const char *s);

void snvm_vprintf_hart(int hartid, const char *fmt, va_list ap);
void snvm_vprintf(const char *fmt, va_list ap);

void snvm_printf_hart(int hartid, const char *fmt, ...);
void snvm_printf(const char *fmt, ...);

void snvm_hexdump_hart(int hartid, const char *label, const void *buf,
    uint32_t len);
void snvm_hexdump(const char *label, const void *buf, uint32_t len);

#endif
