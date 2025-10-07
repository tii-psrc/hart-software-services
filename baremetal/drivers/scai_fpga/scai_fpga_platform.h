#ifndef SCAI_FPGA_PLATFORM_H
#define SCAI_FPGA_PLATFORM_H

#include "hss_types.h"

typedef enum {
    Scai_Fpga_Gpio_Reg_Wdata   =  0,
    Scai_Fpga_Gpio_Reg_Wmask   =  4,
    Scai_Fpga_Gpio_Reg_Wtoggle =  8,
    Scai_Fpga_Gpio_Reg_Rpins   =  0,
    Scai_Fpga_Gpio_Reg_Rdata   =  4,
    Scai_Fpga_Gpio_Reg_Rmask   =  8,
    Scai_Fpga_Gpio_Reg_Rtoggle = 12
} Scai_Fpga_Gpio_Reg_Offsets;

extern const uintptr_t MSS_APB_BASE_ADDRESS;
extern const uintptr_t QSPI_0_BASE_ADDRESS;
extern const uintptr_t QSPI_1_BASE_ADDRESS;
extern const uintptr_t QSPI_2_BASE_ADDRESS;

extern const uintptr_t W25N01_FPGA_BASE_ADDR;
extern const uintptr_t W25N01_DIRECT_BASE_ADDR;

// static const uintptr_t MT29F_BASE_ADDR         = QSPI_1_BASE_ADDRESS;
extern const uintptr_t MT29F_CHIP_0_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_1_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_2_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_3_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_4_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_5_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_6_BASE_ADDR;
extern const uintptr_t MT29F_CHIP_7_BASE_ADDR;

extern const uintptr_t MT29F_BASE_ADDRS[];

extern const uintptr_t GPIO_0_BASE_ADDRESS;
extern const uintptr_t GPIO_1_BASE_ADDRESS;
extern const uintptr_t GPIO_2_BASE_ADDRESS;
extern const uintptr_t GPIO_3_BASE_ADDRESS;

//------------------------------------------------------------------------------
// QSPI Controller Register Definitions
//------------------------------------------------------------------------------
/**
 * @brief CTRL1 Register (Write-Only, Offset +4)
 * Configures and initiates a QSPI transaction.
 */

typedef union {
    struct {
        uint32_t Scai_Qspi_Ctrl_1_Chip_Enable      : 1;  // Bit 0:      Chip Enable (1=active) when software driven
        uint32_t Scai_Qspi_Ctrl_1_Nwp              : 1;  // Bit 1:      nWP line value
        uint32_t Scai_Qspi_Ctrl_1_Reset            : 1;  // Bit 2:      nReset line value (1=normal operation)
        uint32_t Scai_Qspi_Ctrl_1_Data_Mode        : 1;  // Bit 3:      Data interface (1=Byte, 0=Word)
        uint32_t Scai_Qspi_Ctrl_1_Lane_Width       : 1;  // Bit 4:      Interface usage (1=x4, 0=x1)
        uint32_t Scai_Qspi_Ctrl_1_Auto_Mask        : 1;  // Bit 5:      Mask operation for auto mode
        uint32_t Scai_Qspi_Ctrl_1_Clk_Div          : 3;  // Bits 8:6:   Clock divider
        uint32_t Scai_Qspi_Ctrl_1_Start            : 1;  // Bit 9:      Start transaction
        uint32_t Scai_Qspi_Ctrl_1_Tx_Count         : 11; // Bits 20:10: Words/Bytes to Transmit
        uint32_t Scai_Qspi_Ctrl_1_Rx_Count         : 11; // Bits 31:21: Words/Bytes to Receive
    } bits;
    uint32_t word;
} Scai_Qspi_Ctrl_1_Reg_t;

/**
 * @brief CTRL2 Register (Write-Only, Offset +8)
 * Configures automated operations.
 */
