/*******************************************************************************/
/* Copyright David Gil 1998-2025                                               */
/* 								                                               */
/* davidgil@dgadv.com 			                                               */
/*******************************************************************************/
#ifndef MYTYPES_H
#define MYTYPES_H

/* system includes-------------------------------------------------------------*/
#include <stdlib.h>
#include <stdint.h>

/* application includes--------------------------------------------------------*/
/* none */

/* component includes----------------------------------------------------------*/
/* none */

/* macros-----------------------------------------------------------------------*/
#define M_FALSE (0)
#define M_TRUE (1)
#define ERROR (-1)
#define NO_ERROR (0)

#define IS_LITTLE_ENDIAN (1)
//#define UINT16_MAX (65535)
//#define UINT32_MAX (4294967295)

#define SMALL_STRING_LEN (30)
#define MEDIUM_STRING_LEN (80)

/* types------------------------------------------------------------------------*/
typedef int8_t bool_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef __uint64_t uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long int64_t;

typedef char shortText_t[40];
typedef char mediumText_t[255];
typedef char longText_t[1024];

typedef float float32_t;
typedef double float64_t;

/* public variables-------------------------------------------------------------*/
/* none */

/* public functions--------------------------------------------------------------*/
/* none */


/* end */
#endif /* MYTYPES_H */
