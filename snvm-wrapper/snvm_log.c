#include "snvm_log.h"

#include <stddef.h>
#include <stdio.h>

#include "snvm_config.h"
#include "snvm_uart.h"
#include "snvm_spin_lock.h"

static snvm_spinlock_t snvm_log_lock;

static void snvm_putc_hart(int hartid, char c);
static void snvm_putc_hart(int hartid, char c)
{
  uint8_t ch = (uint8_t)c;
  snvm_uart_tx(uart_instance(hartid), &ch, 1u);
}

#if 0
static void snvm_putc(char c);
static void snvm_putc(char c)
{
  int hartid = SNVM_HSS_HART_E51;

  snvm_putc_hart(hartid, c);
}
#endif

static void snvm_puts_hart(int hartid, const char *s);
static void snvm_puts_hart(int hartid, const char *s)
{
  if (s == NULL) {
    s = "(null)";
  }

  while (*s != '\0') {
    if (*s == '\n') {
      snvm_putc_hart(hartid, '\r');
    }
    snvm_putc_hart(hartid, *s++);
  }
}

#if 0
static void snvm_puts(const char *s);
static void snvm_puts(const char *s)
{
  int hartid = SNVM_HSS_HART_E51;

  snvm_puts_hart(hartid, s);
}
#endif

/* -------------------------------------------------------------------------- */
/* Formatter internals                                                         */
/* -------------------------------------------------------------------------- */

static void snvm_putnchar_hart(int hartid, char ch, unsigned int count);
static void snvm_putnchar_hart(int hartid, char ch, unsigned int count)
{
  while (count-- > 0U) {
    snvm_putc_hart(hartid, ch);
  }
}

static unsigned int snvm_u64_to_str(uint64_t value,
    uint32_t base,
    int uppercase,
    char *buf,
    unsigned int buf_sz);
static unsigned int snvm_u64_to_str(uint64_t value,
    uint32_t base,
    int uppercase,
    char *buf,
    unsigned int buf_sz)
{
  const char *digits_l = "0123456789abcdef";
  const char *digits_u = "0123456789ABCDEF";
  const char *digits = uppercase ? digits_u : digits_l;
  unsigned int i = 0U;

  if ((buf == NULL) || (buf_sz == 0U)) {
    return 0U;
  }

  if ((base < 2U) || (base > 16U)) {
    buf[0] = '\0';
    return 0U;
  }

  if (value == 0U) {
    if (buf_sz >= 2U) {
      buf[0] = '0';
      buf[1] = '\0';
      return 1U;
    }
    buf[0] = '\0';
    return 0U;
  }

  while ((value != 0U) && (i < (buf_sz - 1U))) {
    buf[i++] = digits[value % base];
    value /= base;
  }

  /* reverse in-place */
  for (unsigned int j = 0U; j < (i / 2U); j++) {
    char tmp = buf[j];
    buf[j] = buf[i - 1U - j];
    buf[i - 1U - j] = tmp;
  }

  buf[i] = '\0';
  return i;
}

static void snvm_print_u64_width(int hartid,
    uint64_t value,
    uint32_t base,
    int uppercase,
    unsigned int width,
    int zero_pad);
static void snvm_print_u64_width(int hartid,
    uint64_t value,
    uint32_t base,
    int uppercase,
    unsigned int width,
    int zero_pad)
{
  char tmp[32];
  unsigned int len = snvm_u64_to_str(value, base, uppercase, tmp, sizeof(tmp));

  if ((width > len) && zero_pad) {
    snvm_putnchar_hart(hartid, '0', width - len);
  } else if (width > len) {
    snvm_putnchar_hart(hartid, ' ', width - len);
  }

  snvm_puts_hart(hartid, tmp);
}

static void snvm_print_i64_width(int hartid,
    int64_t value,
    unsigned int width,
    int zero_pad);
