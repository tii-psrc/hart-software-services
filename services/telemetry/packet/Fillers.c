/*******************************************************************************/
/* Copyright XXXXXXXXX 1998-YYYY                                               */
/*                                                                             */
/* xxxxxxxx@xxxxx.xxx                                                          */
/*******************************************************************************/

/* system includes-------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* component includes-------------------------------------------------------------*/
#include <BOOT_FillersUser.h>
#include <BOOT_Fillers.h>

/* local macros-------------------------------------------------------------*/
/* none */

/* local types-------------------------------------------------------------*/
/* none */

/* public variables-------------------------------------------------------------*/
/* none */

/* local variables-------------------------------------------------------------*/
/* none */

/* local prototypes-------------------------------------------------------------*/
/* none */

/* public functions-------------------------------------------------------------*/
void BOOTF_FillHkReport(BOOT_HkReport_t *target, CCSDS_PrimaryHeader_t *ccsdsPrimaryHeader, PUS_TmSecondaryHeader_t *pusTmSecondaryHeader, BOOT_EnumHkReportId_t structureId, void *hkData, uint16_t hkDataNb)
{
  memcpy(&target->ccsdsPrimaryHeader, ccsdsPrimaryHeader, sizeof(CCSDS_PrimaryHeader_t));
  memcpy(&target->pusTmSecondaryHeader, pusTmSecondaryHeader, sizeof(PUS_TmSecondaryHeader_t));
  target->structureId = structureId;
  if (hkData != NULL && hkDataNb > 0) {
    memcpy(&target->hkData, hkData, hkDataNb);
  }
}

uint16_t BOOTF_GetSizeForEnumPktType(BOOT_EnumPktType_t valueEnumPktType)
{
  uint16_t size = 0;
  switch (valueEnumPktType)
  {
    default:
      size = 0;
      break;
  }
  return size;
}

uint16_t BOOTF_GetSizeForEnumBoolean(BOOT_EnumBoolean_t valueEnumBoolean)
{
  uint16_t size = 0;
  switch (valueEnumBoolean)
  {
    default:
      size = 0;
      break;
  }
  return size;
}

uint16_t BOOTF_GetSizeForEnumApid(BOOT_EnumApid_t valueEnumApid)
{
  uint16_t size = 0;
  switch (valueEnumApid)
  {
    default:
      size = 0;
      break;
  }
  return size;
}

uint16_t BOOTF_GetSizeForEnumBootDevice(BOOT_EnumBootDevice_t valueEnumBootDevice)
{
  uint16_t size = 0;
  switch (valueEnumBootDevice)
  {
    default:
      size = 0;
      break;
  }
  return size;
}

uint16_t BOOTF_GetSizeForEnumBootStatus(BOOT_EnumBootStatus_t valueEnumBootStatus)
{
  uint16_t size = 0;
  switch (valueEnumBootStatus)
  {
    default:
      size = 0;
      break;
  }
  return size;
}

uint16_t BOOTF_GetSizeForEnumHkReportId(BOOT_EnumHkReportId_t valueEnumHkReportId)
{
  uint16_t size = 0;
  switch (valueEnumHkReportId)
  {
    case BOOT_HKID_BOOT_HK:
      size = sizeof(BOOT_HkBootSw_t);
      break;
    default:
      size = 0;
      break;
  }
  return size;
}

