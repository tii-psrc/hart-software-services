#ifndef HSS_TELEMETRY_PUBLISH_H
#define HSS_TELEMETRY_PUBLISH_H

#ifdef __cplusplus
extern "C" {
#endif

void do_tm_publish(void);
void do_tm_publish_init(void *this_uart);

#ifdef __cplusplus
}
#endif

#endif
