#include "ctest/reg_mitm.h"

#include "hss_debug.h"
#include "hss_types.h"

void scai_set_reg(addr_t reg_addr, uint32_t value) {
    volatile uint32_t* reg = (volatile uint32_t*)reg_addr;
    mHSS_DEBUG_PRINTF(LOG_ERROR, "REGW 0x%08x\t=\t0x%08x\n", reg_addr, value);
    *reg = value;
}

uint32_t scai_get_reg(addr_t reg_addr) {
    uint32_t val;
    volatile uint32_t* reg = (volatile uint32_t*)reg_addr;
    val = *reg;
    mHSS_DEBUG_PRINTF(LOG_ERROR, "REGR 0x%08x\t=\t0x%08x\n", reg_addr, val);
    return val;
}
