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
/* none */

/* component includes----------------------------------------------------------*/
#include <LIB_Endian.h>

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
void LEND_Swap(uint8_t* data,uint8_t dataNb)
{
	uint8_t buffer[sizeof(uint64_t)];
	uint8_t cIx=0;
	if (dataNb<=sizeof(uint64_t))
	{
		for (cIx=0;cIx<dataNb;cIx++)
		{
			buffer[dataNb-cIx-1]=data[cIx];
		}
		memcpy(data,buffer,dataNb);
	}
	else
	{
		printf("ERROR: LEND_Swap length %d > 64 bits",dataNb*8);
	}
}
void LEND_Host2Network(uint8_t* data,uint8_t dataNb)
{
	if (IS_LITTLE_ENDIAN)
	{
		LEND_Swap(data,dataNb);
	}
}
void LEND_Network2Host(uint8_t* data,uint8_t dataNb)
{
	if (IS_LITTLE_ENDIAN)
	{
		LEND_Swap(data,dataNb);
	}
}

void LEND_CopySwap(uint8_t *target,uint8_t* data,uint8_t dataNb)
{
	uint8_t cIx=0;
	if (dataNb<=sizeof(uint64_t))
	{
		for (cIx=0;cIx<dataNb;cIx++)
		{
			target[dataNb-cIx-1]=data[cIx];
		}
		//printf("%d %d\n",target[0],data[0]);
	}
	else
	{
		printf("ERROR: LEND_Swap length %d > 64 bits",dataNb*8);
	}
}
void LEND_CopyHost2Network(uint8_t *target,uint8_t* data,uint8_t dataNb)
{
	if (IS_LITTLE_ENDIAN)
	{
		LEND_CopySwap(target,data,dataNb);
	}
	else
	{
		memcpy(target,data,dataNb);
	}
}
void LEND_CopyNetwork2Host(uint8_t *target,uint8_t* data,uint8_t dataNb)
{
	if (IS_LITTLE_ENDIAN)
	{
		LEND_CopySwap(target,data,dataNb);
	}
	else
	{
		memcpy(target,data,dataNb);
	}
}

/* local functions ------------------------------------------------------------*/
/* none */

/* end */
