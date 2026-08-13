#include "config.h"
#include "hss_types.h"
#include "hss_debug.h"

#include "telemetry_publish.h"
#include "tinycli_hexdump.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "uart_helper.h"

#include <myTypes.h>

#include <SBCC_CcsdsUtils.h>
#include <LIB_PusUtils.h>

#include <BOOT_Fillers.h>
#include <BOOT_Serializers.h>

#define SBRO_PACKET_MAX_NB (200)

static uint16_t sequenceCounter = 0;
static uint16_t sequenceCounterHighSeverity = 0;

void do_tm_publish(void)
{
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "Hello World\r\n");

	CCSDS_PrimaryHeader_t ccsdsHeader;
	PUS_TmSecondaryHeader_t pusTmHeader;
	BOOT_HkReport_t sendTotalPacketStructureData;
	BOOT_HkBootSw_t hkInfo;
	uint8_t sendPacketRaw[SBRO_PACKET_MAX_NB];
	uint16_t totalDataSize=0;

	CCSDS_FillPrimaryHeader(&ccsdsHeader, M_FALSE, M_TRUE, BOOT_APID_BOOTSW, sequenceCounter++);
	PUS_CreateTmHeader(&pusTmHeader, BOOT_HKREPORT_SERVICE, BOOT_HKREPORT_SUBSERVICE, sequenceCounterHighSeverity++, BOOT_APID_OBC);
	hkInfo.bootStatus=BOOT_BS_BOOTLOADER1_STARTED;
	hkInfo.bootDevice=BOOT_BD_MSSW25_NOM;
	BOOTF_FillHkReport(&sendTotalPacketStructureData,&ccsdsHeader,&pusTmHeader,BOOT_HKID_BOOT_HK,(void*)&hkInfo,sizeof(hkInfo));

	//serialize
	BOOTS_SerializeHkReport(sendPacketRaw,sizeof(sendPacketRaw),&totalDataSize,&sendTotalPacketStructureData);
	PUS_FinalizePacket(sendPacketRaw,&totalDataSize);//puts crc and fills data length

	//send
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "info: sending %d bytes\r\n",totalDataSize);
	//print buffer
#if 1
	HSS_TinyCLI_HexDump(sendPacketRaw, totalDataSize);
	MSS_UART_polled_tx(HSS_UART_GetInstance(HSS_HART_U54_2), sendPacketRaw, totalDataSize);
	MSS_UART_polled_tx(HSS_UART_GetInstance(HSS_HART_U54_3), sendPacketRaw, totalDataSize);
#else
	for (uint16_t bIx=0;bIx<totalDataSize;bIx++)
	{
		mHSS_DEBUG_PRINTF(LOG_NORMAL, "%02X ",sendPacketRaw[bIx]);
	}
#endif
}

void do_tm_publish_init(void *this_uart)
{
  MSS_UART_init((mss_uart_instance_t *)this_uart, MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
}
