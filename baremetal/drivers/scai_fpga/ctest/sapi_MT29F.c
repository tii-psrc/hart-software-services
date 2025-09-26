/*
 * MT29F.c
 *
 *  Created on: 10 Jun 2025
 *      Author: Sergio.Sirota
 */


/*
 * MT29F.c
 *
 *  Created on: 12 Oct 2024
 *      Author: Sergio.Sirota
 */

#include "ctest/sapi_qspi_memories.h"
#include "ctest/sapi_MT29F.h"

#include <string.h>
#include "hss_debug.h"


/*
 * The memory organization is:
 * 1 Page  = 4096 Bytes + 256 Bytes for metadata / ECC
 * 1 Block = 64 Pages
 * 1 Plane = 2048 Blocks
 * 1 Device= 2 planes
 *                            <----        Row Address      ----> <---Column Address--->
 * Logical Address : D(die) + BBBBBBBBBBB(11 bits) PPPPPP(6 bits) IIIIIIIIIIIII(13 bits)
 */

static int get_feature_mt29f(int instance, int feat, uint8_t *id)
{
    uint8_t tx_d[4];

    tx_d[0] = 0x0F;
    tx_d[1] = (uint8_t)feat;
    return generic_tx_rx_8bits(instance, 0, tx_d, 2, id, 1, QSPI_ACTIVATE_CE);
}

/*
static int get_features_mt29f(int instance, int feat, uint8_t *id) {
    return get_feature_mt29f(instance, MT29F_BLOCK_LOCK_REG, (uint8_t *)&id[0]) &&
            get_feature_mt29f(instance, MT29F_BLOCK_CONFIG_REG, (uint8_t *)&id[1]) &&
            get_feature_mt29f(instance, MT29F_BLOCK_STATUS_REG, (uint8_t *)&id[2]) &&
            get_feature_mt29f(instance, MT29F_BLOCK_DIE_SEL_REG, (uint8_t *)&id[3]);
}
*/

static int set_feature_mt29f(int instance, int feat, uint8_t value)
{
    uint8_t tx_d[4];

    tx_d[0] = 0x1F;
    tx_d[1] = (uint8_t)feat;
    tx_d[2] = value;
    return generic_tx_8bits_x1(instance, tx_d, 3, QSPI_ACTIVATE_CE);
}

int unlock_mt29f(int instance)
{
    uint8_t tx_d[4];

    tx_d[0] = 0x1F;
    tx_d[1] = 0xA0;
    tx_d[2] = 0x00;
    return generic_tx_8bits_x1(instance, tx_d, 3, QSPI_ACTIVATE_CE);
}

static int mt29f_set_die(int instance, int die)
{
    set_feature_mt29f(instance, MT29F_BLOCK_DIE_SEL_REG, (die)?0x40:0);
    return 1;
}

static int mt29f_init(int instance)
{
    //This command will enable continue reading

    uint8_t data;

    get_feature_mt29f(instance, MT29F_BLOCK_CONFIG_REG, &data);
    data |= 1; //turn on CONTI_RD bit
    set_feature_mt29f(instance, MT29F_BLOCK_CONFIG_REG, data);
    unlock_mt29f(instance);
    return 1;
}

int sapi_init_mt29fs(void)
{
    int ret = 0;

    for (int i=0;i<MAX_QSPIS;i++)
    {
        if(MQSPIs[i].mem_type == MICRON_MT29F)
            ret += mt29f_init(i);
    }
    return ret;
}

static int mem_wait_for_op_ready_mt29f(int instance)
{
    int i=1, j;
    uint8_t data;

    for(j=0; (j<100) && i; j++)
    {
        i = get_feature_mt29f(instance, MT29F_BLOCK_STATUS_REG, &data);
        if(i)
        {
            if(!(data & 1))// When bit OIP (Operation In progress) is 0, we can proceed
                break;
        }
    }
    return i && (j<100);
}