typedef union {
    struct {
        uint32_t Scai_Qspi_Ctrl_2_Auto_Cmd         : 8;  // Bits 7:0:   Command for auto operation
        uint32_t Scai_Qspi_Ctrl_2_Auto_Mask        : 8;  // Bits 15:8:  Mask for auto operation
        uint32_t Scai_Qspi_Ctrl_2_Auto_Repeats     : 15; // Bits 30:16: Max retries for auto operation
        uint32_t Scai_Qspi_Ctrl_2_Auto_Enable      : 1;  // Bit 31:     Enable auto operation
    } bits;
    uint32_t word;
} Scai_Qspi_Ctrl_2_Reg_t;

/**
 * @brief CTRL3 Register (Write-Only, Offset +12)
 * Provides direct control over QSPI lines for GPIO/toggling.
 */
typedef union {
    struct {
        uint32_t Scai_Qspi_Ctrl_3_Gpio_Out         : 7;  // Bits 6:0: GPIO output values (Rst,CE,CLK,D[3:0])
        uint32_t Scai_Qspi_Ctrl_3_Gpio_En          : 7;  // Bits 13:7: GPIO enable (1=output, 0=input)
        uint32_t Scai_Qspi_Ctrl_3_Gpio_Use_Gpio    : 1;  // Bit 14: Use GPIO mode (1=GPIO, 0=QSPI)
        uint32_t Scai_Qspi_Ctrl_3_Gpio_Use_Toggle  : 1;  // Bit 15: Use toggling mode
        uint32_t Scai_Qspi_Ctrl_3_Nhold            : 1;  // Bit 16: nHold line value
        uint32_t Scai_Qspi_Ctrl_3_Dummy_Cnt        : 5;  // Bits 21:17: Number of dummy cycles
        uint32_t Scai_Qspi_Ctrl_3_R3_Reserved      : 10; // Reserved
    } bits;
    uint32_t word;
} Scai_Qspi_Ctrl_3_Reg_t;

/**
 * @brief STATUS1 Register (Read-Only, Offset +4)
 * Provides the primary status of the QSPI controller.
 */
typedef union {
    struct {
        uint32_t Scai_Qspi_Status_1_Idle           : 1;  // Bit 0: Controller is idle
        uint32_t Scai_Qspi_Status_1_Rx_Fifo_Empty  : 1;  // Bit 1: RX FIFO is empty
        uint32_t Scai_Qspi_Status_1_Rx_Fifo_Full   : 1;  // Bit 2: RX FIFO is full
        uint32_t Scai_Qspi_Status_1_Tx_Fifo_Empty  : 1;  // Bit 3: TX FIFO is empty
        uint32_t Scai_Qspi_Status_1_Tx_Fifo_Full   : 1;  // Bit 4: TX FIFO is full
        uint32_t Scai_Qspi_Status_1_Err_Overrun    : 1;  // Bit 5: Overrun error
        uint32_t Scai_Qspi_Status_1_Auto_Op_Ready  : 1;  // Bit 6: Auto operation is ready
        uint32_t Scai_Qspi_Status_1_Auto_Op_Error  : 1;  // Bit 7: Auto operation error
        uint32_t Scai_Qspi_Status_1_Lane_Data      : 4;  // Bits 11:8: Current state of QSPI data out lines
        uint32_t Scai_Qspi_Status_1_Lane_Clk       : 1;  // Bit 12: Current state of QSPI CLK line
        uint32_t Scai_Qspi_Status_1_Lane_Ce        : 1;  // Bit 13: Current state of QSPI CE line
        uint32_t Scai_Qspi_Status_1_Lane_Rst       : 1;  // Bit 14: Current state of QSPI RST line
        uint32_t Scai_Qspi_Status_1_Reserved_1     : 1;  // Bit 15: Reserved
        uint32_t Scai_Qspi_Status_1_Lane_Gpio_Out  : 7;  // Bits 22:16: GPIO data out lines
        uint32_t Scai_Qspi_Status_1_Reserved_2     : 1;  // Bit 23: Reserved
        uint32_t Scai_Qspi_Status_1_Lane_Ext_Data  : 4;  // Bits 27:24: External data lines
        uint32_t Scai_Qspi_Status_1_Lane_Ext_Clk   : 1;  // Bit 28: External CLK
        uint32_t Scai_Qspi_Status_1_Lane_Ext_Rst   : 1;  // Bit 29: External RST
        uint32_t Scai_Qspi_Status_1_Lane_Ext_Ce    : 1;  // Bit 30: External CE
        uint32_t Scai_Qspi_Status_1_Reserved_3     : 1;  // Bit 31: Reserved
    } bits;
    uint32_t word;
} Scai_Qspi_Status_1_Reg_t;

