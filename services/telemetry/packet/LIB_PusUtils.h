/*******************************************************************************/
/* Copyright David Gil 1998-2025                                               */
/* 								                                               */
/* davidgil@dgadv.com 			                                               */
/*******************************************************************************/

#ifndef LIB_PusUtils_H
#define LIB_PusUtils_H

/* system includes-------------------------------------------------------------*/
/* none */

/* application includes--------------------------------------------------------*/
#include "myTypes.h"

/* component includes----------------------------------------------------------*/
#include "SBCC_CcsdsUtils.h"

/* macros-----------------------------------------------------------------------*/
#define PUS_VERSION_NO (2)
#define PUS_IS_TC_HEADER_SIZE(x) (CCSDS_PACKET_DATA_LENGHT(x)>=sizeof(PUS_TcSecondaryHeader_t)?M_TRUE:M_FALSE)
#define PUS_IS_TM_HEADER_SIZE(x) (CCSDS_PACKET_DATA_LENGHT(x)>=sizeof(PUS_TmSecondaryHeader_t)?M_TRUE:M_FALSE)
#define PUS_GET_TC_DATA_SIZE(x) ((x)->primaryHeader.dataLength+1-sizeof(PUS_TcSecondaryHeader_t))
#define PUS_GET_TM_DATA_SIZE(x) ((x)->primaryHeader.dataLength+1-sizeof(PUS_TmSecondaryHeader_t))

#define PUS_DEFAULT_SERVICE (0)
#define PUS_DEFAULT_SUBSERVICE (0)

#define PUS_DEFAULT_CRC_SEED (0xFFFF)
#define PUS_DEFAULT_CRC_OFFSET (0x00)

#define PUS_IS_ACKNOWLEDGE_REQUESTED(x) (((x)->acknowledgementFlags)&(0b1000))
#define PUS_IS_EXECUTION_REQUESTED(x) (((x)->acknowledgementFlags)&(0b0001))

/* types------------------------------------------------------------------------*/
typedef struct __attribute__((packed)) _PUS_TmSecondaryHeader_t_
{
 uint8_t versionTmPus:4; //should be = 1 for ECSS-E-70-41A
 uint8_t scTimeReferenceStatus:4; //0 when not supported NOTS
 uint8_t serviceType;
 uint8_t serviceSubType;
 uint16_t messageTypeCounter;//per pid, per type/subtype, or 0 if not supported NOTS
 uint16_t destinationId;
 uint32_t time;
 uint8_t spare0; //total size of header needs to be of a integer word size
}PUS_TmSecondaryHeader_t;

typedef struct __attribute__((packed)) _PUS_TcSecondaryHeader_t_
{
	uint8_t versionTcPus:4; //should be = 1 for ECSS-E-70-41A
	uint8_t acknowledgementFlags:4; //NOTS //bit 3 = successful acknowledgment //bit 2 = successful start of execution //bit 1 = successful progress of execution // successful end of execution
	uint8_t serviceType;
	uint8_t serviceSubType;
	uint16_t sourceId;
	uint8_t spare0; //total size of header needs to be of a integer word size
}PUS_TcSecondaryHeader_t;

typedef struct __attribute__((packed)) _PUS_AcceptanceReport_t_
{
	uint8_t packetVersionNumber:3;

	uint8_t packetType:1;
	uint8_t secondaryHeaderFlag:1;
	uint16_t applicationProcessId:11;

	uint8_t sequenceFlags:2;
	uint16_t packetSequenceCount:14;

} PUS_AcceptanceReport_t;

typedef struct __attribute__((packed)) _PUS_TcPacket_t__
{
	CCSDS_PrimaryHeader_t primaryHeader;
	PUS_TcSecondaryHeader_t secondaryHeader;
	uint8_t data;
} PUS_TcPacket_t;

