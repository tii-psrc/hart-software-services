#ifndef HSS_WDOG_EXTERNAL_H
#define HSS_WDOG_EXTERNAL_H


#ifdef __cplusplus
extern "C" {
#endif

void wdog_external_init(void);
void wdog_external_idle(void);
void wdog_external_monitoring(void);

int wdog_external_stop(void);
int wdog_external_status(void);

#ifdef __cplusplus
}
#endif

#endif
