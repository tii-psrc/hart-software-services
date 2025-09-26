/*
 * qspi_memories.h
 *
 *  Created on: 15 May 2025
 *      Author: Sergio.Sirota
 */

#ifndef IOB_QSPI_MEMORIES_H_
#define IOB_QSPI_MEMORIES_H_

#include "ctest/sapi_qspi.h"
#include "ctest/sapi_hw_platform.h"

#include <hw_reg_access.h>

    typedef enum
    {
        MICRON_MT29F            = 0,
        MICRON_MT25Q            = 1,
        WINBOND_W25N01          = 2,
//        MSS_WINBOND_W25N01      = 3,
        MEM_TYPES_QUANTITY
    }qspi_mems_t;

    typedef enum
    {
        MEM_ID              = 0,
        MEM_READ_PAGE       = 1,
        MEM_WRITE_ENABLE    = 2,
        MEM_WRITE_DISABLE   = 3,
        MEM_RESET           = 4,
        MEM_GET_FEAT        = 5,
        MEM_SET_FEAT        = 6,
        MEM_CMDS_QUANTITY   = 7
    }qspi_cmds_t;

    typedef enum
    {
        QSPI_ACTIVATE_CE,
        QSPI_DEACTIVATE_CE,
        QSPI_MAX_OPS
    }qspi_ops;

    typedef enum
    {
        MEM_CMDS_W_ENA  = 0x06,
        MEM_CMDS_W_DIS  = 0x04
    }mem_cmds_enum;

    typedef enum
    {
        MEM_WRITE_PROTECTED = 0,
        MEM_NOT_WRITE_PROTECTED = 1
    }mem_protection_enum;

    struct mqspi_struct
    {
        int             mem_type;
        int             die;    //Valid when applies. Example MT29F
        QSPI_type       *qtr;
    };

    typedef struct mqspi_struct MQSPI_T;


    extern MQSPI_T MQSPIs[MAX_QSPIS];

    /*
     * This routine initialize the connection between the qspi data struct and the physical memories.
     *
     * */
    void init_mqspis(void);

    /*
     * This routine shall activate or deactivate the CE of a QSPI memory with a controller in qtr
     * */
    void activate_ce(QSPI_type *qtr,  int value);
    int generic_tx_rx_32bits(int instance, int X4nX1, uint32_t *tx_d, int tx_q, uint32_t *rx_d, int rx_q, int dummy_cycles);
    int generic_tx_rx_8bits(int instance, int X4nX1, uint8_t *tx_d, int tx_q, uint8_t *rx_d, int rx_q, int activate_ce);
    int generic_dummy_rx_32bits(int instance, int X4nX1, int rx_q);
    int generic_tx_8bits_x1(int instance, uint8_t *tx_d, int tx_q, int act_ce);
    int mem_get_id(int instance, uint8_t *id);
    int mem_program_execute(int instance, int address);
    int mem_w_ena(int instance);
    int set_WP(int instance, int protected);
    int mem_w_dis(int instance);
#endif /* IOB_QSPI_MEMORIES_H_ */
