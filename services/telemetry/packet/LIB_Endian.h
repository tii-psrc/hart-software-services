/*******************************************************************************/
/* Copyright David Gil 1998-2025                                               */
/* 								                                               */
/* davidgil@dgadv.com 			                                               */
/*******************************************************************************/

#ifndef END_Endian_H
#define END_Endian_H

/* system includes-------------------------------------------------------------*/
/* none */

/* application includes--------------------------------------------------------*/
#include <myTypes.h>

/* component includes----------------------------------------------------------*/
/* none */

/* macros-----------------------------------------------------------------------*/
/* none */

/* types------------------------------------------------------------------------*/
/* none */

/* public variables-------------------------------------------------------------*/
/* none */

/* public functions--------------------------------------------------------------*/
void LEND_Swap(uint8_t* data,uint8_t dataNb);
void LEND_Host2Network(uint8_t* data,uint8_t dataNb);
void LEND_Network2Host(uint8_t* data,uint8_t dataNb);

void LEND_CopySwap(uint8_t *target,uint8_t* data,uint8_t dataNb);
void LEND_CopyHost2Network(uint8_t *target,uint8_t* data,uint8_t dataNb);
void LEND_CopyNetwork2Host(uint8_t *target,uint8_t* data,uint8_t dataNb);

/* end */
#endif /* END_Endian_H */