static int mem_to_cache_mt29f(int instance, int addr)
{
    uint8_t tx_d[4];

    tx_d[0] = 0x13;
    tx_d[1] = (0x00010000UL & addr)?1:0;
    tx_d[2] = (uint8_t)((0x0000FF00UL & addr)>>8);
    tx_d[3] = (uint8_t)((0x000000FFUL & addr));
    return generic_tx_8bits_x1(instance, tx_d, 4, QSPI_ACTIVATE_CE) && mem_wait_for_op_ready_mt29f(instance);
}

static int read_from_cache_mt29f_x4(int instance, int addr, uint32_t *data, int quantity)
{
    MQSPI_T *mqtr;
    QSPI_type *qtr;
    int i = -1;
    uint8_t tx_dx1[4];

    mqtr = &MQSPIs[instance];
    qtr  = mqtr->qtr;
    activate_ce(qtr, QSPI_ACTIVATE_CE);
    //I will read X4, and I will get the info x32bits. First, send the command 0xEB x1
    tx_dx1[0] = 0x6B;
    tx_dx1[1] = (uint8_t)((addr & 0xFF00)>>8);
    tx_dx1[2] = (uint8_t)(addr & 0xFF);
    tx_dx1[3] = 0;
    generic_tx_8bits_x1(instance, tx_dx1, 4, QSPI_DEACTIVATE_CE);
    generic_tx_rx_32bits(instance, 1, data, 0, data, addr>>2, 0);
    i = generic_tx_rx_32bits(instance, 1, data, 0, data, quantity, 0);
    activate_ce(qtr, QSPI_DEACTIVATE_CE);
    return i;
}


/*int read_from_cache_mt29f_x4(int instance, int addr, uint32_t *data, int quantity)
{
    union qspi_u *q;
    int i = 0;

    M29_cmd.cmd[0]          = (uint8_t) ((0x1F00UL & addr)>>8);
    M29_cmd.cmd[1]          = (uint8_t)((0x00FFUL & addr));
    M29_cmd.cmd[2]          = 0;
    M29_cmd.cmd[3]          = 0;

    to_init(TO_ID_QSPI_CTRL_READY);
    q = QSPI[instance].regs;
    i = 0;
    while(!CONTROLLER_READY(q))
    {
        i=to_ready(TO_ID_QSPI_CTRL_READY);
        if(i)
            break;
    }
    if(!i)
    {
         q->reg.FRAMESUP.BYTESUPPER=0;
         q->reg.FRAMES.QSPI = 1;
         q->reg.FRAMES.FLAGWORD = 1;
         q->reg.FRAMES.CMDBYTES   = 5;
         q->reg.FRAMES.TOTALBYTES = 5 + 4*quantity;
         i = tx_byte(q, READ_FROM_CACHE_X4) &&  tx_word(q, *(uint32_t *)M29_cmd.cmd) && rx_words(q, data, quantity);
    }
    return i;
}*/

static int program_load_mt29f_x4(int instance, int addr, uint32_t *data, int quantity)
{
    uint8_t cmd[3];
    MQSPI_T *mqtr;
    QSPI_type *qtr;
    int i = 1;

    mqtr = &MQSPIs[instance];
    qtr  = mqtr->qtr;
    activate_ce(qtr, QSPI_ACTIVATE_CE);

    cmd[0] = PROGRAM_LOAD_X4;
    cmd[1] = (uint8_t)((0x0000FF00UL & addr)>>8);
    cmd[2] = (uint8_t)(0xFFUL & addr);
    i = generic_tx_8bits_x1(instance, cmd, 3, QSPI_DEACTIVATE_CE) && generic_tx_rx_32bits(instance, 1, data, quantity, data, 0, 0);

    activate_ce(qtr, QSPI_DEACTIVATE_CE);

    return i;
}

