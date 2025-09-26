/*
 * qspi_memories.c
 *
 *  Created on: 15 May 2025
 *      Author: Sergio.Sirota
 */

#include "ctest/sapi_hw_platform.h"
#include "ctest/sapi_qspi_memories.h"


MQSPI_T MQSPIs[MAX_QSPIS];

void init_mqspis(void)
{
    int i;

    for(i=0;i<MAX_QSPIS; i++)
        MQSPIs[i].die = 0;
    MQSPIs[MEM_M0].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M0].qtr = &QSPIs[MEM_M0];
    MQSPIs[MEM_M1].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M1].qtr = &QSPIs[MEM_M1];
    MQSPIs[MEM_M2].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M2].qtr = &QSPIs[MEM_M2];
    MQSPIs[MEM_M3].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M3].qtr = &QSPIs[MEM_M3];
    MQSPIs[MEM_M4].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M4].qtr = &QSPIs[MEM_M4];
    MQSPIs[MEM_M5].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M5].qtr = &QSPIs[MEM_M5];
    MQSPIs[MEM_M6].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M6].qtr = &QSPIs[MEM_M6];
    MQSPIs[MEM_M7].mem_type = MICRON_MT29F;
    MQSPIs[MEM_M7].qtr = &QSPIs[MEM_M7];

    MQSPIs[MEM_BNOR].mem_type = WINBOND_W25N01;
    MQSPIs[MEM_BNOR].qtr = &QSPIs[MEM_BNOR];
    MQSPIs[MEM_BBNOR].mem_type = WINBOND_W25N01;
    MQSPIs[MEM_BBNOR].qtr = &QSPIs[MEM_BBNOR];

    MQSPIs[MEM_SNOR].mem_type = MICRON_MT25Q;
    MQSPIs[MEM_SNOR].qtr = &QSPIs[MEM_SNOR];
    MQSPIs[MEM_BSNOR].mem_type = MICRON_MT25Q;
    MQSPIs[MEM_BSNOR].qtr = &QSPIs[MEM_BSNOR];
}

/*
 * This routine shall activate or deactivate the CE of a QSPI memory
 * */
void activate_ce(QSPI_type *qtr,  int value)
{
    if(value == QSPI_ACTIVATE_CE)
        qtr->ctrl[0] |= Q_CTRL1_ACTIVATE_CE;
    else
        qtr->ctrl[0] &= ~Q_CTRL1_ACTIVATE_CE;
    HW_set_32bit_reg(qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);
}

/*
 * This routine will send 32 bits words, and they shall be transmitted in 4 bits format o standard spi format
 * X4nX1 = 1: transmit / receive in full qspi
 * X4nX1 = 0: transmit / receive in standard spi
 *
 * */
int generic_tx_rx_32bits(int instance, int X4nX1, uint32_t *tx_d, int tx_q, uint32_t *rx_d, int rx_q, int dummy_cycles)
{
     MQSPI_T *mqtr;
     QSPI_type *qtr;
     int ret = -1;

     mqtr = &MQSPIs[instance];
     qtr  = mqtr->qtr;

     //1-Program Dummy Cycles, if is necessary
     if(dummy_cycles)
     {
         qtr->ctrl[2] &=  ~(Q_CTRL3_SET_DUMMY_CNT);
         qtr->ctrl[2] |=  (((uint32_t)dummy_cycles)<<Q_CTRL3_S_SET_DUMMY_CNT);
         HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL3, qtr->ctrl[2]);

     }
     //2-Program the control register
     qtr->ctrl[0] &= ~(Q_CTRL1_QT | Q_CTRL1_QR);
     if(X4nX1)
         qtr->ctrl[0] |=  (Q_CTRL1_SET_X4nX1);
     else
         qtr->ctrl[0] &=  ~(Q_CTRL1_SET_X4nX1);

     qtr->ctrl[0] |=  (Q_CTRL1_SET_WnB | Q_CTRL1_START_OP);
     qtr->ctrl[0] |=  (((uint32_t)tx_q)<<Q_CTRL1_S_QT);
     qtr->ctrl[0] |=  (((uint32_t)rx_q)<<Q_CTRL1_S_QR);
     HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);

     if(tx_q)
         tx_words(instance, tx_d, tx_q);
     if(rx_q)
         rx_words(instance, rx_d, rx_q);
     wait_until_qspi_idle(qtr);
     ret = 1;
     if(dummy_cycles)
     {
         qtr->ctrl[2] &=  ~(Q_CTRL3_SET_DUMMY_CNT);
         HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL3, qtr->ctrl[2]);
     }
     qtr->ctrl[0] &= ~(Q_CTRL1_QT | Q_CTRL1_QR);
     HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);
     return ret;
}

/*
 * This routine will send 32 bits words, and they shall be transmitted in 4 bits format o standard spi format
 * X4nX1 = 1: transmit / receive in full qspi
 * X4nX1 = 0: transmit / receive in standard spi
 *
 * */

