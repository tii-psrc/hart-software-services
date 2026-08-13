/*******************************************************************************/
/* Copyright David Gil 1998-2025                                               */
/* 								                                               */
/* davidgil@dgadv.com 			                                               */
/*******************************************************************************/

/* system includes-------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>

/* application includes--------------------------------------------------------*/
#include <LIB_Endian.h>

/* component includes----------------------------------------------------------*/
#include <SBCC_CcsdsUtils.h>

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
void CCSDS_FillPrimaryHeader(CCSDS_PrimaryHeader_t *target, bool_t isTc,bool_t hasSecondaryHeader,uint16_t apid,uint16_t sequenceCount)
{
	target->versionNumber=CCSDS_VERSION_NUMBER;
	//uint8_t packetType:1; //0-TM, 1-TC
	target->packetType=isTc;
	//uint8_t secondaryHeader:1; //0-no 1-yes
	target->secondaryHeader=hasSecondaryHeader;
	//uint16_t apid:11;
	target->apid=apid;
	//uint8_t sequenceFlag:2; //11 for stand alone
	target->sequenceFlag=CCSDS_STANDALONE_PACKET;
	//uint16_t sequenceCount:14;
	target->sequenceCount=sequenceCount;
	//for now
	target->dataLength=0;
}

bool_t CCSDS_FinalizePacket(uint8_t *target,uint16_t targetNbMax,uint16_t totalDataLength)
{
	bool_t isError=M_FALSE;
	if (targetNbMax<totalDataLength)
	{
		isError=M_TRUE;
		printf("warning: CCSDS_FinalizePacket (targetNbMax<totalDataLength)\n");
	}
	else
	{
		//header
		CCSDS_PrimaryHeader_t *primaryHeader=(CCSDS_PrimaryHeader_t *)&target[0];
		primaryHeader->dataLength=totalDataLength-sizeof(CCSDS_PrimaryHeader_t)-1;
		LEND_Host2Network((uint8_t*)&primaryHeader->dataLength,sizeof(uint16_t));
	}

	return isError;
}

bool_t CCSDS_CreatePacket(uint8_t *target,uint16_t targetNbMax,bool_t isTc,bool_t hasSecondaryHeader,uint16_t apid,uint16_t sequenceCount,uint16_t dataLength,uint8_t *data)
{
	bool_t isError=M_FALSE;
	if (targetNbMax<dataLength+sizeof(CCSDS_PrimaryHeader_t))
	{
		isError=M_TRUE;
	}
	else
	{
		//header
		CCSDS_PrimaryHeader_t *primaryHeader=(CCSDS_PrimaryHeader_t *)&target[0];
		//uint8_t versionNumber:3; //0 CCSDS 133.0-B-1
		primaryHeader->versionNumber=CCSDS_VERSION_NUMBER;
		//uint8_t packetType:1; //0-TM, 1-TC
		primaryHeader->packetType=isTc;
		//uint8_t secondaryHeader:1; //0-no 1-yes
		primaryHeader->secondaryHeader=hasSecondaryHeader;
		//uint16_t apid:11;
		primaryHeader->apid=apid;
		//uint8_t sequenceFlag:2; //11 for stand alone
		primaryHeader->sequenceFlag=CCSDS_STANDALONE_PACKET;
		//uint16_t sequenceCount:14;
		primaryHeader->sequenceCount=sequenceCount;
		primaryHeader->dataLength=dataLength-1;
		//copy the data to the packet
		for (uint16_t bIx=0;bIx<dataLength;bIx++)
		{
			//printf("debug: %d %d\n",bIx,data[bIx]);
			target[sizeof(CCSDS_PrimaryHeader_t)+bIx]=data[bIx];
			//printf("debug: %d %d %d\n",bIx,data[bIx],target[sizeof(primaryHeader)+bIx]);
		}
	}

	return isError;
}

void CCSDS_PrintCccsdsPrimaryHeader(CCSDS_PrimaryHeader_t *self)
{
	printf("\t CCSDS Primary Header:\n");
	//header
	//uint8_t versionNumber:3; //0 CCSDS 133.0-B-1
	printf("\t\t versionNumber: %d\n",self->versionNumber);
	//uint8_t packetType:1; //0-TM, 1-TC
	printf("\t\t packetType: %d\n",self->packetType);
	//uint8_t secondaryHeader:1; //0-no 1-yes
	printf("\t\t secondaryHeader: %d\n",self->secondaryHeader);
	//uint16_t apid:11;
	printf("\t\t apid: %d\n",self->apid);
	//sequence control
	//uint8_t sequenceFlag:2; //11 for stand alone
	printf("\t\t sequenceFlag: %d\n",self->sequenceFlag);
	//uint16_t sequenceCount:14;
	printf("\t\t sequenceCount: %d\n",self->sequenceCount);
	//packet data length //16 bits
	//uint16_t dataLength;
	printf("\t\t dataLength: %d\n",self->dataLength);
}

void CCSDS_PrintPrimaryHeader(CCSDS_Packet_t *self)
{
	printf("CCSDS packet:\n");
	CCSDS_PrintCccsdsPrimaryHeader(&self->primaryHeader);
}

void CCSDS_PrintPacket(CCSDS_Packet_t *self)
{
	CCSDS_PrintPrimaryHeader(self);
	//data
	printf("\t data:");
	for (uint16_t bIx=0;bIx<self->primaryHeader.dataLength+1;bIx++)
	{
		printf(" %d",((uint8_t*)&self->data)[bIx]);
	}
	printf("\n");
}

bool_t CCSDS_IsPacketSizeValid(CCSDS_Packet_t *self,uint16_t packetNb)
{
	bool_t isValid=M_TRUE;
	uint16_t totalLength;
	uint16_t packetDataLength;
	if (packetNb<sizeof(CCSDS_PrimaryHeader_t))
	{
		printf("warning: CCSDS_IsPacketSizeValid packet not large enough to have a mandatory CCSDS header (nb: %d)\n",packetNb);
		isValid=M_FALSE;
	}
	else
	{
		packetDataLength=self->primaryHeader.dataLength;
		//swap size if needed because it is needed for next step
		//LEND_Network2Host((uint8_t*)&packetDataLength,sizeof(uint16_t));
		totalLength=CCSDS_PACKET_DATA_LENGTH_TO_TOTAL_LENGHT(packetDataLength);
		//printf("debug CCSDS_IsPacketSizeValid %d\n",self->primaryHeader.dataLength);
		//
	}

	if (isValid==M_TRUE)
	{
		if (packetNb<totalLength)
		{
			printf("warning: CCSDS_IsPacketSizeValid packetNb %d needed %d\n",packetNb,totalLength);
			isValid=M_FALSE;
		}
	}


	return isValid;
}


/* local functions ------------------------------------------------------------*/
/* none */

/* end */
