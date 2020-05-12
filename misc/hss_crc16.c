/*******************************************************************************
 * Copyright 2019 Microchip Corporation.
 *
 * SPDX-License-Identifier: MIT
 * 
 * MPFS HSS Embedded Software
 *
 */

/**
 * \file CRC16 calculation
 * \brief CRC16 calculation
 */

#include "config.h"
#include "hss_types.h"
#include <stdint.h>
#include <stdlib.h>

#include "hss_debug.h"
#include "hss_crc16.h"

uint16_t CRC16_calculate(const uint8_t *input, size_t numBytes)
{
    uint16_t result = 0u;
    int i;

    while (numBytes--) {
        result = result ^ (*input << 8);
        input++;

        for (i = 0; i < 8; i++) {
            result = result << 1;
            if (result & 0x8000u) {
                result ^= 0x1021u;
            }
        }
    }

    return result;
}