int mem_read_mt29f_x4(int instance, uint32_t addr, uint32_t *data, int quantity)
{
    int i=1;
    int      chunk;
    uint32_t *l_data;
    int      max_chunk, byte_qty;
    union MT29F_Address mta;
    /*
     *
     *I must read chunks based on physical pages on boundaries of 0x1000 :
     *      a page x 4096 : 0 to FFF
     *      if address & 0xFFF + quantity <= 4096: only a reading from address
     *      Example
     *       ---------------------------------------------------------------------------------------------------------
     *      | Address     |  Quantity   |             First Reading            |             Next Reading             |
     *      |             |             | Block    Offset   address  Quantity  | Block    Offset   address  Quantity  |
     *       ---------------------------------------------------------------------------------------------------------
     *      | 0x79123     |  100        | 0x79     0x123    0x79123  100       |                                      |
     *      | 0x79123     |  1100       | 0x79     0x123    0x79123  3805      | 0x7A    0x0       0x7a000  547       |
     *      | 0x79FFF     |  1000       | 0x79     0xFFF    0x79123  1         | 0x7A    0x0       0x7a000  4095      |
     *
     *
     *       First reading : from addr to next boundary address to (upLimit = address | 0xFFF) Quantity = (address + 0x1000 ) & 0xFFF  + 1
     */

    i            = 1;
    mta.address  = ((LOGICAL_2_PHY_ADDRESS_MT29F(addr))>>2)<<2;
    byte_qty     = quantity<<2;
    max_chunk = 0x1000;

    chunk        = (max_chunk- mta.st.c_address);//pages of 0x1100 bytes in MT29F. chunk in bytes
    l_data       = data;
    if (chunk> byte_qty)
        chunk = byte_qty;
    while((byte_qty>0) && i)
    {
        if(mta.st.d_address != MQSPIs[instance].die)
        {
            mt29f_set_die(instance, mta.st.d_address);
            MQSPIs[instance].die = mta.st.d_address;
        }

        i = mem_to_cache_mt29f(instance, mta.st_cr.r_address);
        if(!i)
            break;

        i = read_from_cache_mt29f_x4(instance, mta.st.c_address, l_data, chunk>>2);
        if(i)
        {
            mta.st_cr.r_address++;
            mta.st_cr.c_address = 0;
            byte_qty  -= chunk;

            l_data      += (chunk>>2);
            if (byte_qty > max_chunk)
                chunk = max_chunk;
            else
                chunk = byte_qty;
        }
    }
    return i;
}


int mem_write_mt29f_x4(int instance, uint32_t addr, uint32_t *data, int quantity)
{
    int      chunk;
    uint32_t *l_data;
    int      max_chunk, byte_qty;
    union MT29F_Address mta;

    /*
     *
     *I must read chunks based on physical pages on boundaries of 0x1000 :
     *      a page x 4096 : 0 to FFF
     *      if address & 0xFFF + quantity <= 4096: only a reading from address
     *      Example
     *       ---------------------------------------------------------------------------------------------------------
     *      | Address     |  Quantity   |             First Reading            |             Next Reading             |
     *      |             |             | Block    Offset   address  Quantity  | Block    Offset   address  Quantity  |
     *       ---------------------------------------------------------------------------------------------------------
     *      | 0x79123     |  100        | 0x79     0x123    0x79123  100       |                                      |
     *      | 0x79123     |  1100       | 0x79     0x123    0x79123  3805      | 0x7A    0x0       0x7a000  547       |
     *      | 0x79FFF     |  1000       | 0x79     0xFFF    0x79123  1         | 0x7A    0x0       0x7a000  4095      |
     *
     *
     *       First reading : from addr to next boundary address to (upLimit = address | 0xFFF) Quantity = (address + 0x1000 ) & 0xFFF  + 1
     */

    mta.address  = ((LOGICAL_2_PHY_ADDRESS_MT29F(addr))>>2)<<2;
    byte_qty     = quantity<<2;
    max_chunk = 0x1000;

    chunk        = (max_chunk- mta.st.c_address);//pages of 0x1100 bytes in MT29F. chunk in bytes
    l_data       = data;
    if (chunk> byte_qty)
        chunk = byte_qty;
    while(byte_qty>0)
    {
        if(mta.st.d_address != MQSPIs[instance].die)
        {
            mt29f_set_die(instance, mta.st.d_address);
            MQSPIs[instance].die = mta.st.d_address;
        }
        set_WP(instance, MEM_NOT_WRITE_PROTECTED);
        mem_w_ena(instance);
        program_load_mt29f_x4(instance, mta.st.c_address, l_data, chunk>>2);
        mem_program_execute(instance, mta.st_cr.r_address);
        mem_wait_for_op_ready_mt29f(instance);
        set_WP(instance, MEM_WRITE_PROTECTED);
        mta.st_cr.r_address++;
        mta.st.c_address    = 0;
        byte_qty  -= chunk;

        l_data      += (chunk>>2);
        if (byte_qty > max_chunk)
            chunk = max_chunk;
        else
            chunk = byte_qty;
    }
    return 1;
}

