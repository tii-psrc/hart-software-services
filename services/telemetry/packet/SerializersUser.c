/*******************************************************************************/
/* Copyright David Gil 1998-2025                                               */
/* 								                                               */
/* davidgil@dgadv.com 			                                               */
/*******************************************************************************/

/* system includes-------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* application includes--------------------------------------------------------*/
#include <LIB_Endian.h>

/* component includes----------------------------------------------------------*/
#include "BOOT_SerializersUser.h"

/* local macros ---------------------------------------------------------------*/
/* none */

/* local types ----------------------------------------------------------------*/
/* none */

/* public variables -----------------------------------------------------------*/
/* none */

/* local variables ------------------------------------------------------------*/
/* none */

/* local prototypes -----------------------------------------------------------*/
/* none */

/* public functions -----------------------------------------------------------*/
bool_t BOOTSU_SerializeCCSDS_PrimaryHeader(uint8_t *target,uint16_t targetNb,uint16_t *totalDataSize,CCSDS_PrimaryHeader_t *structuredData)
{
	bool_t isValid=M_TRUE;

	if (sizeof(CCSDS_PrimaryHeader_t)<=(targetNb-*totalDataSize))
	{
		//byte 1
		target[0]=0;
		target[0]|=(structuredData->versionNumber & 0x03) << 5;
		target[0]|=(structuredData->packetType & 0x01) << 4;
		target[0]|=(structuredData->secondaryHeader & 0x01) << 3;
		target[0]|=(structuredData->apid & 0x300) >> 8;
		*totalDataSize=*totalDataSize+1;
		//byte 2
		target[1]=0;
		target[1]|=(structuredData->apid & 0xFF);
		*totalDataSize=*totalDataSize+1;
		//byte 3
		target[2]=0;
		target[2]=(structuredData->sequenceFlag & 0x03) << 6;;
		//printf("debug structuredData->sequenceCount %d\n",structuredData->sequenceCount);
		target[2]|=(structuredData->sequenceCount & 0x3F00) >> 8;
		*totalDataSize=*totalDataSize+1;
		//byte 4
		target[3]=0;
		target[3]=(structuredData->sequenceCount & 0xFF);
		*totalDataSize=*totalDataSize+1;
		//byte 5
		//printf("debug structuredData->dataLength %d\n",structuredData->dataLength);
		target[5]=(structuredData->dataLength & 0xFF00) >> 8;
		*totalDataSize=*totalDataSize+1;
		//byte 6
		target[5]=(structuredData->dataLength & 0xFF);
		*totalDataSize=*totalDataSize+1;

	}
	else
	{
		printf("warning: BOOTSU_SerializeCCSDS_PrimaryHeader not enough space left in target structure.\n");
		isValid=M_FALSE;
	}

	return isValid;
}

bool_t BOOTSU_SerializePUS_TcSecondaryHeader(uint8_t *target,uint16_t targetNb,uint16_t *totalDataSize,PUS_TcSecondaryHeader_t *structuredData)
{
	bool_t isValid=M_TRUE;

	if (sizeof(PUS_TcSecondaryHeader_t)<=(targetNb-*totalDataSize))
	{
		//byte 0
		target[*totalDataSize]=0;
		// 4 bits PIS Version Number
		target[*totalDataSize]|=(structuredData->versionTcPus & 0x0F) << 4;
		// 4 bits Ack
		target[*totalDataSize]|=(structuredData->acknowledgementFlags & 0x0F);
		*totalDataSize+=1;
		//byte 1
		//Service Type
		target[*totalDataSize]=structuredData->serviceType;
		*totalDataSize+=1;
		//byte 2
		//Sub Service
		target[*totalDataSize]=structuredData->serviceSubType;
		*totalDataSize+=1;
		//byte 3 and 4
		// source ID
		target[*totalDataSize]=(structuredData->sourceId & 0xFF00) >> 8;
		*totalDataSize+=1;
		target[*totalDataSize]=(structuredData->sourceId & 0x00FF);
		*totalDataSize+=1;
		//byte 5
		//reserved
		target[*totalDataSize]=structuredData->spare0;
		*totalDataSize+=1;

		//change endieness of needed ones
		//uint16_t sourceId;
//		LEND_Host2Network((uint8_t*)&structuredData->sourceId,sizeof(uint16_t));
//
//		memcpy(&target[*totalDataSize],structuredData,sizeof(PUS_TcSecondaryHeader_t));
//		*totalDataSize+=sizeof(PUS_TcSecondaryHeader_t);
	}
	else
	{
		printf("warning: BOOTSU_SerializePUS_TcSecondaryHeader not enough space left in target structure.\n");
		isValid=M_FALSE;
	}

	return isValid;
}

