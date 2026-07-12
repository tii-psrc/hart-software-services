#include "snvm_log.h"

#include <stddef.h>
#include <stdio.h>

#include "mpfs_hal/common/mss_sysreg.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "mpfs_hal/common/mss_peripherals.h"

void snvm_uart_init(void)
{
  SYSREG->SUBBLK_CLOCK_CR |= (SUBBLK_CLOCK_CR_MMUART2_MASK);
  SYSREG->SOFT_RESET_CR   &= (uint32_t)(~SUBBLK_CLOCK_CR_MMUART2_MASK);
  SYSREG->SUBBLK_CLOCK_CR |= (SUBBLK_CLOCK_CR_MMUART1_MASK);
  SYSREG->SOFT_RESET_CR   &= (uint32_t)(~SUBBLK_CLOCK_CR_MMUART1_MASK);

  (void) mss_config_clk_rst(MSS_PERIPH_MMUART2, (uint8_t) 0, PERIPHERAL_ON);
  (void) mss_config_clk_rst(MSS_PERIPH_MMUART1, (uint8_t) 0, PERIPHERAL_ON);

  MSS_UART_init(&g_mss_uart2_lo,
      MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
  MSS_UART_init(&g_mss_uart1_lo,
      MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

   
  MSS_UART_polled_tx_string(&g_mss_uart2_lo, 
      (const uint8_t *)"\r\nMSS_UART #2 Test @snvm_main()\r\n");
  MSS_UART_polled_tx_string(&g_mss_uart1_lo,
      (const uint8_t *)"\r\nMSS_UART #1 Test @snvm_main()\r\n");
}

void snvm_putc(char c)
{
  uint8_t ch = (uint8_t)c;
  MSS_UART_polled_tx(&g_mss_uart2_lo, &ch, 1u);
  MSS_UART_polled_tx(&g_mss_uart1_lo, &ch, 1u);
}

void snvm_puts(const char *s)
{
  if (s == NULL) {
    s = "(null)";
  }

  while (*s != '\0') {
    if (*s == '\n') {
      snvm_putc('\r');
    }
    snvm_putc(*s++);
  }
}

/* -------------------------------------------------------------------------- */
/* Formatter internals                                                         */
/* -------------------------------------------------------------------------- */

static void snvm_putnchar(char ch, unsigned int count);
static void snvm_putnchar(char ch, unsigned int count)
{
  while (count-- > 0U) {
    snvm_putc(ch);
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

static void snvm_print_u64_width(uint64_t value,
    uint32_t base,
    int uppercase,
    unsigned int width,
    int zero_pad);
static void snvm_print_u64_width(uint64_t value,
    uint32_t base,
    int uppercase,
    unsigned int width,
    int zero_pad)
{
  char tmp[32];
  unsigned int len = snvm_u64_to_str(value, base, uppercase, tmp, sizeof(tmp));

  if ((width > len) && zero_pad) {
    snvm_putnchar('0', width - len);
  } else if (width > len) {
    snvm_putnchar(' ', width - len);
  }

  snvm_puts(tmp);
}

static void snvm_print_i64_width(int64_t value,
    unsigned int width,
    int zero_pad);
static void snvm_print_i64_width(int64_t value,
    unsigned int width,
    int zero_pad)
{
  uint64_t mag;
  unsigned int sign_len = 0U;

  if (value < 0) {
    /* handle sign first */
    snvm_putc('-');
    sign_len = 1U;

    /* avoid UB-ish direct -INT64_MIN handling */
    mag = (uint64_t)(-(value + 1)) + 1U;
  } else {
    mag = (uint64_t)value;
  }

  if (width > sign_len) {
    snvm_print_u64_width(mag, 10U, 0, width - sign_len, zero_pad);
  } else {
    snvm_print_u64_width(mag, 10U, 0, 0U, zero_pad);
  }
}

static void snvm_print_ptr(uintptr_t ptr, unsigned int width, int zero_pad);
static void snvm_print_ptr(uintptr_t ptr, unsigned int width, int zero_pad)
{
  /* If width omitted, default to full pointer width in hex digits */
  unsigned int hex_digits = (unsigned int)(sizeof(uintptr_t) * 2U);

  snvm_puts("0x");

  if (width == 0U) {
    width = hex_digits;
    zero_pad = 1;
  }

  snvm_print_u64_width((uint64_t)ptr, 16U, 0, width, zero_pad);
}

/* -------------------------------------------------------------------------- */
/* tiny printf                                                                 */
/* -------------------------------------------------------------------------- */

void snvm_vprintf(const char *fmt, va_list ap)
{
  while ((*fmt) != '\0') {
    if (*fmt != '%') {
      snvm_putc(*fmt++);
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
        snvm_putc('%');
        break;

      case 'c': {
                  int c = va_arg(ap, int);
                  snvm_putc((char)c);
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
                    snvm_putnchar(' ', width - len);
                  } else if ((width > len) && zero_pad) {
                    snvm_putnchar('0', width - len);
                  }

                  snvm_puts(p);
                  break;
                }

      case 'd':
      case 'i':
                if (long_long_flag) {
                  snvm_print_i64_width(va_arg(ap, long long), width, zero_pad);
                } else if (long_flag) {
                  snvm_print_i64_width(va_arg(ap, long), width, zero_pad);
                } else {
                  snvm_print_i64_width(va_arg(ap, int), width, zero_pad);
                }
                break;

      case 'u':
                if (long_long_flag) {
                  snvm_print_u64_width(va_arg(ap, unsigned long long),
                      10U, 0, width, zero_pad);
                } else if (long_flag) {
                  snvm_print_u64_width(va_arg(ap, unsigned long),
                      10U, 0, width, zero_pad);
                } else {
                  snvm_print_u64_width(va_arg(ap, unsigned int),
                      10U, 0, width, zero_pad);
                }
                break;

      case 'x':
                if (long_long_flag) {
                  snvm_print_u64_width(va_arg(ap, unsigned long long),
                      16U, 0, width, zero_pad);
                } else if (long_flag) {
                  snvm_print_u64_width(va_arg(ap, unsigned long),
                      16U, 0, width, zero_pad);
                } else {
                  snvm_print_u64_width(va_arg(ap, unsigned int),
                      16U, 0, width, zero_pad);
                }
                break;

      case 'X':
                if (long_long_flag) {
                  snvm_print_u64_width(va_arg(ap, unsigned long long),
                      16U, 1, width, zero_pad);
                } else if (long_flag) {
                  snvm_print_u64_width(va_arg(ap, unsigned long),
                      16U, 1, width, zero_pad);
                } else {
                  snvm_print_u64_width(va_arg(ap, unsigned int),
                      16U, 1, width, zero_pad);
                }
                break;

      case 'p': {
                  uintptr_t ptr = (uintptr_t)va_arg(ap, void *);
                  snvm_print_ptr(ptr, width, zero_pad);
                  break;
                }

      default:
                /* unsupported format: print it back literally */
                snvm_putc('%');
                if (zero_pad) {
                  snvm_putc('0');
                }
                if (width != 0U) {
                  /* width 숫자를 다시 찍어주기 */
                  snvm_print_u64_width(width, 10U, 0, 0U, 0);
                }
                if (long_flag) {
                  snvm_putc('l');
                  if (long_long_flag) {
                    snvm_putc('l');
                  }
                }
                snvm_putc(*fmt);
                break;
    }

    fmt++;
  }
}

void snvm_printf(const char *fmt, ...)
{
  va_list ap;

  va_start(ap, fmt);
  snvm_vprintf(fmt, ap);
  va_end(ap);
}

void snvm_hexdump(const char *label, const void *buf, uint32_t len)
{
  const uint8_t *p = (const uint8_t *)buf;
  uint32_t offset = 0U;

  if (label == NULL) {
    label = "hexdump";
  }

  snvm_printf("%s @ %p, len=%u\n", label, buf, len);

  while (offset < len) {
    uint32_t i;
    uint32_t line_len = ((len - offset) > 16U) ? 16U : (len - offset);

    /* line start address */
    snvm_printf("%016lX: ", (unsigned long)((uintptr_t)p + offset));

    /* hex bytes: 16 bytes per line */
    for (i = 0U; i < 16U; i++) {
      if (i < line_len) {
        snvm_printf("%02X ", p[offset + i]);
      } else {
        snvm_puts("   ");
      }

      /* 8-byte boundary spacing */
      if (i == 7U) {
        snvm_putc(' ');
      }
    }

    snvm_puts(" |");

    /* ASCII view */
    for (i = 0U; i < line_len; i++) {
      uint8_t c = p[offset + i];
      if ((c >= 32U) && (c <= 126U)) {
        snvm_putc((char)c);
      } else {
        snvm_putc('.');
      }
    }

    /* pad ASCII area so short last line aligns nicely */
    for (; i < 16U; i++) {
      snvm_putc(' ');
    }

    snvm_puts("|\n");

    offset += line_len;
  }
}
