#include "snvm_cli.h"

#include "mpfs_hal/mss_hal.h"
#include "mpfs_hal/common/mss_peripherals.h"
#include "mpfs_hal/common/mss_util.h"

#include "snvm_log.h"
#include "snvm_utils.h"
#include "snvm_uart.h"

#include <stdint.h>
#include <stdbool.h>

typedef int (*snvm_cli_handler_t)(int argc, char *argv[]);

typedef struct {
  const char *cmd;
  snvm_cli_handler_t handler;
  const char *help;
} snvm_cli_command_t;

static int snvm_cmd_help(int argc, char *argv[]);
static int snvm_cmd_boot(int argc, char *argv[]);
static int snvm_cmd_memdump(int argc, char *argv[]);
static int snvm_cmd_crc32(int argc, char *argv[]);

static snvm_cli_command_t snvm_commands[] =
{
  { "help",    snvm_cmd_help,    "Show commands" },
  { "boot",    snvm_cmd_boot,    "Booting to eNVM" },
  { "memdump", snvm_cmd_memdump, "memdump <addr> <size>" },
  { "crc32",   snvm_cmd_crc32,   "crc32 <addr> <size>" },
};

#define SNVM_NUM_COMMANDS (sizeof(snvm_commands)/sizeof(snvm_commands[0]))

static int snvm_cmd_help(int argc, char *argv[])
{
  unsigned int i;

  for (i = 0; i < SNVM_NUM_COMMANDS; i++)
  {
    snvm_printf("%s : %s\r\n",
        snvm_commands[i].cmd,
        snvm_commands[i].help);
  }

  return 0;
}

static int snvm_cmd_boot(int argc, char *argv[])
{
  snvm_printf("Try to jump to eNVM ...\r\n");
  return 1;
}

static int snvm_cmd_memdump(int argc, char *argv[])
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

static int snvm_cmd_crc32(int argc, char *argv[])
{
  uint64_t addr;
  uint64_t size;
  uint8_t * volatile mmap = NULL;
  uint32_t crc32 = 0;

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

  crc32 = snvm_crc32(0, mmap, size);
  snvm_printf("[%s] crc32(0x%08X)\r\n", __func__, crc32);

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
    if (snvm_uart_rx(uart_instance(SNVM_HSS_HART_E51), &ch, 1) != 1)
      continue;

    switch (ch)
    {
      case '\r':
      case '\n':
        /* Echo newline */
        snvm_puts((const char *)"\r\n");

        line[idx] = '\0';
        return;

      case '\b':
      case 0x7F:  /* DEL */
        if (idx > 0)
        {
          idx--;

          /* Erase character on terminal */
          snvm_puts((const char *)"\b \b");
        }
        break;

      default:
        if (idx < (size - 1))
        {
          line[idx++] = (char)ch;

          /* Echo */
          snvm_putc(ch);
        }
        break;
    }
  }
}

static int snvm_cli_parse(char *line, char *argv[])
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

  argc = snvm_cli_parse(line, argv);

  if (argc == 0)
    return 0;

  for (i = 0; i < SNVM_NUM_COMMANDS; i++)
  {
    if (!snvm_strcmp(argv[0], snvm_commands[i].cmd))
    {
      ret = snvm_commands[i].handler(argc, argv);
      if (ret < 0)
      {
        snvm_printf("Usage: %s\r\n",
            snvm_commands[i].help);
      }
      return ret;
    }
  }

  snvm_printf("Unknown command: %s\r\n", argv[0]);

  return 0;
}

static int __snvm_cli(int32_t timeout)
{
  char line[128];
  int ret;

  snvm_printf("\r\n");
  while (1)
  {
    snvm_printf("snvm> ");

    uart_getline(line, sizeof(line));

    ret = snvm_cli_process(line);
    if (ret > 0 && timeout > 0)
      break;
  }

  return ret;
}

void snvm_cli(int32_t timeout)
{
  uint32_t i;
  uint64_t wait_usecs = 1000000;
  uint8_t ch;
  int ret;

  if (timeout < 0)
    __snvm_cli(timeout);

  snvm_printf("[%s] Try to \'Enter\' key if you want to enter command line interface ...\r\n", __func__);
  snvm_printf("[%s] Timeout %d seconds\r\n", __func__, timeout);
  for (i = 0; i < timeout; i++) {
    snvm_printf(".");
    sleep_ms(wait_usecs);
    if (snvm_uart_rx(uart_instance(SNVM_HSS_HART_E51), &ch, 1) == 1)
    {
      if (ch == '\r' || ch == '\n')
      {
        /* Execute command */
        ret = __snvm_cli(timeout);
        if (ret > 0)
          break;
      }
    }
  }
  snvm_printf("\r\n");
}
