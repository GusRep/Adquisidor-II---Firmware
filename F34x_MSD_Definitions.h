//-----------------------------------------------------------------------------
// F34x_MSD_Definitions.h
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file with all definitions.
//
//
// FID:            34X000032
// Target:         C8051F34x
// Tool chain:     Keil
// Command Line:   See Readme.txt
// Project Name:   F34x_USB_MSD
//
// Release 1.1
//    -All changes by PKC
//    -09 JUN 2006
//    -Replaced SFR definitions file "c8051f320.h" with "c8051f340.h"
//
// Release 1.0
//    -Initial Release
//

//-----------------------------------------------------------------------------
// Header File Preprocessor Directive
//-----------------------------------------------------------------------------

#ifndef __DEFINITIONS_H__
#define __DEFINITIONS_H__

#define DEBUG_TIMEOUTS
#include "c8051f340.h"
#ifdef DEBUG_TIMEOUTS
sbit START_STOP_SPI = P3^0 ;
sbit START_STOP_READ_TO = P3^1;
sbit START_STOP_WRITE_TO = P3^2;

#define START_SPI_TIMEOUT (START_STOP_SPI = 1)
#define STOP_SPI_TIME_OUT (START_STOP_SPI = 0)
#define START_READ_COPY (START_STOP_READ_TO = 1)
#define STOP_READ_COPY (START_STOP_READ_TO = 0)
#define START_WRITE_COPY (START_STOP_WRITE_TO = 1)
#define STOP_WRITE_COPY (START_STOP_WRITE_TO = 0)
#else

#define START_SPI_TIMEOUT /\
/
#define STOP_SPI_TIME_OUT /\
/
#define START_READ_COPY /\
/
#define STOP_READ_COPY /\
/
#define START_WRITE_COPY /\
/
#define STOP_WRITE_COPY /\
/

#endif

#define ENDLINE 	"\r\n"
#define ENDLINE_SGN '\r'

#endif