int generic_tx_rx_8bits(int instance, int X4nX1, uint8_t *tx_d, int tx_q, uint8_t *rx_d, int rx_q, int act_ce)
{
     MQSPI_T *mqtr;
     QSPI_type *qtr;

     mqtr = &MQSPIs[instance];
     qtr  = mqtr->qtr;
     //1-Program the control register
     if(X4nX1)
         qtr->ctrl[0] |=  (Q_CTRL1_SET_X4nX1);
     else
         qtr->ctrl[0] &=  ~(Q_CTRL1_SET_X4nX1);
     qtr->ctrl[0] &=  ~(Q_CTRL1_SET_WnB | Q_CTRL1_QT | Q_CTRL1_QR);
     qtr->ctrl[0] |=  (Q_CTRL1_START_OP | (((uint32_t)tx_q)<<Q_CTRL1_S_QT) | (((uint32_t)rx_q)<<Q_CTRL1_S_QR));
     HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);

     if(act_ce==QSPI_ACTIVATE_CE)
         activate_ce(qtr,  QSPI_ACTIVATE_CE);
     if(tx_q)
         tx_bytes(instance, tx_d, tx_q);
     if(rx_q)
         rx_bytes(instance, rx_d, rx_q);

     wait_until_qspi_idle(qtr);
     //Close the operations
     qtr->ctrl[0] &= ~(Q_CTRL1_QT | Q_CTRL1_QR | Q_CTRL1_START_OP) ;
     HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);
     if(act_ce==QSPI_ACTIVATE_CE)  //If I asked to activate the CE, now is moment to deactivate it
         activate_ce(qtr,  QSPI_DEACTIVATE_CE);
     return 1;
}

int generic_tx_8bits_x1(int instance, uint8_t *tx_d, int tx_q, int act_ce)
{
     MQSPI_T *mqtr;
     QSPI_type *qtr;

     mqtr = &MQSPIs[instance];
     qtr  = mqtr->qtr;
     //1-Program the control register
     qtr->ctrl[0] &=  ~(Q_CTRL1_QT | Q_CTRL1_QR);
     qtr->ctrl[0] &=  ~(Q_CTRL1_SET_X4nX1);
     qtr->ctrl[0] &=  ~(Q_CTRL1_SET_WnB);
     qtr->ctrl[0] |=  (Q_CTRL1_START_OP);
     qtr->ctrl[0] |=  (((uint32_t)tx_q)<<Q_CTRL1_S_QT);
     HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);

     if(act_ce==QSPI_ACTIVATE_CE)
         activate_ce(qtr,  QSPI_ACTIVATE_CE);
     tx_bytes(instance, tx_d, tx_q);
     wait_until_qspi_idle(qtr);
     //Close the operations
     if(act_ce==QSPI_ACTIVATE_CE)  //If I asked to activate the CE, now is moment to deactivate it
         qtr->ctrl[0] &= ~(Q_CTRL1_QT | Q_CTRL1_QR | Q_CTRL1_START_OP | Q_CTRL1_ACTIVATE_CE) ;
     else
         qtr->ctrl[0] &= ~(Q_CTRL1_QT | Q_CTRL1_QR | Q_CTRL1_START_OP) ;
     HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);
     return 1;
}


int mem_get_id(int instance, uint8_t *id)
{
    uint8_t tx_d[8];
    int i = -1;

    switch(MQSPIs[instance].mem_type)
    {
         case MICRON_MT29F:
             tx_d[0] = 0x9F;
             tx_d[1] = 0xFF;
             i = generic_tx_rx_8bits(instance, 0, tx_d, 2, id, 2, QSPI_ACTIVATE_CE);
             break;
         case MICRON_MT25Q:
             tx_d[0] = 0x9F;
             i = generic_tx_rx_8bits(instance, 0, tx_d, 1, id, 3, QSPI_ACTIVATE_CE);
             break;
         case WINBOND_W25N01:
             tx_d[0] = 0x9F;
             tx_d[1] = 0xFF;
             i = generic_tx_rx_8bits(instance, 0, tx_d, 2, id, 3, QSPI_ACTIVATE_CE);
             break;
         default:
             break;
     }
     return i;

 }

static int mem_one_byte_cmds(int instance, mem_cmds_enum cmd)
{
    uint8_t tx_d[4];

    tx_d[0] = cmd;
    return  generic_tx_8bits_x1(instance, tx_d, 1, QSPI_ACTIVATE_CE);
}

int mem_w_ena(int instance)
{
     return mem_one_byte_cmds(instance, MEM_CMDS_W_ENA);
}

int set_WP(int instance, int protected)
{
    MQSPI_T *mqtr;
    QSPI_type *qtr;

    mqtr = &MQSPIs[instance];
    qtr  = mqtr->qtr;
    if(protected == MEM_WRITE_PROTECTED)
        qtr->ctrl[0] &=  ~(Q_CTRL1SET_nWP);
    else
        qtr->ctrl[0] |=  (Q_CTRL1SET_nWP);
    HW_set_32bit_reg(mqtr->qtr->address + Q_WR_CTRL1, qtr->ctrl[0]);
    return 1;
}

int mem_w_dis(int instance)
{
    return mem_one_byte_cmds(instance, MEM_CMDS_W_DIS);
}

int mem_program_execute(int instance, int address)
 {
    uint8_t tx_d[8];

    switch(MQSPIs[instance].mem_type)
     {
         case MICRON_MT29F:
             tx_d[0] = 0x10;
             tx_d[1] = (uint8_t)((address & 0xFF0000)>>16);
             tx_d[2] = (uint8_t)((address & 0x00FF00)>>8 );
             tx_d[3] = (uint8_t)( address & 0x0000FF);
             generic_tx_8bits_x1(instance, tx_d, 4, QSPI_ACTIVATE_CE);
             break;
         case MICRON_MT25Q:
             break;
         case WINBOND_W25N01:
             tx_d[0] = 0x10;
             tx_d[1] = 0;
             tx_d[2] = (uint8_t)((address & 0x00FF00)>>8 );
             tx_d[3] = (uint8_t)( address & 0x0000FF);
             generic_tx_8bits_x1(instance, tx_d, 4, QSPI_ACTIVATE_CE);
             break;
         default:
             break;
     }
     return 1;
 }
