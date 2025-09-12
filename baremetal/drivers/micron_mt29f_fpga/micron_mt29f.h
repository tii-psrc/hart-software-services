#ifndef MICRON_MT29_H
#define MICRON_MT29_H

#include "mss_qspi.h"

bool MT29_Flash_init(mss_qspi_io_format io_format);
bool MT29_Flash_read(uint8_t *buf, uint32_t addr, uint32_t len);
bool MT29_Flash_program(uint8_t *buf, uint32_t addr, uint32_t len);
bool MT29_Flash_erase_block(uint32_t block_index);

#endif