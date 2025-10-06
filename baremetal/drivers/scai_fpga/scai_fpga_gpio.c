#include "scai_fpga_gpio.h"

#include "scai_fpga_platform.h"
#include "hss_debug.h"

static void setGpioReg(volatile uintptr_t reg, uint32_t value) {
    mHSS_DEBUG_PRINTF(LOG_ERROR, "GPIO REGW 0x%08x\t=\t0x%08x\n", reg, value);
    *(volatile uint32_t*)(reg + Scai_Fpga_Gpio_Reg_Wdata) = value;
}

static uint32_t getGpioReg(volatile uintptr_t reg) {
    uint32_t value = *(volatile uint32_t*)(reg + Scai_Fpga_Gpio_Reg_Rdata);
    mHSS_DEBUG_PRINTF(LOG_ERROR, "GPIO REGR 0x%08x\t=\t0x%08x\n", reg, value);
    return value;
}

// =============================================================================
// GPIO Public API
// =============================================================================

void scai_fpga_gpio_init(void) {
    Scai_Fpga_Gpio_Reg_0_t gpio_0 = {0};
    Scai_Fpga_Gpio_Reg_1_t gpio_1 = {0};
    Scai_Fpga_Gpio_Reg_2_t gpio_2 = {0};

    setGpioReg(GPIO_0_BASE_ADDRESS, gpio_0.word);

    gpio_1.bits.C21_ENA_SS1 = 1;
    setGpioReg(GPIO_1_BASE_ADDRESS, gpio_1.word);

    gpio_2.bits.A23_ENA_SS2 = 1;
    gpio_2.bits.F20_ENA_RS2 = 1;
    setGpioReg(GPIO_2_BASE_ADDRESS, gpio_2.word);
}

void scai_fpga_gpio_enable_mt29f(void) {
    Scai_Fpga_Gpio_Reg_1_t gpio_1;
    Scai_Fpga_Gpio_Reg_2_t gpio_2;

    gpio_1.word = getGpioReg(GPIO_1_BASE_ADDRESS);
    gpio_1.bits.C21_ENA_SS1 = 1;
    setGpioReg(GPIO_1_BASE_ADDRESS, gpio_1.word);

    gpio_2.word = getGpioReg(GPIO_2_BASE_ADDRESS);
    gpio_2.bits.A23_ENA_SS2 = 1;
    setGpioReg(GPIO_2_BASE_ADDRESS, gpio_2.word);
}

void scai_fpga_gpio_disable_mt29f(void) {
    Scai_Fpga_Gpio_Reg_2_t gpio_2;

    gpio_2.word = getGpioReg(GPIO_2_BASE_ADDRESS);
    gpio_2.bits.A23_ENA_SS2 = 0;

    setGpioReg(GPIO_2_BASE_ADDRESS, gpio_2.word);
}