static void snvm_print_i64_width(int hartid,
    int64_t value,
    unsigned int width,
    int zero_pad)
{
  uint64_t mag;
  unsigned int sign_len = 0U;

  if (value < 0) {
    /* handle sign first */
    snvm_putc_hart(hartid, '-');
    sign_len = 1U;

    /* avoid UB-ish direct -INT64_MIN handling */
    mag = (uint64_t)(-(value + 1)) + 1U;
  } else {
    mag = (uint64_t)value;
  }

  if (width > sign_len) {
    snvm_print_u64_width(hartid, mag, 10U, 0, width - sign_len, zero_pad);
  } else {
    snvm_print_u64_width(hartid, mag, 10U, 0, 0U, zero_pad);
  }
}

static void snvm_print_ptr(int hartid, uintptr_t ptr, unsigned int width,
    int zero_pad);
static void snvm_print_ptr(int hartid, uintptr_t ptr, unsigned int width,
    int zero_pad)
{
  /* If width omitted, default to full pointer width in hex digits */
  unsigned int hex_digits = (unsigned int)(sizeof(uintptr_t) * 2U);

  snvm_puts_hart(hartid, "0x");

  if (width == 0U) {
    width = hex_digits;
    zero_pad = 1;
  }

  snvm_print_u64_width(hartid, (uint64_t)ptr, 16U, 0, width, zero_pad);
}

/* -------------------------------------------------------------------------- */
/* tiny printf                                                                 */
/* -------------------------------------------------------------------------- */

static void snvm_vprintf_hart(int hartid, const char *fmt, va_list ap);
static void snvm_vprintf_hart(int hartid, const char *fmt, va_list ap)
{
  snvm_spin_lock(&snvm_log_lock);

  while ((*fmt) != '\0') {
    if (*fmt != '%') {
      snvm_putc_hart(hartid, *fmt++);
      continue;
    }

    fmt++; /* skip '%' */

    if (*fmt == '\0') {
      break;
    }

    /* ---- flags ---- */
    int zero_pad = 0;
    if (*fmt == '0') {
      zero_pad = 1;
      fmt++;
    }

    /* ---- width ---- */
    unsigned int width = 0U;
    while ((*fmt >= '0') && (*fmt <= '9')) {
      width = (width * 10U) + (unsigned int)(*fmt - '0');
      fmt++;
    }

    /* ---- length modifier ---- */
    int long_flag = 0;
    int long_long_flag = 0;

    if (*fmt == 'l') {
      long_flag = 1;
      fmt++;
      if (*fmt == 'l') {
        long_long_flag = 1;
        fmt++;
      }
    }

    switch (*fmt) {
      case '%':
        snvm_putc_hart(hartid, '%');
        break;

      case 'c': {
                  int c = va_arg(ap, int);
                  snvm_putc_hart(hartid, (char)c);
                  break;
                }

      case 's': {
                  const char *s = va_arg(ap, const char *);
                  unsigned int len = 0U;
                  const char *p = (s != NULL) ? s : "(null)";

                  while (p[len] != '\0') {
                    len++;
                  }

                  if ((width > len) && !zero_pad) {
                    snvm_putnchar_hart(hartid, ' ', width - len);
                  } else if ((width > len) && zero_pad) {
                    snvm_putnchar_hart(hartid, '0', width - len);
                  }

                  snvm_puts_hart(hartid, p);
                  break;
                }

      case 'd':
      case 'i':
                if (long_long_flag) {
                  snvm_print_i64_width(hartid, va_arg(ap, long long), width,
                      zero_pad);
                } else if (long_flag) {
                  snvm_print_i64_width(hartid, va_arg(ap, long), width,
                      zero_pad);
                } else {
                  snvm_print_i64_width(hartid, va_arg(ap, int), width,
                      zero_pad);
                }
                break;

      case 'u':
                if (long_long_flag) {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned long long),
                      10U, 0, width, zero_pad);
                } else if (long_flag) {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned long),
                      10U, 0, width, zero_pad);
                } else {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned int),
                      10U, 0, width, zero_pad);
                }
                break;

      case 'x':
                if (long_long_flag) {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned long long),
                      16U, 0, width, zero_pad);
                } else if (long_flag) {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned long),
                      16U, 0, width, zero_pad);
                } else {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned int),
                      16U, 0, width, zero_pad);
                }
                break;

      case 'X':
                if (long_long_flag) {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned long long),
                      16U, 1, width, zero_pad);
                } else if (long_flag) {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned long),
                      16U, 1, width, zero_pad);
                } else {
                  snvm_print_u64_width(hartid, va_arg(ap, unsigned int),
                      16U, 1, width, zero_pad);
                }
                break;

      case 'p': {
                  uintptr_t ptr = (uintptr_t)va_arg(ap, void *);
                  snvm_print_ptr(hartid, ptr, width, zero_pad);
                  break;
                }

      default:
                /* unsupported format: print it back literally */
                snvm_putc_hart(hartid, '%');
                if (zero_pad) {
                  snvm_putc_hart(hartid, '0');
                }
                if (width != 0U) {
                  /* width 숫자를 다시 찍어주기 */
                  snvm_print_u64_width(hartid, width, 10U, 0, 0U, 0);
                }
                if (long_flag) {
                  snvm_putc_hart(hartid, 'l');
                  if (long_long_flag) {
                    snvm_putc_hart(hartid, 'l');
                  }
                }
                snvm_putc_hart(hartid, *fmt);
                break;
    }

    fmt++;
  }

  snvm_spin_unlock(&snvm_log_lock);
}

