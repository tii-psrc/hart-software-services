/*
 * MT29F.h
 *
 *  Created on: 10 Jun 2025
 *      Author: Sergio.Sirota
 */

#ifndef IOB_MT29F_H_
#define IOB_MT29F_H_

    #define MT29F_BLOCK_LOCK_REG    0xA0
    #define MT29F_BLOCK_CONFIG_REG  0xB0
    #define MT29F_BLOCK_STATUS_REG  0xC0
    #define MT29F_BLOCK_DIE_SEL_REG 0xD0

    #define PROGRAM_LOAD_X4         0x32
    #define READ_FROM_CACHE_X4      0xEB

#define  LOGICAL_2_PHY_ADDRESS_MT29F(addr_in) ((addr_in & 0xFFFFF000UL)<<1) | (addr_in & 0x00000FFFUL)

union MT29F_Address
{
    uint32_t address;
    struct
    {
        uint32_t c_address : 13; //Column address
        uint32_t p_address : 6;  //page address
        uint32_t b_address : 11; //Block Address
        uint32_t d_address : 1;  //Die Number
        uint32_t spare     : 3;
    }st;
    struct
    {
        uint32_t c_address : 13; //Column address
        uint32_t r_address : 18; //Row Address
        uint32_t spare     : 3;
    }st_cr;

    struct
    {
        uint32_t c_address : 13; //Column address
        uint32_t r_address : 17; //Row Address
        uint32_t spare     : 4;
    }st_cr_no_die;
};


int sapi_init_mt29fs(void);
//int get_feature_mt29f(int instance, int feat, uint8_t *id);
//int get_features_mt29f(int instance, int feat, uint8_t *id);
//int set_feature_mt29f(int instance, int feat, uint8_t value);
int unlock_mt29f(int instance);
int mem_read_mt29f_x1(int instance, uint32_t addr, uint8_t *data, int quantity);
int mem_read_mt29f_x4(int instance, uint32_t addr, uint32_t *data, int quantity);
int mem_write_mt29f_x4(int instance, uint32_t addr, uint32_t *data, int quantity);
int block_erase_mt29f(int instance, uint32_t full_addr);
void mt29f_test(void);

#endif /* IOB_MT29F_H_ */


