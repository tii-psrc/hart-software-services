/*******************************************************************************
 * Copyright 2019-2021 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * MPFS Embedded Software
 *
 */


#include "config.h"
#include "hss_types.h"

#include <sbi/sbi_ecall.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_error.h>
//#include <sbi/sbi_trap.h>
//#include <sbi/sbi_version.h>
//#include <sbi/riscv_asm.h>
//#include <sbi/riscv_barrier.h>


#include "opensbi_service.h"
#include "opensbi_ecall.h"

#if !IS_ENABLED(CONFIG_OPENSBI)
#  error OPENSBI needed for this module
#endif

#if IS_ENABLED(CONFIG_USE_IHC) && IS_ENABLED(CONFIG_SERVICE_OPENSBI_IHC)
#  include "miv_ihc.h"
#  include "opensbi_ihc_ecall.h"
#endif

#if IS_ENABLED(CONFIG_USE_IHC) && IS_ENABLED(CONFIG_SERVICE_OPENSBI_RPROC)
#  include "opensbi_rproc_ecall.h"
#endif

#if IS_ENABLED(CONFIG_USE_USER_CRYPTO) && IS_ENABLED(CONFIG_SERVICE_OPENSBI_CRYPTO)
#  include "opensbi_crypto_ecall.h"
#endif

#include "hss_boot_service.h"

#if IS_ENABLED(CONFIG_SERVICE_TELEMETRY)
#include "uart_helper.h"
#include "opensbi_telemetry_ecall.h"
#endif

