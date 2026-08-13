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

	//convert to hex20 format
	ConvertToHex20Format(sendPacketRaw,&totalDataSize);

	//send
	mHSS_DEBUG_PRINTF(LOG_NORMAL, "info: sending %d bytes\r\n",totalDataSize);
	//print buffer
	HSS_TinyCLI_HexDump(sendPacketRaw, totalDataSize);
#if defined(CONFIG_BOARD_SCAI_DPU460)
	MSS_UART_polled_tx(HSS_UART_GetInstance(HSS_HART_U54_2), sendPacketRaw, totalDataSize); // Real send packet via UART
	MSS_UART_polled_tx(HSS_UART_GetInstance(HSS_HART_U54_3), sendPacketRaw, totalDataSize); // Real send packet via UART
#endif
}

void do_tm_publish_init(void *this_uart)
{
  MSS_UART_init((mss_uart_instance_t *)this_uart, MSS_UART_921600_BAUD,
      MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);
}
