//-----------------------------------------------------------------------------
// F34x_MSD_Util.h
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file for F34x_Util.h. It contains functions and variables 
// prototypes.
//
//
// FID:            34X000068
// Target:         C8051F34x
// Tool chain:     Keil
// Command Line:   See Readme.txt
// Project Name:   F34x_USB_MSD
//
// Release 1.1
//    -All changes by PKC
//    -09 JUN 2006
//    -No changes; incremented revision number to match project revision
//
// Release 1.0
//    -Initial Release
//

//-----------------------------------------------------------------------------
// Header File Preprocessor Directive
//-----------------------------------------------------------------------------

#ifndef _UTIL_H_
#define _UTIL_H_

#include "F34x_MSD_USB_Main.h"

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

extern DWORD htonl(DWORD d);
extern unsigned htons(unsigned w);
#define ntohs htons
#define ntohl htonl

//-----------------------------------------------------------------------------
// Variable Prototype
//-----------------------------------------------------------------------------

char* Str_Token(char* str);

//-----------------------------------------------------------------------------
// Macros Prototypes
//-----------------------------------------------------------------------------

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

#define msb(x) (((x)>>8)&0x000000FFul)
#define lsb(x) ((x)&0x000000FFul)

#endif
