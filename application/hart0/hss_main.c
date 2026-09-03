/******************************************************************************************
 *
 * MPFS HSS Embedded Software
 *
 * Copyright 2019-2025 Microchip FPGA Embedded Systems Solutions.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/*\!
 *\file Main Entrypoint
 *\brief Main Entrypoint
 */

#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include <assert.h>

#include "hss_state_machine.h"

#include "hss_debug.h"
#include "ssmb_ipi.h"

#include "hss_init.h"
#include "hss_registry.h"

#if IS_ENABLED(CONFIG_SERVICE_WDOG)
#  include "wdog_service.h"
#endif

#include "csr_helper.h"

#include <string.h>

#if defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
#include "telemetry_publish.h"
#endif

/******************************************************************************************/

void hss_main(void);


void hss_main(void)
{
    /*
     * FAULT INJECTION (psrc2025_fault0): stop here, before the wdog service
     * is told what to monitor, before BOOTLOADER1_STARTED (0) is published
     * and before the superloop runs a single service. Only HSS_Init() has
     * run: data/bss, the E51 console UART and OpenSBI setup. One console
     * line, then a bare spin - no watchdog is served.
     */
    mHSS_DEBUG_PRINTF(LOG_ERROR,
        "[BOOT TM] fault injection: E51 halted before BOOTLOADER1_STARTED, no service will run\n");
    while (true) { ; }

#if IS_ENABLED(CONFIG_SERVICE_WDOG)
    HSS_Wdog_MonitorHart(HSS_HART_ALL);
#endif

#if defined(CONFIG_SERVICE_TELEMETRY_PUBLISH)
		do_tm_publish(HSS_BOOT_BS_BOOTLOADER1_STARTED);
#endif
    while (true) {
        RunStateMachines(spanOfPGlobalStateMachines, pGlobalStateMachines);
    }
}

int main(int argc, char **argv)
{
    (void)argc; // unused
    (void)argv; // unused

    HSS_Init();

    if (current_hartid() != 0) {
        sbi_hart_hang();
    }

    hss_main();

    // will never be reached
    __builtin_unreachable();

    return 0;
}
