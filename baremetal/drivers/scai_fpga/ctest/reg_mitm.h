#ifndef SCAI_REG_MITM_H
#define SCAI_REG_MITM_H

#include "cpu_types.h"

void scai_set_reg(addr_t reg_addr, uint32_t value);
uint32_t scai_get_reg(addr_t reg_addr);
void scai_gpio_set_reg(addr_t reg_addr, uint32_t value);
uint32_t scai_gpio_get_reg(addr_t reg_addr);

#endif /* SCAI_REG_MITM_H */