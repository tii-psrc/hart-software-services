/*******************************************************************************/
/* Copyright XXXXXXXXX 1998-YYYY                                               */
/*                                                                             */
/* xxxxxxxx@xxxxx.xxx                                                          */
/*******************************************************************************/

/* system includes-------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* application includes includes-------------------------------------------------------------*/
#include <LIB_Endian.h>

/* component includes-------------------------------------------------------------*/
#include <BOOT_Serializers.h>

/* local macros-------------------------------------------------------------*/
/* none */

/* local types-------------------------------------------------------------*/
/* none */

/* public variables-------------------------------------------------------------*/
/* none */

/* local variables-------------------------------------------------------------*/
/* none */

/* local prototypes-------------------------------------------------------------*/
bool_t BOOTS_SerializeHkBootSw(uint8_t *target, uint16_t targetNb, uint16_t *totalDataSize, BOOT_HkBootSw_t *structuredData);

/* public functions-------------------------------------------------------------*/
bool_t BOOTS_SerializeHkReport(uint8_t *target, uint16_t targetNb, uint16_t *totalDataSize, BOOT_HkReport_t *structuredData)
{
  bool_t isValid = M_TRUE;
  *totalDataSize = 0;

  //ccsdsPrimaryHeader
  BOOTSU_SerializeCCSDS_PrimaryHeader(target, targetNb, totalDataSize, &structuredData->ccsdsPrimaryHeader);
  //pusTmSecondaryHeader
  BOOTSU_SerializePUS_TmSecondaryHeader(target, targetNb, totalDataSize, &structuredData->pusTmSecondaryHeader);
  //structureId
  memcpy(&target[*totalDataSize], &structuredData->structureId, sizeof(uint16_t));
  LEND_Host2Network(&target[*totalDataSize], sizeof(uint16_t));
  *totalDataSize += sizeof(uint16_t);
  //hkData
  if (structuredData->structureId == BOOT_HKID_BOOT_HK)
  {
    BOOTS_SerializeHkBootSw(target, targetNb, totalDataSize, (BOOT_HkBootSw_t*)&structuredData->hkData);
  }

  return isValid;
}

/* local functions-------------------------------------------------------------*/
bool_t BOOTS_SerializeHkBootSw(uint8_t *target, uint16_t targetNb, uint16_t *totalDataSize, BOOT_HkBootSw_t *structuredData)
{
  bool_t isValid = M_TRUE;

  //bootDevice
  memcpy(&target[*totalDataSize], &structuredData->bootDevice, sizeof(uint8_t));
  *totalDataSize += sizeof(uint8_t);
  //bootStatus
  memcpy(&target[*totalDataSize], &structuredData->bootStatus, sizeof(uint8_t));
  *totalDataSize += sizeof(uint8_t);

  return isValid;
}

