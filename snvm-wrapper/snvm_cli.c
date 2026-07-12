#include "snvm_cli.h"

#include "mpfs_hal/mss_hal.h"
#include "mpfs_hal/common/mss_peripherals.h"
#include "mpfs_hal/common/mss_util.h"

#include "snvm_log.h"
#include "crc32.h"

#include <stdint.h>
#include <stdbool.h>

typedef int (*cli_handler_t)(int argc, char *argv[]);

typedef struct {
  const char      *cmd;
  cli_handler_t    handler;
  const char      *help;
} cli_command_t;

static int cmd_help(int argc, char *argv[]);
static int cmd_boot(int argc, char *argv[]);
static int cmd_memdump(int argc, char *argv[]);

static cli_command_t commands[] =
{
  { "help",    cmd_help,    "Show commands" },
  { "boot",    cmd_boot,    "Booting to eNVM" },
  { "memdump", cmd_memdump, "memdump <addr> <size>" },
};

#define NUM_COMMANDS (sizeof(commands)/sizeof(commands[0]))

static int cmd_help(int argc, char *argv[])
{
  unsigned int i;

  for (i = 0; i < NUM_COMMANDS; i++)
  {
    snvm_printf("%s : %s\r\n",
        commands[i].cmd,
        commands[i].help);
  }

  return 0;
}

static int cmd_boot(int argc, char *argv[])
{
  snvm_printf("Try to jump to eNVM ...\r\n");
  return 1;
}

static bool snvm_parse_u64(const char *str, uint64_t *value)
{
  uint64_t result = 0;
  uint32_t base = 10;

  if (str == NULL || *str == '\0')
    return false;

  if ((str[0] == '0') &&
      (str[1] == 'x' || str[1] == 'X'))
  {
    base = 16;
    str += 2;
  }

  if (*str == '\0')
    return false;

  while (*str)
  {
    uint32_t digit;

    if (*str >= '0' && *str <= '9')
      digit = *str - '0';
    else if (base == 16 && *str >= 'a' && *str <= 'f')
      digit = *str - 'a' + 10;
    else if (base == 16 && *str >= 'A' && *str <= 'F')
      digit = *str - 'A' + 10;
    else
      return false;

    if (digit >= base)
      return false;

    result = result * base + digit;
    str++;
  }

  *value = result;
  return true;
}

static int snvm_strcmp(const char *a, const char *b)
{
  while (*a && (*a == *b))
  {
    a++;
    b++;
  }

  return (unsigned char)*a - (unsigned char)*b;
}

static int cmd_memdump(int argc, char *argv[])
{
  uint64_t addr;
  uint64_t size;
  uint8_t * volatile mmap = NULL;

  if (argc != 3) {
    return -1;
  }

  if (!snvm_parse_u64(argv[1], &addr) ||
      !snvm_parse_u64(argv[2], &size))
  {
    snvm_printf("Invalid argument\r\n");
    return -1;
  }
  mmap = (uint8_t *)addr;

  snvm_hexdump("MEM", (void *)mmap, size);

  return 0;
}

static void uart_getline(char *line, size_t size)
{
  size_t idx = 0;
  uint8_t ch;

  if (size == 0)
    return;

  while (1)
  {
    if (MSS_UART_get_rx(&g_mss_uart2_lo, &ch, 1) != 1)
      continue;

    switch (ch)
    {
      case '\r':
      case '\n':
        /* Echo newline */
        MSS_UART_polled_tx_string(&g_mss_uart2_lo, (const uint8_t *)"\r\n");

        line[idx] = '\0';
        return;

      case '\b':
      case 0x7F:  /* DEL */
        if (idx > 0)
        {
          idx--;

          /* Erase character on terminal */
          MSS_UART_polled_tx_string(&g_mss_uart2_lo,
              (const uint8_t *)"\b \b");
        }
        break;

      default:
        if (idx < (size - 1))
        {
          line[idx++] = (char)ch;

          /* Echo */
          MSS_UART_polled_tx(&g_mss_uart2_lo, &ch, 1);
        }
        break;
    }
  }
}

static int cli_parse(char *line, char *argv[])
{
  int argc = 0;

  while (*line)
  {
    while (*line == ' ' || *line == '\t')
      line++;

    if (*line == '\0')
      break;

    argv[argc++] = line;

    while (*line &&
        *line != ' ' &&
        *line != '\t' &&
        *line != '\r' &&
        *line != '\n')
    {
      line++;
    }

    if (*line == '\0')
      break;

    *line++ = '\0';

    if (argc >= 16)
      break;
  }

  return argc;
}

static int snvm_cli_process(char *line)
{
  char *argv[16];
  int argc;
  unsigned int i;
  int ret;

  argc = cli_parse(line, argv);

  if (argc == 0)
    return 0;

  for (i = 0; i < NUM_COMMANDS; i++)
  {
    if (!snvm_strcmp(argv[0], commands[i].cmd))
    {
      ret = commands[i].handler(argc, argv);
      if (ret < 0)
      {
        snvm_printf("Usage: %s\r\n",
            commands[i].help);
      }
      return ret;
    }
  }

  snvm_printf("Unknown command: %s\r\n", argv[0]);

  return 0;
}

static int __do_cli(void)
{
  char line[128];
  int ret;

  snvm_printf("\r\n");
  while (1)
  {
    //MSS_UART_polled_tx_string(g_uart, (const uint8_t *)"snvm> ");
    snvm_printf("snvm> ");

    uart_getline(line, sizeof(line));

    ret = snvm_cli_process(line);
    if (ret > 0)
      break;
  }

  return ret;
}

void do_cli(uint32_t timeout)
{
  uint32_t i;
  uint64_t wait_usecs = 1000000;
  uint8_t ch;
  int ret;

  snvm_printf("[%s] Try to \'Enter\' key if you want to enter command line interface ...\r\n", __func__);
  snvm_printf("[%s] Timeout %d seconds\r\n", __func__, timeout);
  for (i = 0; i < timeout; i++) {
    snvm_printf(".");
    sleep_ms(wait_usecs);
    if (MSS_UART_get_rx(&g_mss_uart2_lo, &ch, 1) == 1)
    {
      if (ch == '\r' || ch == '\n')
      {
        /* Execute command */
        ret = __do_cli();
        if (ret > 0)
          break;
      }
    }
  }
  snvm_printf("\r\n");
}
