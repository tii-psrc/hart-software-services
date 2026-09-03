#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"
#include "hss_clock.h"
#include "csr_helper.h"
#include "ssmb_ipi.h"
#include "sbi/riscv_encoding.h"
#include "sbi/riscv_asm.h"

#if defined(CONFIG_SERVICE_WDOG_ENABLE_EXTERNAL)
#include "wdog_external.h"
#endif
#if IS_ENABLED(CONFIG_SERVICE_WDOG_ENABLE_E51)
#include "mss_watchdog.h"
#endif

#include "telemetry_publish.h"
#include "tinycli_hexdump.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "uart_helper.h"

#include <myTypes.h>

#include <SBCC_CcsdsUtils.h>
#include <LIB_PusUtils.h>

#include <BOOT_Fillers.h>
#include <BOOT_Serializers.h>

#include <LIB_Crc.h>

#include <string.h>

#define SBRO_PACKET_MAX_NB (200)

static uint16_t sequenceCounter = 0;
static uint16_t sequenceCounterHighSeverity = 0;

void ConvertToHex20Format(uint8_t *dataOut,uint16_t *totalDataSize);
void ConvertToHex20Format(uint8_t *dataOut,uint16_t *totalDataSize)
{
	uint8_t dataOut2[SBRO_PACKET_MAX_NB];
	uint16_t walkingIx=0;
	uint16_t dataOutNb=*totalDataSize;
	//add start sync 0x1A CF FC 1D
	dataOut2[walkingIx++]=0x1A;
	dataOut2[walkingIx++]=0xCF;
	dataOut2[walkingIx++]=0xFC;
	dataOut2[walkingIx++]=0x1D;
	//copy headers
	uint16_t headersSize=sizeof(CCSDS_PrimaryHeader_t)+sizeof(PUS_TmSecondaryHeader_t);
	memcpy(&dataOut2[walkingIx],&dataOut[0],headersSize);
	walkingIx+=headersSize;
	//add Headers CRC
	uint16_t headersCrc=CRC_CcsdsCrc16Get(PUS_DEFAULT_CRC_SEED, PUS_DEFAULT_CRC_OFFSET, &dataOut[0], headersSize);
	dataOut2[walkingIx++]=(headersCrc & 0xFF00) >> 8;
	dataOut2[walkingIx++]=(headersCrc & 0x00FF);
	//copy data
	uint16_t dataSize=dataOutNb-headersSize-2;
	memcpy(&dataOut2[walkingIx],&dataOut[headersSize],dataSize);
	walkingIx+=dataSize;
	//add Data CRC (replace end CRC)
	uint16_t dataCrc=CRC_CcsdsCrc16Get(PUS_DEFAULT_CRC_SEED, PUS_DEFAULT_CRC_OFFSET, &dataOut[headersSize], dataSize);
	dataOut2[walkingIx++]=(dataCrc & 0xFF00) >> 8;
	dataOut2[walkingIx++]=(dataCrc & 0x00FF);
	//add end sync 0x1D FC CF 1A
	dataOut2[walkingIx++]=0x1D;
	dataOut2[walkingIx++]=0xFC;
	dataOut2[walkingIx++]=0xCF;
	dataOut2[walkingIx++]=0x1A;
	dataOutNb=walkingIx;
	memcpy(dataOut,dataOut2,dataOutNb);
	*totalDataSize=dataOutNb;
}


static uint8_t boot_device = 255;
uint8_t update_boot_w25(uint8_t boot_w25)
{
	if (!memcmp(CONFIG_SERVICE_BOOT_DEVICE_NAME, "red", 4)
			&& boot_w25 != 255) {
		boot_device = BOOT_BD_MSSW25_RED + boot_w25;
	} else if (!memcmp(CONFIG_SERVICE_BOOT_DEVICE_NAME, "nom", 4) &&
			boot_w25 != 255) {
		boot_device = boot_w25;
	}

	return boot_device;
}

static const char *boot_status_name(uint8_t boot_status)
{
	switch (boot_status) {
	case HSS_BOOT_BS_BOOTLOADER1_STARTED:
		return "BOOTLOADER1_STARTED";

	case HSS_BOOT_BS_DDR_TRAINING_STARTED:
		return "DDR_TRAINING_STARTED";

	case HSS_BOOT_BS_DDR_TRAINING_FAILED_REBOOT:
		return "DDR_TRAINING_FAILED_REBOOT";

	case HSS_BOOT_BS_BOOTLOADER2_STARTED:
		return "BOOTLOADER2_STARTED";

	case HSS_BOOT_BS_BOOTLOADER2_FAILED_REBOOT:
		return "BOOTLOADER2_FAILED_REBOOT";

	case HSS_BOOT_BS_LINUX_BOOT_STARTED:
		return "LINUX_BOOT_STARTED";

	case HSS_BOOT_BS_LINUX_BOOT_FAILED_REBOOT:
		return "LINUX_BOOT_FAILED_REBOOT";

	case HSS_BOOT_BS_LINUX_BOOT_SUCEEDED:
		return "LINUX_BOOT_SUCEEDED";

	default:
		return "UNKNOWN";
	}
}

static const char *boot_device_name(uint8_t device)
{
	switch (device) {
	case BOOT_BD_MSSW25_NOM:
		return "MSSW25_NOM";

	case BOOT_BD_FPGAW25_NOM:
		return "FPGAW25_NOM";

	case BOOT_BD_MSSW25_RED:
		return "MSSW25_RED";

	case BOOT_BD_FPGAW25_RED:
		return "FPGAW25_RED";

	default:
		return "UNKNOWN";
	}
}

