/*******************************************************************************
 * (c) Copyright 2016-2017 Microsemi Corporation.  All rights reserved.
 *
 * Platform definitions
 * Version based on requirements of RISCV-HAL
 *
 * SVN $Revision: 9546 $
 * SVN $Date: 2017-10-24 10:27:17 +0530 (Tue, 24 Oct 2017) $
 */
 /*=========================================================================*//**
  @mainpage Sample file detailing how hw_platform.h should be constructed for 
    the Mi-V processors.

    @section intro_sec Introduction
    The hw_platform.h is to be located in the project root directory.
    Currently this file must be hand crafted when using the Mi-V Soft Processor.
    
    You can use this file as sample.
    Rename this file from sample_hw_platform.h to hw_platform.h and store it in
    the root folder of your project. Then customize it per your HW design.

    @section driver_configuration Project configuration Instructions
    1. Change SYS_CLK_FREQ define to frequency of Mi-V Soft processor clock
    2  Add all other core BASE addresses
    3. Add peripheral Core Interrupt to Mi-V Soft processor interrupt mappings
    4. Define MSCC_STDIO_UART_BASE_ADDR if you want a CoreUARTapb mapped to STDIO
*//*=========================================================================*/

#ifndef HW_PLATFORM_H
#define HW_PLATFORM_H

/***************************************************************************//**
 * Soft-processor clock definition
 * This is the only clock brought over from the Mi-V Soft processor Libero design.
 */
#ifndef SYS_CLK_FREQ 
#define SYS_CLK_FREQ                    100000000UL
#endif
/***************************************************************************//**
 * Non-memory Peripheral base addresses
 * Format of define is:
 * <corename>_<instance>_BASE_ADDR
 */
#define APB_BASE_ADDRESS                0x40000000UL

#define UARTS_BASE_ADDRESS              (APB_BASE_ADDRESS+0x0000L)
#define GPIOs_BASE_ADDRESS              (APB_BASE_ADDRESS+0x0100L)
#define TIMERS_BASE_ADDRESS             (APB_BASE_ADDRESS+0x0200L)
#define QSPIs0_BASE_ADDRESS             (APB_BASE_ADDRESS+0x0300L)
#define QSPIs1_BASE_ADDRESS             (APB_BASE_ADDRESS+0x0400L)
#define QSPIs2_BASE_ADDRESS             (APB_BASE_ADDRESS+0x0500L)
#define ADCs_BASE_ADDRESS               (APB_BASE_ADDRESS+0x0600L)
#define IMUs_BASE_ADDRESS               (APB_BASE_ADDRESS+0x0700L)



#define UART_DBG0_BASE_ADDRESS          (UARTS_BASE_ADDRESS)
#define UART_DBG1_BASE_ADDRESS         (UARTS_BASE_ADDRESS + 16)

#define GPIOs0_BASE_ADDRESS             (GPIOs_BASE_ADDRESS)
#define GPIOs1_BASE_ADDRESS             (GPIOs_BASE_ADDRESS + 16)
#define GPIOs2_BASE_ADDRESS             (GPIOs_BASE_ADDRESS + 32)
#define GPIOs3_BASE_ADDRESS             (GPIOs_BASE_ADDRESS + 48)

#define BNOR_BASE_ADDRESS               (QSPIs0_BASE_ADDRESS)
#define BacBNOR_BASE_ADDRESS            (QSPIs0_BASE_ADDRESS + 16)
#define SNOR_BASE_ADDRESS               (QSPIs0_BASE_ADDRESS + 32)
#define BacSNOR_BASE_ADDRESS            (QSPIs0_BASE_ADDRESS + 48)

#define TIMERS_100NH_ADDRESS            (TIMERS_BASE_ADDRESS)
#define TIMERS_100NL_ADDRESS            (TIMERS_BASE_ADDRESS+4)
#define TIMERS_STATUS_ADDRESS           (TIMERS_BASE_ADDRESS+8)
#define TIMERS_1U_ADDRESS               (TIMERS_BASE_ADDRESS+12)
#define TIMERS_STATUS_ADDRESS           (TIMERS_BASE_ADDRESS+8)
#define TIMERS_TO0_ADDRESS              (TIMERS_BASE_ADDRESS)
#define TIMERS_TO1_ADDRESS              (TIMERS_BASE_ADDRESS+4)
#define TIMERS_TO2_ADDRESS              (TIMERS_BASE_ADDRESS+8)
#define TIMERS_TO3_ADDRESS              (TIMERS_BASE_ADDRESS+12)
#define TIMERS_TO4_ADDRESS              (TIMERS_BASE_ADDRESS+16)
#define TIMERS_TO5_ADDRESS              (TIMERS_BASE_ADDRESS+20)
#define TIMERS_TO6_ADDRESS              (TIMERS_BASE_ADDRESS+24)
#define TIMERS_TO7_ADDRESS              (TIMERS_BASE_ADDRESS+28)
#define TIMERS_CONTROL_ADDRESS          (TIMERS_BASE_ADDRESS+32)



#define M0_BASE_ADDRESS                 (QSPIs1_BASE_ADDRESS)
#define M1_BASE_ADDRESS                 (QSPIs1_BASE_ADDRESS + 16)
#define M2_BASE_ADDRESS                 (QSPIs1_BASE_ADDRESS + 32)
#define M3_BASE_ADDRESS                 (QSPIs1_BASE_ADDRESS + 48)

#define M4_BASE_ADDRESS                 (QSPIs2_BASE_ADDRESS)
#define M5_BASE_ADDRESS                 (QSPIs2_BASE_ADDRESS + 16)
#define M6_BASE_ADDRESS                 (QSPIs2_BASE_ADDRESS + 32)
#define M7_BASE_ADDRESS                 (QSPIs2_BASE_ADDRESS + 48)

#define IMU1_BASE_ADDRESS               (IMUs_BASE_ADDRESS)
#define IMU2_BASE_ADDRESS               (IMUs_BASE_ADDRESS + 64)


enum timers_assignment
{
        TMR_ADC = 1,
        TMR_QSPI = 2,
        TMR_MQSPI = 3,
};

/***************************************************************************//**
 * User edit section- Edit sections below if required
 */
#ifdef MSCC_STDIO_THRU_CORE_UART_APB
/*
 * A base address mapping for the STDIO printf/scanf mapping to CortUARTapb
 * must be provided if it is being used
 *
 * e.g. #define MSCC_STDIO_UART_BASE_ADDR COREUARTAPB1_BASE_ADDR
 */
#define MSCC_STDIO_UART_BASE_ADDR COREUARTAPB0_BASE_ADDR

#ifndef MSCC_STDIO_UART_BASE_ADDR
#error MSCC_STDIO_UART_BASE_ADDR not defined- e.g. #define MSCC_STDIO_UART_BASE_ADDR COREUARTAPB1_BASE_ADDR
#endif

#ifndef MSCC_STDIO_BAUD_VALUE
/*
 * The MSCC_STDIO_BAUD_VALUE define should be set in your project's settings to
 * specify the baud value used by the standard output CoreUARTapb instance for
 * generating the UART's baud rate if you want a different baud rate from the
 * default of 115200 baud
 */
#define MSCC_STDIO_BAUD_VALUE           115200
#endif  /*MSCC_STDIO_BAUD_VALUE*/

#endif  /* end of MSCC_STDIO_THRU_CORE_UART_APB */
/*******************************************************************************
 * End of user edit section
 */
#endif /* HW_PLATFORM_H */


