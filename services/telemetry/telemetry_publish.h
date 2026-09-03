#ifndef HSS_TELEMETRY_PUBLISH_H
#define HSS_TELEMETRY_PUBLISH_H

#ifdef __cplusplus
extern "C" {
#endif

enum boot_w25
{
	MSS_W25 = 0,
	FPGA_W25 = 1,
};

#if 1
typedef enum _HSS_BOOT_EnumBootStatus_t
{
  HSS_BOOT_BS_BOOTLOADER1_STARTED=0,
  HSS_BOOT_BS_DDR_TRAINING_STARTED=1,
  HSS_BOOT_BS_DDR_TRAINING_FAILED_REBOOT=2,
  HSS_BOOT_BS_BOOTLOADER2_STARTED=3,
  HSS_BOOT_BS_BOOTLOADER2_FAILED_REBOOT=4,
  HSS_BOOT_BS_LINUX_BOOT_STARTED=5,
  HSS_BOOT_BS_LINUX_BOOT_FAILED_REBOOT=6,
  HSS_BOOT_BS_LINUX_BOOT_SUCEEDED=7,
  HSS_BOOT_BS_NB=8
} HSS_BOOT_EnumBootStatus_t;
#endif

uint8_t update_boot_w25(uint8_t boot_w25);
void do_tm_publish(uint8_t boot_status);
void do_tm_publish_init(void *this_uart);

/* Fault-injection helpers (psrc2025_fault* branches). */
void tm_fault_wait_secs(uint32_t seconds);
void tm_fault_halt(void) __attribute__((noreturn));
void tm_fault_park_u54s(void);

#ifdef __cplusplus
}
#endif

#endif
