/*******************************************************************************/
/* Copyright David Gil 1998-2025                                               */
/* 								                                               */
/* davidgil@dgadv.com 			                                               */
/*******************************************************************************/

#ifndef SBCC_CcsdsUtils_H
#define SBCC_CcsdsUtils_H

/* system includes-------------------------------------------------------------*/
/* none */

/* application includes--------------------------------------------------------*/
#include "myTypes.h"

/* component includes----------------------------------------------------------*/
/* none */

/* macros-----------------------------------------------------------------------*/
#define CCSDS_VERSION_NUMBER (0) //0 CCSDS 133.0-B-1
#define CCSDS_STANDALONE_PACKET (0b11)

#define CCSDS_PRIMARY_HEADER_IS_TM (0)
#define CCSDS_PRIMARY_HEADER_IS_TC (1)

#define CCSDS_PACKET_TOTAL_LENGHT(x) ((x)->primaryHeader.dataLength+1+sizeof(CCSDS_PrimaryHeader_t))
#define CCSDS_PACKET_DATA_LENGHT(x) ((x)->primaryHeader.dataLength+1)
#define CCSDS_PACKET_DATA_LENGTH_TO_TOTAL_LENGHT(x) ((x)+1+sizeof(CCSDS_PrimaryHeader_t))
#define CCSDS_PACKET_START_DATA (sizeof(CCSDS_PrimaryHeader_t))


/* types------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) _CCSDS_RequestId_t_
{
	int temp;
}CCSDS_RequestId_t;

/*CCSDS-PUS ECSS-E-ST-70-41C15April2016-PUS
-- packet primary header
3 bits: version number : 0 CCSDS 133.0-B-1
1 bit: packet type : 0-TM, 1-TC
1 bit: secondary header :
11 bits: (destination if TM or dest if TC) application process ID : note, some may be reserved by the CCSDS standard
--  packet primary header - packet sequence control
2 bits: sequence flag : 11 for stand alone
14 bits: packet sequence count or packet name : incremented by 1 when the destination application process releases a packet
-- packet primary header - packet data length
16 bits: packet data length: just length of the data field
-- packet data field - up to 65536
var: secondary header
var: user data field*/
typedef struct __attribute__((packed)) _CCSDS_PrimaryHeader_t_
{
	//16 bits
	uint8_t versionNumber:3; //0 CCSDS 133.0-B-1
	uint8_t packetType:1; //0-TM, 1-TC
	uint8_t secondaryHeader:1; //0-no 1-yes
	uint16_t apid:11;//source APID if TM, destination APID if TC
	//sequence control
	uint8_t sequenceFlag:2; //11 for stand alone
	uint16_t sequenceCount:14;

	//packet data length //16 bits
	uint16_t dataLength;
}CCSDS_PrimaryHeader_t;

typedef struct __attribute__((packed)) _CCSDS_Packet_t_
{
	CCSDS_PrimaryHeader_t primaryHeader;
	uint8_t data;
} CCSDS_Packet_t;



/* public variables-------------------------------------------------------------*/
/* none */

/* public functions--------------------------------------------------------------*/
void CCSDS_FillPrimaryHeader(CCSDS_PrimaryHeader_t *target, bool_t isTc,bool_t hasSecondaryHeader,uint16_t apid,uint16_t sequenceCount);
bool_t CCSDS_CreatePacket(uint8_t *target,uint16_t targetNbMax,bool_t isTc,bool_t hasSecondaryHeader,uint16_t apid,uint16_t sequenceCount,uint16_t dataLength,uint8_t *data);
bool_t CCSDS_FinalizePacket(uint8_t *target,uint16_t targetNbMax,uint16_t totalDataLength);
void CCSDS_PrintPrimaryHeader(CCSDS_Packet_t *self);
void CCSDS_PrintCccsdsPrimaryHeader(CCSDS_PrimaryHeader_t *self);
void CCSDS_PrintPacket(CCSDS_Packet_t *self);
bool_t CCSDS_IsPacketSizeValid(CCSDS_Packet_t *self,uint16_t packetNb);

/* end */
#endif /* SBCC_CcsdsUtils_H */

