/*******************************************************************************
 * Copyright 2019-2025 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * MPFS HSS Embedded Software
 *
 */

/*!
 * \file Health Monitor State Machine
 * \brief E51-Assisted Health Monitor
 */

#include "config.h"
#include "hss_types.h"
#include "healthmon_service.h"

const struct HealthMonitor monitors[] =
{
    { "SYSREG:FABRIC_RESET_CR",         0x20002010, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1u,      healthmon_nop_trigger, 5u },
    { "SYSREG:BOOT_FAIL_CR",            0x20002014, NOT_EQUAL_TO_VALUE, 0u,      0u, 0u,  1u,       healthmon_nop_trigger, 5u },
    { "SYSREG:MSS_RESET_CR",            0x20002018, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFu,   healthmon_nop_trigger, 5u },
    { "SYSREG:CONFIG_LOCK_CR",          0x2000201c, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1u,      healthmon_nop_trigger, 5u },
    { "SYSREG:RESET_SR",                0x20002020, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1FFu,    healthmon_nop_trigger, 5u },
    { "SYSREG:DEVICE_STATUS",           0x20002024, NOT_EQUAL_TO_VALUE, 0x1F09u, 0u, 0u,  0x1FFFu,   healthmon_nop_trigger, 5u },
    { "SYSREG:APBBUS_CR",               0x20002080, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFu, healthmon_nop_trigger, 5u },
    { "SYSREG:SUBBLK_CLOCK_CR",         0x20002084, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3FFFFFFFu, healthmon_nop_trigger, 5u },
    { "SYSREG:SOFT_RESET_CR",           0x20002088, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3FFFFFFFu, healthmon_nop_trigger, 5u },
    { "SYSREG:AHBAXI_CR",               0x2000208c, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFu,   healthmon_nop_trigger, 5u },
    { "SYSREG:AHBAPB_CR",               0x20002090, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3u,      healthmon_nop_trigger, 5u },
    { "SYSREG:DFIAPB_CR",               0x20002098, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3u,      healthmon_nop_trigger, 5u },
    { "SYSREG:GPIO_CR",                 0x2000209c, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFF7733u, healthmon_nop_trigger, 5u },
    { "SYSREG:MESH_CR",                 0x200020b0, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3011FFFu, healthmon_nop_trigger, 5u },
    { "SYSREG:MESH_SEED_CR",            0x200020b4, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFF7FFFFFu, healthmon_nop_trigger, 5u },
    { "SYSREG:ENVM_CR",                 0x200020b8, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFF07037Fu, healthmon_nop_trigger, 5u },


    { "SYSREG:MPU_VIOLATION_SR",        0x200020F0, NOT_EQUAL_TO_VALUE, 0u,      0u, 0u,  1u,       healthmon_nop_trigger, 5u },
    { "SYSREG:SW_FAIL_ADDR0_CR",        0x200020F8, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFu, healthmon_nop_trigger, 5u },
    { "SYSREG:SW_FAIL_ADDR1_CR",        0x200020Fc, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3FF3Fu, healthmon_nop_trigger, 5u },


    { "SYSREG:EDAC_SR",                 0x20002100, NOT_EQUAL_TO_VALUE, 0u,      0u, 0u,  0x3FFFu,   healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_INTEN_CR",           0x20002100, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3FFFu,   healthmon_nop_trigger, 5u },

    { "SYSREG:EDAC_CNT_MMC",            0x20002108, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1Fu,     healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_CNT_DDRC",           0x2000210C, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1Fu,     healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_CNT_MAC0",           0x20002110, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1Fu,     healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_CNT_MAC1",           0x20002114, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1Fu,     healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_CNT_USB",            0x20002118, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1Fu,     healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_CNT_CAN0",           0x2000211c, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1Fu,     healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_CNT_CAN1",           0x20002120, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1Fu,     healthmon_nop_trigger, 5u },
    { "SYSREG:EDAC_INJECT_CR",          0x20002124, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x3FFFu,   healthmon_nop_trigger, 5u },
    { "SYSREG:MAINTENANCE_INTEN_CR",    0x20002140, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1FFFFFu, healthmon_nop_trigger, 5u },
    { "SYSREG:PLL_STATUS_INTEN_CR",     0x20002144, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x77u,     healthmon_nop_trigger, 5u },

    { "SYSREG:MAINTENANCE_INT_SR",      0x20002148, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1FFFFFu, healthmon_nop_trigger, 5u },// [20:0] == some cleared by writing 1, some y writing to PLL_STATUS
    { "SYSREG:PLL_STATUS_SR",           0x2000214c, NOT_EQUAL_TO_VALUE, 0x707u,  0u, 0u,  0x7FFu,    healthmon_nop_trigger, 5u },
    { "SYSREG:MISC_SR",                 0x20002154, NOT_EQUAL_TO_VALUE, 0u,      0u, 0u,  2u,       healthmon_nop_trigger, 5u },
    { "SYSREG:DLL_STATUS_SR",           0x2000215c, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x1FFFFFu, healthmon_nop_trigger, 5u },

    { "IOSCBCFG:STATUS",                0x37080004, NOT_EQUAL_TO_VALUE, 0u,      0u, 0u,  0xEu,     healthmon_nop_trigger, 5u },// [3:1] => scb_buserr, timeout, scb_error

    // unknown what the following should be...
    { "IOSCB_PLL:pll_se_0:PLL_CTRL",	0x38010004, CHANGED_SINCE_LAST, 0u,      0u, 25u, 1u,       healthmon_nop_trigger, 5u },
    { "IOSCB_PLL:pll_se_1:PLL_CTRL",	0x38020004, CHANGED_SINCE_LAST, 0u,      0u, 25u, 1u,       healthmon_nop_trigger, 5u },
    { "IOSCB_PLL:pll_ne_0:PLL_CTRL",	0x38040004, CHANGED_SINCE_LAST, 0u,      0u, 25u, 1u,       healthmon_nop_trigger, 5u },
    { "IOSCB_PLL:pll_ne_1:PLL_CTRL",	0x38080004, CHANGED_SINCE_LAST, 0u,      0u, 25u, 1u,       healthmon_nop_trigger, 5u },
    { "IOSCB_PLL:pll_nw_0:PLL_CTRL",	0x38100004, NOT_EQUAL_TO_VALUE, 1u,      0u, 25u, 1u,       healthmon_nop_trigger, 1u },
    { "IOSCB_PLL:pll_nw_1:PLL_CTRL",	0x38200004, CHANGED_SINCE_LAST, 0u,      0u, 25u, 1u,       healthmon_nop_trigger, 5u },
    { "IOSCB_PLL:pll_sw_0:PLL_CTRL",	0x38400004, CHANGED_SINCE_LAST, 0u,      0u, 25u, 1u,       healthmon_nop_trigger, 5u },
    { "IOSCB_PLL:pll_sw_1:PLL_CTRL",	0x38800004, CHANGED_SINCE_LAST, 0u,      0u, 25u, 1u,       healthmon_nop_trigger, 5u },
    { "IOSCB_PLL_MSS:PLL_CTRL",		0x3E001004, NOT_EQUAL_TO_VALUE, 1u,      0u, 25u, 1u,       healthmon_nop_trigger, 1u },
    { "IOSCB_PLL_DDR:PLL_CTRL",		0x3E010004, NOT_EQUAL_TO_VALUE, 1u,      0u, 25u, 1u,       healthmon_nop_trigger, 1u },
    { "IOSCB_PLL_SGMII:PLL_CTRL",	0x3E001004, NOT_EQUAL_TO_VALUE, 1u,      0u, 25u, 1u,       healthmon_nop_trigger, 1u },

    { "L2:Config:ECCInjectError",       0x02010040, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0x100FFu,            healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDirFixAddr",        0x02010100, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFFFFFFFFFu, healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDirFixCount",       0x02010108, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFu,         healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDirFailAddr",       0x02010120, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFFFFFFFFFu, healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDirFailCount",      0x02010128, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFu,         healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDataFixAddr",       0x02010140, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFFFFFFFFFu, healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDataFixCount",      0x02010148, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFu,         healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDataFailAddr",      0x02010160, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFFFFFFFFFu, healthmon_nop_trigger, 1u },
    { "L2:Config:ECCDataFailCount",     0x02010168, CHANGED_SINCE_LAST, 0u,      0u, 0u,  0xFFFFFFFFu,         healthmon_nop_trigger, 1u },
};

struct HealthMonitor_Status monitor_status[ARRAY_SIZE(monitors)] =
{
   { 0u, 0u, 0u, false }
};

const size_t monitors_array_size = ARRAY_SIZE(monitors);
