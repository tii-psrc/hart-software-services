/*
 * main.c
 *
 *  Created on: Jul 14, 2026
 *      Author: david
 */

#include <stdio.h>
#include <stdlib.h>


#include "myTypes.h"

#include "SBCC_CcsdsUtils.h"
#include "LIB_PusUtils.h"

#include "BOOT_Fillers.h"
#include "BOOT_Serializers.h"


#define SBRO_PACKET_MAX_NB (200)

void main()
{
	printf("Hello World");
	uint16_t sequenceCounter=0;
	uint16_t sequenceCounterHighSeverity=0;

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
	printf("info: sending %d bytes\n",totalDataSize);
	//print buffer
	for (uint16_t bIx=0;bIx<totalDataSize;bIx++)
	{
		printf("%02X ",sendPacketRaw[bIx]);
	}


}