int HSS_SBI_ECALL_Handler(long extid, long funcid,
    const struct sbi_trap_regs *regs, unsigned long *out_val, struct sbi_trap_info *out_trap)
{
    int result = 0;
    uint32_t index;
		char buf[1024];

    switch (funcid) {
        //
        // MiV IHC functions
        case SBI_EXT_IHC_CTX_INIT:
            __attribute__((fallthrough)); // deliberate fallthrough
        case SBI_EXT_IHC_SEND:
            __attribute__((fallthrough)); // deliberate fallthrough
        case SBI_EXT_IHC_RECEIVE:
#if IS_ENABLED(CONFIG_USE_IHC) && IS_ENABLED(CONFIG_SERVICE_OPENSBI_IHC)
            result = sbi_ecall_ihc_handler(extid, funcid, regs, out_val, out_trap);
#endif
            break;

        case SBI_EXT_RPROC_STATE:
            __attribute__((fallthrough)); // deliberate fallthrough
        case SBI_EXT_RPROC_START:
            __attribute__((fallthrough)); // deliberate fallthrough
        case SBI_EXT_RPROC_STOP:
#if IS_ENABLED(CONFIG_USE_IHC) && IS_ENABLED(CONFIG_SERVICE_OPENSBI_RPROC)
            result = sbi_ecall_rproc_handler(extid, funcid, regs, out_val, out_trap);
#endif
            break;

#if IS_ENABLED(CONFIG_USE_USER_CRYPTO) && IS_ENABLED(CONFIG_SERVICE_OPENSBI_CRYPTO)
        case SBI_EXT_CRYPTO_INIT:
            __attribute__((fallthrough)); // deliberate fallthrough
        case SBI_EXT_CRYPTO_SERVICES_PROBE:
            __attribute__((fallthrough)); // deliberate fallthrough
        case SBI_EXT_CRYPTO_SERVICES:
            result = sbi_ecall_crypto_handler(extid, funcid, regs, out_val, out_trap);
            break;
#endif

#if IS_ENABLED(CONFIG_SERVICE_TELEMETRY)
        case SBI_EXT_SCAI_RPROC_STATE:
        case SBI_EXT_SCAI_RPROC_START:
        case SBI_EXT_SCAI_RPROC_STOP:
            format_log(HSS_HART_E51, buf, "%s(): funcid(0x%X)\r\n", __func__, funcid);
            format_log(HSS_HART_E51, buf, "regs->zero     : 0x%08lX\r\n", regs->zero);
            format_log(HSS_HART_E51, buf, "regs->ra       : 0x%08lX\r\n", regs->ra);
            format_log(HSS_HART_E51, buf, "regs->sp       : 0x%08lX\r\n", regs->sp);
            format_log(HSS_HART_E51, buf, "regs->gp       : 0x%08lX\r\n", regs->gp);
            format_log(HSS_HART_E51, buf, "regs->tp       : 0x%08lX\r\n", regs->tp);
            format_log(HSS_HART_E51, buf, "regs->t0       : 0x%08lX\r\n", regs->t0);
            format_log(HSS_HART_E51, buf, "regs->t1       : 0x%08lX\r\n", regs->t1);
            format_log(HSS_HART_E51, buf, "regs->t2       : 0x%08lX\r\n", regs->t2);
            format_log(HSS_HART_E51, buf, "regs->s0       : 0x%08lX\r\n", regs->s0);
            format_log(HSS_HART_E51, buf, "regs->s1       : 0x%08lX\r\n", regs->s1);
            format_log(HSS_HART_E51, buf, "regs->a0       : 0x%08lX\r\n", regs->a0);
            format_log(HSS_HART_E51, buf, "regs->a1       : 0x%08lX\r\n", regs->a1);
            format_log(HSS_HART_E51, buf, "regs->a2       : 0x%08lX\r\n", regs->a2);
            format_log(HSS_HART_E51, buf, "regs->a3       : 0x%08lX\r\n", regs->a3);
            format_log(HSS_HART_E51, buf, "regs->a4       : 0x%08lX\r\n", regs->a4);
            format_log(HSS_HART_E51, buf, "regs->a5       : 0x%08lX\r\n", regs->a5);
            format_log(HSS_HART_E51, buf, "regs->a6       : 0x%08lX\r\n", regs->a6);
            format_log(HSS_HART_E51, buf, "regs->a7       : 0x%08lX\r\n", regs->a7);
            format_log(HSS_HART_E51, buf, "regs->s2       : 0x%08lX\r\n", regs->s2);
            format_log(HSS_HART_E51, buf, "regs->s3       : 0x%08lX\r\n", regs->s3);
            format_log(HSS_HART_E51, buf, "regs->s4       : 0x%08lX\r\n", regs->s4);
            format_log(HSS_HART_E51, buf, "regs->s5       : 0x%08lX\r\n", regs->s5);
            format_log(HSS_HART_E51, buf, "regs->s6       : 0x%08lX\r\n", regs->s6);
            format_log(HSS_HART_E51, buf, "regs->s7       : 0x%08lX\r\n", regs->s7);
            format_log(HSS_HART_E51, buf, "regs->s8       : 0x%08lX\r\n", regs->s8);
            format_log(HSS_HART_E51, buf, "regs->s9       : 0x%08lX\r\n", regs->s9);
            format_log(HSS_HART_E51, buf, "regs->s10      : 0x%08lX\r\n", regs->s10);
            format_log(HSS_HART_E51, buf, "regs->s11      : 0x%08lX\r\n", regs->s11);
            format_log(HSS_HART_E51, buf, "regs->t3       : 0x%08lX\r\n", regs->t3);
            format_log(HSS_HART_E51, buf, "regs->t4       : 0x%08lX\r\n", regs->t4);
            format_log(HSS_HART_E51, buf, "regs->t5       : 0x%08lX\r\n", regs->t5);
            format_log(HSS_HART_E51, buf, "regs->t6       : 0x%08lX\r\n", regs->t6);
            format_log(HSS_HART_E51, buf, "regs->mepc     : 0x%08lX\r\n", regs->mepc);
            format_log(HSS_HART_E51, buf, "regs->mstatus  : 0x%08lX\r\n", 
                regs->mstatus);
            format_log(HSS_HART_E51, buf, "regs->mstatusH : 0x%08lX\r\n", 
                regs->mstatusH);
            format_log(HSS_HART_E51, buf, "\r\n"); 
            format_log(HSS_HART_E51, buf, "out_trap->epc   : 0x%08lX\r\n",
                out_trap->epc);
            format_log(HSS_HART_E51, buf, "out_trap->cause : 0x%08lX\r\n",
                out_trap->cause);
            format_log(HSS_HART_E51, buf, "out_trap->tval  : 0x%08lX\r\n",
                out_trap->tval);
            format_log(HSS_HART_E51, buf, "out_trap->tval2 : 0x%08lX\r\n",
                out_trap->tval2);
            format_log(HSS_HART_E51, buf, "out_trap->tinst : 0x%08lX\r\n",
                out_trap->tinst);
            format_log(HSS_HART_E51, buf, "out_trap->gva   : 0x%08lX\r\n",
                out_trap->gva);
            format_log(HSS_HART_E51, buf, "\r\n"); 
            result = sbi_ecall_telemetry_handler(extid, funcid, regs, out_val, out_trap);
            break;
#endif
        //
        // HSS functions
        case SBI_EXT_HSS_REBOOT:
            IPI_MessageAlloc(&index);
            IPI_MessageDeliver(index, HSS_HART_E51, IPI_MSG_BOOT_REQUEST, 0u, NULL, NULL);
            result = SBI_OK;
            break;

        default:
            result = SBI_ENOTSUPP;
    };

    return result;
}

int HSS_SBI_Vendor_Ext_Check(long extid)
{
    return (SBI_EXT_MICROCHIP_TECHNOLOGY == extid);
}