/**
 * @brief STATUS2 Register (Read-Only, Offset +8)
 * Provides FIFO status and counters.
 */
typedef union {
    struct {
        uint32_t Scai_Qspi_Status_2_Rx_Fifo_Full   : 1;  // Bit 0: RX FIFO is full
        uint32_t Scai_Qspi_Status_2_Rx_Fifo_Empty  : 1;  // Bit 1: RX FIFO is empty
        uint32_t Scai_Qspi_Status_2_Rx_Fifo_RdCnt  : 7;  // Bits 8:2: RX FIFO read counter
        uint32_t Scai_Qspi_Status_2_Rx_Fifo_WrCnt  : 7;  // Bits 15:9: RX FIFO write counter
        uint32_t Scai_Qspi_Status_2_Tx_Fifo_Full   : 1;  // Bit 16: TX FIFO is full
        uint32_t Scai_Qspi_Status_2_Tx_Fifo_Empty  : 1;  // Bit 17: TX FIFO is empty
        uint32_t Scai_Qspi_Status_2_Tx_Fifo_RdCnt  : 7;  // Bits 24:18: TX FIFO read counter
        uint32_t Scai_Qspi_Status_2_Tx_Fifo_WrCnt  : 7;  // Bits 31:25: TX FIFO write counter
    } bits;
    uint32_t word;
} Scai_Qspi_Status_2_Reg_t;

typedef enum {
    Scai_Fpga_Qspi_Reg_Offset_Data                 = 0x00,
    Scai_Fpga_Qspi_Reg_Offset_Ctrl1                = 0x04,
    Scai_Fpga_Qspi_Reg_Offset_Ctrl2                = 0x08,
    Scai_Fpga_Qspi_Reg_Offset_Ctrl3                = 0x0C,
    Scai_Fpga_Qspi_Reg_Offset_Status1              = 0x04,
    Scai_Fpga_Qspi_Reg_Offset_Status2              = 0x08
} Scai_Fpga_Qspi_Reg_Offset_t;


// GPIO Pins
typedef union {
    struct {
        uint32_t A5_C4_I2C_SDA_Z : 1;
        uint32_t E8_C3_I2C_SDA_Z : 1;
        uint32_t B7_C2_I2C_SDA_Z : 1;
        uint32_t C8_C1_I2C_SDA_Z : 1;
        uint32_t Reserved1       : 18;
        uint32_t SDDR_PLL_LOCK   : 1;
        uint32_t CPU_PLL_LOCK    : 1;
        uint32_t FIC3_DLL_LOCK   : 1;
        uint32_t FIC0_DLL_LOCK   : 1;
        uint32_t DDR4_CTRL_RDY   : 1;
        uint32_t A5_C4_I2C_SDA   : 1;
        uint32_t E8_C3_I2C_SDA   : 1;
        uint32_t B7_C2_I2C_SDA   : 1;
        uint32_t C8_C1_I2C_SDA   : 1;
        uint32_t L14_nINT        : 1;
    } bits;
    uint32_t word;
} Scai_Fpga_Gpio_Reg_0_t;