/*
 * This routine will erase a block of MT29F
 * The memory organization is:
 * 1 Page  = 4096 Bytes + 256 Bytes for metadata / ECC
 * 1 Block = 64 Pages
 * 1 Plane = 2048 Blocks
 * 1 Device= 2 planes
 *                            <----        Row Address      ----> <---Column Address--->
 * Logical Address : D(die) + BBBBBBBBBBB(11 bits) PPPPPP(6 bits) IIIIIIIIIIIII(13 bits)
 */
int block_erase_mt29f(int instance, uint32_t full_addr)
{
    int i;
    union MT29F_Address mta;
    uint8_t cmd[4];
    uint32_t b_addr;

    mta.address  = ((LOGICAL_2_PHY_ADDRESS_MT29F(full_addr))>>2)<<2;
    if(mta.st.d_address != MQSPIs[instance].die)
    {
        mt29f_set_die(instance, mta.st.d_address);
        MQSPIs[instance].die = mta.st.d_address;
    }
    cmd[0] = 0xD8;
    b_addr = mta.st_cr.r_address;
    cmd[1] = (uint8_t)((0xFF0000 & b_addr)>>16);
    cmd[2] = (uint8_t)((0x00FF00 & b_addr)>>8);
    cmd[3] = (uint8_t)((0x0000FF & b_addr));
    i =     mem_w_ena(instance) &&
            generic_tx_8bits_x1(instance, cmd, 4, QSPI_ACTIVATE_CE) &&
            mem_wait_for_op_ready_mt29f(instance);
    return i;
}

static uint32_t data[0x900];


void mt29f_test(void)
{
    int i, j, k;
    uint8_t id[5];
    int err = 0;
    union MT29F_Address mta;

    k = 0;

    mta.address = 0x1890A020UL;

    memset((uint8_t *)data, 0xBEBECAFE, 384);
    for (i=0;i<64;i++)
        data[i]=i+0x12345600ul;

    for (i=0; i<8;i++)
    {
        memset(id, 0, sizeof(id));
        mem_get_id(i, id);
        mem_write_mt29f_x4(i, mta.address , data, 64);
        mem_read_mt29f_x4 (i, mta.address , data+0x100, 64);
        block_erase_mt29f(i, mta.address);
        mem_read_mt29f_x4 (i, mta.address, data+0200, 64);

        for (j=0, err = 0;j<64;j++)
        {
            if(data[j]!=data[j+0x100])
            {
                err++;
                mHSS_DEBUG_PRINTF(LOG_ERROR, "(%08X-%08X)\r\n", data[j], data[j+0x100]);
            }
        }
        mHSS_DEBUG_PRINTF(LOG_ERROR, "mem[%d]: %d errors - k=%d\r\n", i, err, k);
        for (j=0, err = 0; j<64; j++)
        {
            if(0xFFFFFFFF != data[j+0x200])
            {
                err++;
                mHSS_DEBUG_PRINTF(LOG_ERROR, "j(%d):%08X\r\n", j, data[j+0x200]);
            }
        }
        mHSS_DEBUG_PRINTF(LOG_ERROR, "mem[%d]: %d errors - k=%d\r\n", i, err);
        break;
    }
}





