/*******************************************************************************/
/* Copyright XXXXXXXXX 1998-YYYY                                               */
/*                                                                             */
/* xxxxxxxx@xxxxx.xxx                                                          */
/*******************************************************************************/

#ifndef BOOTF_Fillers_H
#define BOOTF_Fillers_H

/* system includes-------------------------------------------------------------*/
/* none */
/* application includes-------------------------------------------------------------*/
#include "myTypes.h"
/* component includes-------------------------------------------------------------*/
#include "BOOT_FillersUser.h"

/* macros-------------------------------------------------------------*/
//packet IDs
#define BOOT_HKREPORT_PACKETID (0x0319)
#define BOOT_HKREPORT_SERVICE ((BOOT_HKREPORT_PACKETID & (0xFF00)) >> 8)
#define BOOT_HKREPORT_SUBSERVICE (BOOT_HKREPORT_PACKETID & (0x00FF))

/* types-------------------------------------------------------------*/
//enumerations
typedef enum _BOOT_EnumPktType_t
{
  BOOT_PKT_TELEMETRY=0,
  BOOT_PKT_TELECOMMAND=1,
  BOOT_PKT_NB=2
} BOOT_EnumPktType_t;

typedef enum _BOOT_EnumBoolean_t
{
  BOOT_BOL_FALSE=0,
  BOOT_BOL_TRUE=1,
  BOOT_BOL_NB=2
} BOOT_EnumBoolean_t;

typedef enum _BOOT_EnumApid_t
{
  BOOT_APID_BOOTSW=34,
  BOOT_APID_OBC=100,
  BOOT_APID_NB=2
} BOOT_EnumApid_t;

typedef enum _BOOT_EnumBootDevice_t
{
  BOOT_BD_MSSW25_NOM=0,
  BOOT_BD_FPGAW25_NOM=1,
  BOOT_BD_MSSW25_RED=2,
  BOOT_BD_FPGAW25_RED=3,
  BOOT_BD_NB=4
} BOOT_EnumBootDevice_t;

typedef enum _BOOT_EnumBootStatus_t
{
  BOOT_BS_BOOTLOADER1_STARTED=0,
  BOOT_BS_DDR_TRAINING_STARTED=1,
  BOOT_BS_DDR_TRAINING_FAILED_REBOOT=2,
  BOOT_BS_BOOTLOADER2_STARTED=3,
  BOOT_BS_BOOTLOADER2_FAILED_REBOOT=4,
  BOOT_BS_LINUX_BOOT_STARTED=5,
  BOOT_BS_LINUX_BOOT_FAILED_REBOOT=6,
  BOOT_BS_LINUX_BOOT_SUCEEDED=7,
  BOOT_BS_NB=8
} BOOT_EnumBootStatus_t;

typedef enum _BOOT_EnumHkReportId_t
{
  BOOT_HKID_ALL=0,
  BOOT_HKID_BOOT_HK=1,
  BOOT_HKID_NB=2
} BOOT_EnumHkReportId_t;


//structures
typedef struct __attribute__((packed)) _BOOT_HkBootSw_t_
{
  uint8_t bootDevice;
  uint8_t bootStatus;
} BOOT_HkBootSw_t;


//packets
typedef struct __attribute__((packed)) _BOOT_HkReport_t_
{
  CCSDS_PrimaryHeader_t ccsdsPrimaryHeader;
  PUS_TmSecondaryHeader_t pusTmSecondaryHeader;
  uint16_t structureId;
  union
  {
   BOOT_HkBootSw_t HkBootSw;
  }hkData;
} BOOT_HkReport_t;


void BOOTF_FillHkReport(BOOT_HkReport_t *target, CCSDS_PrimaryHeader_t *ccsdsPrimaryHeader, PUS_TmSecondaryHeader_t *pusTmSecondaryHeader, BOOT_EnumHkReportId_t structureId, void *hkData, uint16_t hkDataNb);

uint16_t BOOTF_GetSizeForEnumPktType(BOOT_EnumPktType_t valueEnumPktType);
uint16_t BOOTF_GetSizeForEnumBoolean(BOOT_EnumBoolean_t valueEnumBoolean);
uint16_t BOOTF_GetSizeForEnumApid(BOOT_EnumApid_t valueEnumApid);
uint16_t BOOTF_GetSizeForEnumBootDevice(BOOT_EnumBootDevice_t valueEnumBootDevice);
uint16_t BOOTF_GetSizeForEnumBootStatus(BOOT_EnumBootStatus_t valueEnumBootStatus);
uint16_t BOOTF_GetSizeForEnumHkReportId(BOOT_EnumHkReportId_t valueEnumHkReportId);

#endif
