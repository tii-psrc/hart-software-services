/* micron_mt29f.c
 *
 * Driver for Micron MT29F NAND flash memory using APB QSPI interface
 * on PolarFire SoC FPGA.
 * 

*/
#include "micron_mt29.h"
#include "fpga_interface.h"
#include "mss_qspi.h"


// Constants for MT29F commands and status
#define NAND_CMD_READ_ID             0x90
#define NAND_CMD_RESET               0xFF
#define NAND_CMD_READ_PAGE           0x00
#define NAND_CMD_READ_PAGE_CONFIRM   0x30
#define NAND_CMD_PROGRAM_PAGE        0x80
#define NAND_CMD_PROGRAM_PAGE_CONFIRM 0x10
#define NAND_CMD_ERASE_BLOCK         0x60
#define NAND_CMD_ERASE_BLOCK_CONFIRM 0xD0
#define NAND_CMD_STATUS              0x70

#define NAND_STATUS_READY            (1 << 6)
#define NAND_STATUS_FAIL             (1 << 0)

#define MT29_PAGE_SIZE               2048
#define MT29_BLOCK_SIZE              (MT29_PAGE_SIZE * 64)

// Function to send a command to the NAND flash
static void nand_send_command(uint8_t cmd) {
    FPGA_QSPI_Write(&cmd, 0, 1);
}

// Function to send address cycles
static void nand_send_address(uint32_t addr, uint8_t cycles) {
    for (uint8_t i = 0; i < cycles; i++) {
        uint8_t byte = (addr >> (i * 8)) & 0xFF;
        FPGA_QSPI_Write(&byte, 0, 1);
    }
}

// Функция для проверки статуса
static bool nand_check_status(void) {
    uint8_t status;
    nand_send_command(NAND_CMD_STATUS);
    FPGA_QSPI_Read(&status, 0, 1);
    return (status & NAND_STATUS_READY) && !(status & NAND_STATUS_FAIL);
}

// Инициализация MT29
bool MT29_Flash_init(mss_qspi_io_format io_format) {
    FPGA_QSPI_Configure(); // Настройка FPGA QSPI I/O для MT29

    nand_send_command(NAND_CMD_RESET); // Сброс чипа
    if (!nand_check_status()) {
        mHSS_DEBUG_PRINTF(LOG_ERROR, "MT29 reset failed\n");
        return false;
    }

    // Чтение ID для проверки
    uint8_t id[5];
    nand_send_command(NAND_CMD_READ_ID);
    nand_send_address(0, 1);
    FPGA_QSPI_Read(id, 0, 5);
    if (id[0] != 0x2C) { // Micron ID
        mHSS_DEBUG_PRINTF(LOG_ERROR, "Invalid MT29 ID: 0x%02X\n", id[0]);
        return false;
    }

    mHSS_DEBUG_PRINTF(LOG_NORMAL, "MT29 initialized\n");
    return true;
}

// Чтение страницы
bool MT29_Flash_read(uint8_t *buf, uint32_t addr, uint32_t len) {
    if (len > MT29_PAGE_SIZE) return false;

    nand_send_command(NAND_CMD_READ_PAGE);
    nand_send_address(addr, 5); // 5 циклов адреса для MT29F
    nand_send_command(NAND_CMD_READ_PAGE_CONFIRM);

    FPGA_QSPI_Read(buf, 0, len);
    return nand_check_status();
}

// Запись страницы
bool MT29_Flash_program(uint8_t *buf, uint32_t addr, uint32_t len) {
    if (len > MT29_PAGE_SIZE) return false;

    nand_send_command(NAND_CMD_PROGRAM_PAGE);
    nand_send_address(addr, 5);
    FPGA_QSPI_Write(buf, 0, len);
    nand_send_command(NAND_CMD_PROGRAM_PAGE_CONFIRM);

    return nand_check_status();
}

// Стирание блока
bool MT29_Flash_erase_block(uint32_t block_index) {
    uint32_t addr = block_index * MT29_BLOCK_SIZE;

    nand_send_command(NAND_CMD_ERASE_BLOCK);
    nand_send_address(addr, 3); // 3 цикла адреса для блока
    nand_send_command(NAND_CMD_ERASE_BLOCK_CONFIRM);

    return nand_check_status();
}