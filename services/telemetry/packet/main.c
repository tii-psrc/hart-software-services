/*
 * main.c
 *
 *  Created on: Jul 14, 2026
 *      Author: david
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include <myTypes.h>

#include <SBCC_CcsdsUtils.h>
#include <LIB_PusUtils.h>
#include <LIB_Crc.h>

#include <BOOT_Fillers.h>
#include <BOOT_Serializers.h>


#define SBRO_PACKET_MAX_NB (200)

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

	//convert to hex20 format
	ConvertToHex20Format(sendPacketRaw,&totalDataSize);

	//send
	printf("info: sending %d bytes\n",totalDataSize);
	//print buffer
	for (uint16_t bIx=0;bIx<totalDataSize;bIx++)
	{
		printf("%02X ",sendPacketRaw[bIx]);
	}


}