void snvm_printf_hart(int hartid, const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  snvm_vprintf_hart(hartid, fmt, ap);
  va_end(ap);
}

void snvm_printf(const char *fmt, ...)
{
  int hartid = SNVM_HSS_HART_E51;
  va_list ap;

  va_start(ap, fmt);
  snvm_vprintf_hart(hartid, fmt, ap);
  va_end(ap);
}

void snvm_hexdump_hart(int hartid, const char *label, const void *buf,
    uint32_t len)
{
  snvm_spin_lock(&snvm_log_lock);

  const uint8_t *p = (const uint8_t *)buf;
  uint32_t offset = 0U;

  if (label == NULL) {
    label = "hexdump";
  }

  snvm_printf_hart(hartid, "%s @ %p, len=%u\r\n", label, buf, len);

  while (offset < len) {
    uint32_t i;
    uint32_t line_len = ((len - offset) > 16U) ? 16U : (len - offset);

    /* line start address */
    snvm_printf_hart(hartid, "%016lX: ",
        (unsigned long)((uintptr_t)p + offset));

    /* hex bytes: 16 bytes per line */
    for (i = 0U; i < 16U; i++) {
      if (i < line_len) {
        snvm_printf_hart(hartid, "%02X ", p[offset + i]);
      } else {
        snvm_puts_hart(hartid, "   ");
      }

      /* 8-byte boundary spacing */
      if (i == 7U) {
        snvm_putc_hart(hartid, ' ');
      }
    }

    snvm_puts_hart(hartid, " |");

    /* ASCII view */
    for (i = 0U; i < line_len; i++) {
      uint8_t c = p[offset + i];
      if ((c >= 32U) && (c <= 126U)) {
        snvm_putc_hart(hartid, (char)c);
      } else {
        snvm_putc_hart(hartid, '.');
      }
    }

    /* pad ASCII area so short last line aligns nicely */
    for (; i < 16U; i++) {
      snvm_putc_hart(hartid, ' ');
    }

    snvm_puts_hart(hartid, "|\n");

    offset += line_len;
  }

  snvm_spin_unlock(&snvm_log_lock);
}

void snvm_hexdump(const char *label, const void *buf, uint32_t len)
{
  int hartid = SNVM_HSS_HART_E51;

  snvm_hexdump_hart(hartid, label, buf, len);
}
