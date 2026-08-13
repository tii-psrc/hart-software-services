/*******************************************************************************/
/* Copyright XXXXXXXXX 1998-YYYY                                               */
/*                                                                             */
/* xxxxxxxx@xxxxx.xxx                                                          */
/*******************************************************************************/

#ifndef BOOTF_Serializers_H
#define BOOTF_Serializers_H

/* system includes-------------------------------------------------------------*/
/* none */
/* application includes-------------------------------------------------------------*/
#include <myTypes.h>
/* component includes-------------------------------------------------------------*/
#include <BOOT_Fillers.h>
#include <BOOT_SerializersUser.h>

/* macros-------------------------------------------------------------*/
/* none */

/* macros-------------------------------------------------------------*/
/* none */

/* types-------------------------------------------------------------*/
/* none */

/* public variables-------------------------------------------------------------*/
/* none */

/* public functions-------------------------------------------------------------*/
bool_t BOOTS_SerializeHkReport(uint8_t *target, uint16_t targetNb, uint16_t *totalDataSize, BOOT_HkReport_t *structuredData);

#endif