bool_t BOOTSU_SerializePUS_TmSecondaryHeader(uint8_t *target,uint16_t targetNb,uint16_t *totalDataSize,PUS_TmSecondaryHeader_t *structuredData)
{
	bool_t isValid=M_TRUE;

	if (sizeof(PUS_TmSecondaryHeader_t)<=(targetNb-*totalDataSize))
	{
		target[*totalDataSize]=0;
		 //uint8_t versionTmPus:4; //should be = 1 for ECSS-E-70-41A
		target[*totalDataSize]|=(structuredData->versionTmPus & 0x0F) << 4;
		 //uint8_t scTimeReferenceStatus:4; //0 when not supported NOTS
		target[*totalDataSize]|=(structuredData->scTimeReferenceStatus & 0x0F);
		*totalDataSize+=1;
		 //uint8_t serviceType;
		target[*totalDataSize]=structuredData->serviceType;
		*totalDataSize+=1;
		 //uint8_t serviceSubType;
		target[*totalDataSize]=structuredData->serviceSubType;
		*totalDataSize+=1;
		 //uint16_t messageTypeCounter;//per pid, per type/subtype, or 0 if not supported NOTS
		target[*totalDataSize]=(structuredData->messageTypeCounter & 0xFF00) >> 8;
		*totalDataSize+=1;
		target[*totalDataSize]=(structuredData->messageTypeCounter & 0x00FF);
		*totalDataSize+=1;
		 //uint16_t destinationId;
		target[*totalDataSize]=(structuredData->destinationId & 0xFF00) >> 8;
		*totalDataSize+=1;
		target[*totalDataSize]=(structuredData->destinationId & 0x00FF);
		*totalDataSize+=1;
		 //uint32_t time;
		target[*totalDataSize]=(structuredData->time & 0xFF000000) >> 24;
		*totalDataSize+=1;
		target[*totalDataSize]=(structuredData->time & 0x00FF0000) >> 16;
		*totalDataSize+=1;
		target[*totalDataSize]=(structuredData->time & 0x0000FF00) >> 8;
		*totalDataSize+=1;
		target[*totalDataSize]=(structuredData->time & 0x000000FF);
		*totalDataSize+=1;
		 //uint8_t spare0; //total size of header needs to be of a integer word size
		target[*totalDataSize]=structuredData->spare0;
		*totalDataSize+=1;

//		//change endieness of needed ones
//		// uint16_t messageTypeCounter;//per pid, per type/subtype, or 0 if not supported NOTS
//		LEND_Host2Network((uint8_t*)&structuredData->messageTypeCounter,sizeof(uint16_t));
//		//uint16_t destinationId;
//		LEND_Host2Network((uint8_t*)&structuredData->destinationId,sizeof(uint16_t));
//		//uint32_t time;
//		LEND_Host2Network((uint8_t*)&structuredData->time,sizeof(uint32_t));
//
//		memcpy(&target[*totalDataSize],structuredData,sizeof(PUS_TmSecondaryHeader_t));
//		*totalDataSize+=sizeof(PUS_TmSecondaryHeader_t);
	}
	else
	{
		printf("warning: BOOTSU_SerializePUS_TmSecondaryHeader not enough space left in target structure.\n");
		isValid=M_FALSE;
	}

	return isValid;
}

bool_t BOOTSU_SerializePUS_AcceptanceReport(uint8_t *target,uint16_t targetNb,uint16_t *totalDataSize,PUS_AcceptanceReport_t *structuredData)
{
	bool_t isValid=M_TRUE;

	if (sizeof(PUS_AcceptanceReport_t)<=(targetNb-*totalDataSize))
	{
		//change endieness of needed ones
		//uint16_t applicationProcessId:11;
		//uint16_t applicationProcessId=structuredData->applicationProcessId;

		//uint8_t debug1[2];
		//memcpy(&debug1,&applicationProcessId,sizeof(uint16_t));
		//printf("debug2 structuredData->applicationProcessId 0x%X 0x%X\n",debug1[0],debug1[1]);

		//TODO LEND_Host2Network((uint8_t*)&applicationProcessId,sizeof(uint16_t));
		//TODO structuredData->applicationProcessId=((0x7FF)&(applicationProcessId>>3));

		//memcpy(&debug1,&applicationProcessId,sizeof(uint16_t));
		//printf("debug2 structuredData->applicationProcessId 0x%X 0x%X\n",debug1[0],debug1[1]);

		//uint16_t packetSequenceCount:14;
		//TODO uint16_t sequenceCount=structuredData->packetSequenceCount;
		//TODO LEND_Host2Network((uint8_t*)&sequenceCount,sizeof(uint16_t));
		//TODO structuredData->packetSequenceCount=((0x3FFF)&sequenceCount);

		//((uint8_t*)(&structuredData))[0]=0xAB;

		memcpy(&target[*totalDataSize],structuredData,sizeof(PUS_AcceptanceReport_t));
		*totalDataSize+=sizeof(PUS_AcceptanceReport_t);

		//uint16_t arBase=*totalDataSize-sizeof(PUS_AcceptanceReport_t);//sizeof(PUS_TmSecondaryHeader_t)+sizeof(CCSDS_PrimaryHeader_t);
		//printf("debug3 BOOTSU_SerializePUS_AcceptanceReport 0x%02X 0x%02X 0x%02X 0x%02X\n",target[arBase+0],target[arBase+1],target[arBase+2],target[arBase+3]);
	}
	else
	{
		printf("warning: BOOTSU_SerializePUS_AcceptanceReport not enough space left in target structure.\n");
		isValid=M_FALSE;
	}

	return isValid;
}
/* local functions ------------------------------------------------------------*/
/* none */

/* end */
