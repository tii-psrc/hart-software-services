#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include "wdog_external.h"

#include "mss_gpio.h"

static int stop_external_watchdog = 0;
static int external_watchdog_status_changed = 0;

static HSSTicks_t wdog_external_last_time = 0u;
static HSSTicks_t wdog_external_current_time = 0u;

static int __wdog_external_handler(void)
{
	HSSTicks_t duration_time = (HSSTicks_t)(1 * TICKS_PER_SEC);
	wdog_external_current_time = HSS_GetTime();
    
	HSSTicks_t delayTick;

	if (stop_external_watchdog) {
		if (external_watchdog_status_changed) {
			mHSS_DEBUG_PRINTF(LOG_NORMAL, "External WDOG stopped ...\r\n");
			external_watchdog_status_changed = 0;
		}
		return stop_external_watchdog;
	}

	if (wdog_external_current_time > (wdog_external_last_time + duration_time)) {
		mHSS_DEBUG_PRINTF(LOG_WARN, "External WDOG: %lu msec elapsed ... \n",
				(wdog_external_current_time - wdog_external_last_time) / TICKS_PER_MILLISEC);
	}


#if 1
	/*
	 *
	 *           FPGA detects this risiding edge signal for valid ping
	 *          /
	 *         /
	 *        /_________________________________________________
	 *       /                                                   \
	 *      v                                                     v
	 *
	 *      +-----+                                               +-----+
	 *      |     |                                               |     |
	 *      |     |                                               |     |
	 *      |     |     (Ping signal to external watchdog)        |     |
	 *      |     |                                               |     |
	 *      |     |                                               |     |
	 *      |     |                                               |     |
	 * -----+     +-----------------------------------------------+     +-----
	 *
	 *      <---------------------------+------------------------->
	 *                                  |
	 *                                  |
	 *       The external watchdog timeout is configured to 10sec
	 *
	 *
	 *      <--+--><--------------------+------------------------->
	 *         |                        |
	 *         |                        |
	 *        1us     This time is variable depending on HSS bootloader's superloop service due to iteration operation,
	 *                but most of time it takes under 1sec.
	 *             ---------(superloop iteration cycle)------------
	 *             |                                              |
	 *             v                                              v
	 *            (watdog service --> service A --> B --> ... ---> watchdog service ---> service A again ---> ...)
	 *
	 *
	 *
	 */
	MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_5, 1);

	delayTick = HSS_GetTime();
	while (!HSS_Timer_IsElapsed(delayTick, ONE_MILLISEC/1000llu)) { ; }

	MSS_GPIO_set_output(GPIO2_LO, MSS_GPIO_5, 0);
#endif

	wdog_external_last_time = HSS_GetTime();

	return 0;
}

void wdog_external_init(void)
{
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "External WDOG started ...\r\n");
	stop_external_watchdog = 0;

	wdog_external_last_time = HSS_GetTime();
	MSS_GPIO_init(GPIO2_LO);
	MSS_GPIO_config(GPIO2_LO, MSS_GPIO_5, MSS_GPIO_OUTPUT_MODE);

	__wdog_external_handler();
}

void wdog_external_idle(void)
{
	__wdog_external_handler();
}

void wdog_external_monitoring(void)
{
	__wdog_external_handler();
}

int wdog_external_stop(void)
{
	stop_external_watchdog = 1;
	external_watchdog_status_changed = 1;

	return stop_external_watchdog;
}

int wdog_external_status(void)
{
	HSSTicks_t current_time = HSS_GetTime();

	if (stop_external_watchdog) {
		mHSS_DEBUG_PRINTF(LOG_NORMAL, "External WDOG stopped %llu sec ago ... \n",
				(current_time - wdog_external_last_time) / TICKS_PER_SEC);
	} else {
		mHSS_DEBUG_PRINTF(LOG_NORMAL, "External WDOG: last ping was %llu usec ago ... \n",
				(current_time - wdog_external_last_time) / (TICKS_PER_MILLISEC/1000llu));
	}

	return 0;
}