typedef union {
    struct {
        uint32_t A27_CC4_IO0     : 1;
        uint32_t B29_CC2_IO1     : 1;
        uint32_t B15_ENA_CAN     : 1;
        uint32_t C17_ENA_RS1     : 1;
        uint32_t C21_ENA_SS1     : 1;   // MT29F Chip Enable
        uint32_t C26_LED1        : 1;
        uint32_t D15_SOC_P_OFF   : 1;
        uint32_t D26_LED2F       : 1;
        uint32_t B24_CC3_IO1F    : 1;
        uint32_t A24_CC3_IO0F    : 1;
        uint32_t F22_E_PWR_IMU_R : 1;
        uint32_t H18_E_PWR_IMU_N : 1;
        uint32_t R1_E_PWR_C3     : 1;
        uint32_t R2_E_PWR_C4     : 1;
        uint32_t U4_E_PWR_C2     : 1;
        uint32_t V1_E_PWR_C1     : 1;
        uint32_t D16_SOC_IS_NOM0 : 1;
        uint32_t A15_SOC_IS_NOM1 : 1;
        uint32_t D18_SOC_IS_NOM2 : 1;
        uint32_t B25_CC3_IO2     : 1;
        uint32_t A25_CC3_IO3     : 1;
        uint32_t A29_CC2_IO2     : 1;
        uint32_t C27_CC2_IO3     : 1;
        uint32_t B14_FVTT_PGOOD  : 1;
        uint32_t C13_IO_PGOOD    : 1;
        uint32_t D14_SVTT_PGOOD  : 1;
        uint32_t D13_IO_nPFO     : 1;
        uint32_t E15_P_1V2_IMON  : 1;
        uint32_t E16_P_1V2_PGOOD : 1;
        uint32_t E17_P_5V0_nFLT  : 1;
        uint32_t E18_P_5V0_PGOOD : 1;
        uint32_t F15_P_3V3_IMON  : 1;
    } bits;
    uint32_t word;
} Scai_Fpga_Gpio_Reg_1_t;

typedef union {
    struct {
        uint32_t A23_ENA_SS2     : 1;   // MT29F Chip Enable
        uint32_t K17_ENA         : 1;
        uint32_t J14_nRst        : 1;
        uint32_t L15_MDC         : 1;
        uint32_t B22_CC2_IO1     : 1;
        uint32_t E23_CC2_IO0     : 1;
        uint32_t E21_CC1_IO1     : 1;
        uint32_t D21_CC1_IO0     : 1;
        uint32_t F20_ENA_RS2     : 1;   // RS-422 Enable
        uint32_t D8_C1_I2C_SCL   : 1;
        uint32_t C7_C2_I2C_SCL   : 1;
        uint32_t F8_C3_I2C_SCL   : 1;
        uint32_t B6_C4_I2C_SCL   : 1;
        uint32_t A12_IO_WDI_DATA : 1;
        uint32_t FOO             : 1;
        uint32_t J18_P_2V5_IMON  : 1;        
        uint32_t H17_P_1V8_IMON  : 1;
        uint32_t H16_P_1V0_PGOOD : 1;
        uint32_t G17_P_1V8_PGOOD : 1;
        uint32_t G16_P_1V0_IMON  : 1;        
        uint32_t G15_P_3V3_PGOOD : 1;
        uint32_t A13_IO_nWDO     : 1;
        uint32_t V2_PGOOD_C1     : 1;
        uint32_t T4_PGOOD_C2     : 1;        
        uint32_t K18_P_2V5_PGOOD : 1;
        uint32_t H14_MDIO        : 1;
        uint32_t B21_CC2_IO3     : 1;
        uint32_t C22_CC2_IO2     : 1;        
        uint32_t D24_CC1_IO3     : 1;
        uint32_t D25_CC1_IO2     : 1;
        uint32_t R3_PGOOD_C4     : 1;
        uint32_t P1_PGOOD_C3     : 1;
    } bits;
    uint32_t word;
} Scai_Fpga_Gpio_Reg_2_t;

#endif /* SCAI_FPGA_PLATFORM_H */