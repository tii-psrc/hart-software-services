/*
 * qspi.h
 *
 *  Created on: 9 May 2025
 *      Author: Sergio.Sirota
 */

#ifndef IOB_QSPI_H_
#define IOB_QSPI_H_

#include "ctest/sapi_hw_platform.h"

#include <stdint.h>


typedef enum mems_id_t
{
    MEM_M0          = 0,
    MEM_M1          = 1,
    MEM_M2          = 2,
    MEM_M3          = 3,
    MEM_M4          = 4,
    MEM_M5          = 5,
    MEM_M6          = 6,
    MEM_M7          = 7,
    MEM_BNOR        = 8,
    MEM_BBNOR       = 9,
    MEM_SNOR        = 10,
    MEM_BSNOR       = 11,
    MAX_QSPIS
}QSPIS_ENUM;

typedef enum
{
    Q_WR_DATA   = 0,
    Q_WR_CTRL1  = 4,
    Q_WR_CTRL2  = 8,
    Q_WR_CTRL3  = 12,
    Q_RD_DATA   = 0,
    Q_RD_ST1    = 4,
    Q_RD_ST2    = 8,
}QSPIS_REGS;

struct QSPI_st
{
    uint32_t    address;
    uint32_t    ctrl[3], old_ctrl[3];
//    QFIFO       RX;//TX,
};


typedef struct QSPI_st QSPI_type;
extern QSPI_type QSPIs[MAX_QSPIS];

typedef enum
{
    Q_IDLE          =   0x00000001,
    Q_FR_EMPTY      =   0x00000002,
    Q_FR_FULL       =   0x00000004,
    Q_FT_EMPTY      =   0x00000008,
    Q_FT_FULL       =   0x00000010,
    Q_OVERUN        =   0x00000020,
    Q_AUTO_OP_RDY   =   0x00000040,
    Q_AUTO_OP_ERROR =   0x00000080,
    Q_D_OUT         =   0x00000F00,
    Q_CLK           =   0x00001000,
    Q_CE            =   0x00002000,
    Q_RST           =   0x00004000,
    Q_G_            =   0x003F0000,
    Q_E_            =   0x3F000000,
}QSPI_STATUS1;


typedef enum
{
    Q_FR_FULL2      =   0x00000001,
    Q_FR_EMPTY2     =   0x00000002,
    Q_FR_RDCNT      =   0x000001FC,
    Q_FR_WRCNT      =   0x0000FE00,
    Q_FT_FULL2      =   0x00010000,
    Q_FT_EMPTY2     =   0x00020000,
    Q_FT_RDCNT      =   0x01FC0000,
    Q_FT_WRCNT      =   0xFE000000
}QSPI_STATUS2;

typedef enum
{
    Q_FR_S_FULL2    =   0,
    Q_FR_S_EMPTY2   =   1,
    Q_FR_S_RDCNT    =   2,
    Q_FR_S_WRCNT    =   9,
    Q_FT_S_FULL2    =   0x10,
    Q_FT_S_EMPTY2   =   0x11,
    Q_FT_S_RDCNT    =   0x12,
    Q_FT_S_WRCNT    =   0x19
}QSPI_STATUS2_SHIFT;

typedef enum
{
    Q_CTRL1_ACTIVATE_CE  =   0x00000001UL,
    Q_CTRL1SET_nWP       =   0x00000002UL,
    Q_CTRL1_SET_nRESET   =   0x00000004UL,
    Q_CTRL1_SET_WnB      =   0x00000008UL,
    Q_CTRL1_SET_X4nX1    =   0x00000010UL,
    Q_CTRL1_ANDnOR       =   0x00000020UL,
    Q_CTRL1_DIV          =   0x000001C0UL,
    Q_CTRL1_START_OP     =   0x00000200UL,
    Q_CTRL1_QT           =   0x001FFC00UL,
    Q_CTRL1_QR           =   0xFFE00000UL
}QSPI_CTRL1;

typedef enum
{
    Q_CTRL1_S_ACTIVATE_CE  =   0,
    Q_CTRL1SET_S_nWP       =   1,
    Q_CTRL1_SET_S_nRESET   =   2,
    Q_CTRL1_SET_S_WnB      =   3,
    Q_CTRL1_SET_S_X4nX1    =   4,
    Q_CTRL1_S_ANDnOR       =   5,
    Q_CTRL1_S_DIV          =   6,
    Q_CTRL1_S_START_OP     =   9,
    Q_CTRL1_S_QT           =   10,
    Q_CTRL1_S_QR           =   21
}QSPI_CTRL1_SHIFT;



typedef enum
{
    Q_CTRL2_CMD           =   0x000000FF,
    Q_CTRL2_MASK          =   0x00000F00,
    Q_CTRL2_TIMES         =   0x7FFFF000,
    Q_CTRL2_AUTO_ENABLE   =   0x80000000
}QSPI_CTRL2;

typedef enum
{
    Q_CTRL3_G_OUT          =   0x0000007F,
    Q_CTRL3_G_ENA          =   0x00003F80,
    Q_CTRL3_G_USE_GPIO     =   0x00004000,
    Q_CTRL3_G_USE_TOGGLE   =   0x00008000,
    Q_CTRL3_SET_nHOLD      =   0x00010000,
    Q_CTRL3_SET_DUMMY_CNT  =   0x003E0000,
    Q_CTRL3_S_SET_DUMMY_CNT=   17
}QSPI_CTRL3;

/*
 * Init the hardware IpCore and the software structures, including TX and RX FIFOs
 * */
void sapi_init_qspis(void);

/*
 * This routine transfer to interface "instance", "quantity" 32 bits words. It is not blocking.
 *
 * */
int nw_tx_words(int instance, uint32_t *d, int quantity);


/*
 * This routine will read the status searching for the bit 0 (Operation IDLE) to understand all the transaction was done
 */
int wait_until_qspi_idle(QSPI_type *qtr);


/*
 * This routine get qspi data:
 *      * If sFIFO has data, take from there.
 *      * If not, get data from hFIFO
 *      * One finished, if all the data were got, if more data exist in hFIFO, take them and put them in sFIFO.
 *
 * */
int nw_rx_words(int instance, uint32_t *d, int quantity);

/*
 * This routine get qspi data:
 *      * If sFIFO has data, take from there.
 *      * If not, get data from hFIFO
 *      * One finished, if all the data were got, if more data exist in hFIFO, take them and put them in sFIFO.
 *
 * */

int rx_words(int instance, uint32_t *d, int quantity);

/*
 * This routine generate dummy clocks: the cache reading have the trouble about read ever from the first byte
 *
 * */
int dummy_rx_words(int instance, int quantity);


/*
 * This routine transfer to interface "instance", "quantity" 32 bits words. It is blocking.
 *
 * */
int tx_words(int instance, uint32_t *d, int quantity);


/*
 * This routine will transmit quantity bytes : each one will be packed in 32 bits
 * It assumes previously the the ctrl[0].WnB was reset
 * It is blocking
 * */
int tx_bytes(int instance, uint8_t *d, int quantity);


/*
 * This routine will receive quantity bytes : each one coms packed in 32 bits
 * It assumes previously the the ctrl[0].WnB was reset
 * It is blocking
 * */
int rx_bytes(int instance, uint8_t *d, int quantity);

#endif /* IOB_QSPI_H_ */