void do_tm_publish(uint8_t boot_status)
{
	char buf[1024];

	/*
	 * One line per published boot message, so the console shows the boot
	 * progression - 0, 1, 3, 5, 7 on a clean boot - and names anything
	 * off that path. The device reads UNKNOWN(255) until
	 * update_boot_w25() has run, which is expected for the first two.
	 */
	format_log(HSS_HART_E51, buf,
			"\r\n[BOOT TM] status %u (%s), device %u (%s)\r\n",
			(uint32_t)boot_status, boot_status_name(boot_status),
			(uint32_t)boot_device, boot_device_name(boot_device));

	CCSDS_PrimaryHeader_t ccsdsHeader;
	PUS_TmSecondaryHeader_t pusTmHeader;
	BOOT_HkReport_t sendTotalPacketStructureData;
	BOOT_HkBootSw_t hkInfo;
	uint8_t sendPacketRaw[SBRO_PACKET_MAX_NB];
	uint16_t totalDataSize=0;

	CCSDS_FillPrimaryHeader(&ccsdsHeader, M_FALSE, M_TRUE, BOOT_APID_BOOTSW, sequenceCounter++);
	PUS_CreateTmHeader(&pusTmHeader, BOOT_HKREPORT_SERVICE, BOOT_HKREPORT_SUBSERVICE, sequenceCounterHighSeverity++, BOOT_APID_OBC);
	hkInfo.bootStatus=boot_status;
	hkInfo.bootDevice=boot_device;
	BOOTF_FillHkReport(&sendTotalPacketStructureData,&ccsdsHeader,&pusTmHeader,BOOT_HKID_BOOT_HK,(void*)&hkInfo,sizeof(hkInfo));

	//serialize
	BOOTS_SerializeHkReport(sendPacketRaw,sizeof(sendPacketRaw),&totalDataSize,&sendTotalPacketStructureData);
	PUS_FinalizePacket(sendPacketRaw,&totalDataSize);//puts crc and fills data length

	//convert to hex20 format
	ConvertToHex20Format(sendPacketRaw,&totalDataSize);

	//send
	format_log(HSS_HART_E51, buf, "info: sending %d bytes\r\n",totalDataSize);
	//print buffer
	HSS_TinyCLI_HexDump(sendPacketRaw, totalDataSize);
#if defined(CONFIG_BOARD_SCAI_DPU460)
	MSS_UART_polled_tx(HSS_UART_GetInstance(HSS_HART_U54_2), sendPacketRaw, totalDataSize); // Real send packet via UART
	MSS_UART_polled_tx(HSS_UART_GetInstance(HSS_HART_U54_3), sendPacketRaw, totalDataSize); // Real send packet via UART
#endif
}

/*
 * Fault-injection helpers, used by the psrc2025_fault* test branches to
 * stop the boot at a chosen point after publishing a failure status.
 *
 * Halting the E51 must not stop the watchdogs from being served, or the halt
 * turns into a reset: on the DPU460 the FPGA external watchdog resets the
 * board 20 s after its last ping, and it is only pinged by the wdog service
 * from the E51 superloop, which a halt no longer runs. The wait therefore
 * spins in 500 ms steps and serves the watchdogs itself.
 */
static void tm_fault_serve_watchdogs(void)
{
#if defined(CONFIG_SERVICE_WDOG_ENABLE_EXTERNAL)
	__wdog_external_ping();
#endif
#if IS_ENABLED(CONFIG_SERVICE_WDOG_ENABLE_E51)
	if (!MSS_WD_forbidden_status(MSS_WDOG0_LO)) {
		MSS_WD_reload(MSS_WDOG0_LO);
	}
#endif
}

void tm_fault_wait_secs(uint32_t seconds)
{
	for (uint32_t i = 0u; i < seconds * 2u; i++) {
		HSS_SpinDelay_MilliSecs(500u);
		tm_fault_serve_watchdogs();
	}
}

void tm_fault_halt(void)
{
	mHSS_DEBUG_PRINTF(LOG_ERROR,
			"[BOOT TM] fault injection: E51 halted, boot will not proceed\n");

	while (true) {
		tm_fault_wait_secs(1u);
	}
}

/*
 * Entered on a U54, in M-mode, via IPI_MSG_GOTO: park the hart for good.
 * Interrupts are already off on arrival (the GOTO handler clears MIE); keep
 * them off so nothing - not even a later IPI - brings the hart back.
 */
static void __attribute__((noreturn)) tm_fault_u54_park(void)
{
	csr_write(CSR_MIE, 0u);

	while (true) {
		wfi();
	}
}

/*
 * Stop whatever the U54s are running - u-boot or Linux - by parking each of
 * them in tm_fault_u54_park(). This is the same GOTO IPI the reboot service
 * uses to warm-restart a hart whose watchdog fired, so it reaches a hart that
 * is running Linux in S-mode: the IPI is taken by the hart's M-mode trap
 * handler, which switches to the parking loop and never returns.
 */
void tm_fault_park_u54s(void)
{
	mHSS_DEBUG_PRINTF(LOG_ERROR,
			"[BOOT TM] fault injection: parking u54_1..u54_4\n");

	for (enum HSSHartId peer = HSS_HART_U54_1; peer < HSS_HART_NUM_PEERS; peer++) {
		IPI_Send(peer, IPI_MSG_GOTO, 0u, PRV_M, tm_fault_u54_park, NULL);
		HSS_SpinDelay_MilliSecs(50u);
	}
}

void do_tm_publish_init(void *this_uart)
{
  MSS_UART_init((mss_uart_instance_t *)this_uart, MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
}
