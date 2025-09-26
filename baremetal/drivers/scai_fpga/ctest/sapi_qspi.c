/*
 * qspi.c
 *
 *  Created on: 9 May 2025
 *      Author: Sergio.Sirota
 */

#include "ctest/sapi_qspi.h"
#include "ctest/sapi_hw_platform.h"
#include "ctest/reg_mitm.h"

#include <string.h>
#include <stdio.h>

QSPI_type QSPIs[MAX_QSPIS];

static void init_qspi(int instance, uint32_t address)
{
    QSPI_type *qtr;

    qtr = &QSPIs[instance];
    qtr->address = address;
    qtr->ctrl[0] = Q_CTRL1_SET_nRESET;// | (1<<Q_CTRL1_S_DIV);
    qtr->ctrl[1] = 0;   //No auto operation
    qtr->ctrl[2] = Q_CTRL3_SET_nHOLD;   //No Toggling or G_PIO operation. And not hold

    scai_set_reg(qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);
    scai_set_reg(qtr->address + Q_WR_CTRL2, qtr->ctrl[1]);
    scai_set_reg(qtr->address + Q_WR_CTRL3, qtr->ctrl[2]);
}

void sapi_init_qspis(void)
{
    init_qspi(MEM_M0, M0_BASE_ADDRESS);
    init_qspi(MEM_M1, M1_BASE_ADDRESS);
    init_qspi(MEM_M2, M2_BASE_ADDRESS);
    init_qspi(MEM_M3, M3_BASE_ADDRESS);
    init_qspi(MEM_M4, M4_BASE_ADDRESS);
    init_qspi(MEM_M5, M5_BASE_ADDRESS);
    init_qspi(MEM_M6, M6_BASE_ADDRESS);
    init_qspi(MEM_M7, M7_BASE_ADDRESS);
    init_qspi(MEM_BNOR, BNOR_BASE_ADDRESS);
    init_qspi(MEM_BBNOR, BacBNOR_BASE_ADDRESS);
    init_qspi(MEM_SNOR, SNOR_BASE_ADDRESS);
    init_qspi(MEM_BSNOR, BacSNOR_BASE_ADDRESS);
}

/*
 * This routine will read the status searching for the bit 0 (Operation IDLE) to understand all the transaction was done
 */
int wait_until_qspi_idle(QSPI_type *qtr)
{
    int i;

    for(i=0;i<100;i++)
    {
        if(1 & scai_get_reg(qtr->address + Q_RD_ST1))
            break;
    }
    return (i<100);
}

int rx_words(int instance, uint32_t *d, int quantity)
{
    int i = 0, qty, j=0;
    QSPI_type *qtr;
    uint32_t status;

    qtr = &QSPIs[instance];

    status = scai_get_reg(qtr->address + Q_RD_ST2);
    qty = ((status & Q_FR_RDCNT)>>Q_FR_S_RDCNT);

    while(1)
    {
        j = 0;
        while(j<qty)
        {
            j++;
            d[i++] = scai_get_reg(qtr->address + Q_RD_DATA);
            if((i)>=quantity)
                break;
        }
        if(i>=quantity)
            break;
        status = scai_get_reg(qtr->address + Q_RD_ST2);
        qty = ((status & Q_FR_RDCNT)>>Q_FR_S_RDCNT);
    }
    return i;
}

int tx_words(int instance, uint32_t *d, int quantity)
{
    int i=0;
    QSPI_type *qtr;
    uint32_t status;
    int qty;
    uint32_t *ptr;

    qtr = &QSPIs[instance];
    ptr = d;
    while(i<quantity)
    {
        status = scai_get_reg(qtr->address + Q_RD_ST2);
        if(!(status & Q_FT_FULL))
        {
            qty = 64-((status & Q_FT_WRCNT)>>Q_FT_S_WRCNT);
            while(qty && (i<quantity))
            {
                scai_set_reg(qtr->address + Q_WR_DATA, *ptr++);
                i++;
                qty--;
            }
        }
    }
    return i;
}

int tx_bytes(int instance, uint8_t *d, int quantity)
{
    int i=0;
    QSPI_type *qtr;
    uint32_t status, data;
    uint8_t *ptr;
    int qty;

    qtr = &QSPIs[instance];
    ptr = d;
    while(i<quantity)
    {
        status = scai_get_reg(qtr->address + Q_RD_ST2);
        if(!(status & Q_FT_FULL))
        {
            qty = 64-((status & Q_FT_WRCNT)>>Q_FT_S_WRCNT);
            while(qty && (i<quantity))
            {
                data = (((uint32_t)ptr[0])<<24) | 0x00FFFFFF;
                ptr++;
                scai_set_reg(qtr->address + Q_WR_DATA, data);
                i++;
                qty--;
            }
        }
    }
    return i;
}

/*
 * This routine will receive quantity bytes : each one coms packed in 32 bits
 * It assumes previously the the ctrl[0].WnB was reset
 * It is blocking
 * */
int rx_bytes(int instance, uint8_t *d, int quantity)
{
    int i=0, j, max_j;
    uint32_t w[16];
    uint8_t *ptr;
    ptr = d;

    while(i<quantity)
    {
        max_j = (quantity-i);
        if(max_j>16)
            max_j = 16;
        i += max_j;
        rx_words(instance, w, max_j);

        for(j=0; j < max_j; j++, ptr++)  //I will create a buffer up to 16 words and I will call tx_words
            *ptr = (uint8_t)(w[j]);
    }
    return i;
}