typedef struct __attribute__((packed)) _PUS_TmPacket_t__
{
	CCSDS_PrimaryHeader_t primaryHeader;
	PUS_TmSecondaryHeader_t secondaryHeader;
	uint8_t data;
} PUS_TmPacket_t;



typedef  uint16_t PUS_Crc_t;

/* public variables-------------------------------------------------------------*/
/* none */

/* public functions--------------------------------------------------------------*/

//headers creators
void PUS_CreateTcHeader(PUS_TcSecondaryHeader_t *self, bool_t isWantedAcknowledgment, bool_t isWantedExecutionResult, uint8_t serviceType, uint8_t serviceSubType, uint16_t sourceId);
void PUS_CreateTmHeader(PUS_TmSecondaryHeader_t *self, uint8_t serviceType, uint8_t serviceSubType,uint16_t messageTypeCounter,uint16_t destinationId);
void PUS_CreateAcceptanceReport(PUS_AcceptanceReport_t *self,uint8_t packetType,bool_t secondaryHeaderFlag,uint16_t applicationProcessId,uint8_t sequenceFlags,uint16_t packetSequenceCount);
void PUS_FinalizePacket(uint8_t *target,uint16_t *totalDataLength);

//headers creators + data joiners
bool_t PUS_CreateTcDataField(uint8_t *target,uint16_t targetMaxNb, uint8_t *data, uint16_t dataNb,bool_t isWantedAcknowledgment, bool_t isWantedExecutionResult, uint8_t serviceType, uint8_t serviceSubType, uint16_t sourceId);
bool_t PUS_CreateTmDataField(uint8_t *target,uint16_t targetMaxNb, uint8_t *data, uint16_t dataNb, uint8_t serviceType, uint8_t serviceSubType,uint16_t messageTypeCounter,uint16_t destinationId);

//header join and data
bool_t PUS_JoinTcHeaderAndData(uint8_t *target,uint16_t targetMaxNb,PUS_TcSecondaryHeader_t *header,uint8_t *data, uint16_t dataNb);
bool_t PUS_JoinTmHeaderAndData(uint8_t *target,uint16_t targetMaxNb,PUS_TmSecondaryHeader_t *header,uint8_t *data, uint16_t dataNb);

//header getter
PUS_TcSecondaryHeader_t *PUS_GetTcHeader(uint8_t *packetRaw,uint16_t packetNb);
PUS_TmSecondaryHeader_t *PUS_GetTmHeader(uint8_t *packetRaw,uint16_t packetNb);
uint16_t PUS_GetServiceSubServiceCompound(uint8_t *packetRaw,uint16_t packetNb);

//data getters
uint8_t *PUS_GetTcDataPointer(uint16_t *dataSize,uint8_t *packet,uint16_t pusDataLength);
uint8_t *PUS_GetTmDataPointer(uint16_t *dataSize,uint8_t *packet,uint16_t pusDataLength);

//printers
void PUS_PrintTcHeader(PUS_TcSecondaryHeader_t *self);
void PUS_PrintTmHeader(PUS_TmSecondaryHeader_t *self);
void PUS_PrintAcceptanceReport(PUS_AcceptanceReport_t *self);
void PUS_PrintPacket(uint8_t *packet, uint16_t packetNb);
void PUS_PrintTc(uint8_t *packet, uint16_t packetNb);
void PUS_PrintTm(uint8_t *packet, uint16_t packetNb);

//validators
bool_t PUS_IsPacketSizeValid(uint8_t *packet, uint16_t packetNb);
bool_t PUS_IsPusPusTc(uint8_t *packet, uint16_t packetNb);
bool_t PUS_IsCrcValid(uint8_t *target, uint16_t packetNb,uint16_t *expectedCrc,PUS_Crc_t *foundCrc);

//management
void PUS_SetTmTimeStamp(uint32_t timeStamp);
uint32_t PUS_GetTimeStamp(void);

/* end */
#endif /* LIB_PusUtils_H */

