//-----------------------------------------------------------------------------
// F34x_MSD_Physical_Settings.h
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file with common definitions
//
//
// FID:            34X000048
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

#ifndef __PHYSICAL_SETTINGS_H__
#define __PHYSICAL_SETTINGS_H__

#include "c8051f340.h"

// Physical size in bytes of one MMC FLASH sector
#define PHYSICAL_BLOCK_SIZE     512   

sbit SCLK = P0^0;
sbit SCS  = P0^3;
sbit MISO = P0^1;
sbit MOSI = P0^2;

#endif