//-----------------------------------------------------------------------------
// F34x_MSD_MMC.h
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file with function prototypes relevant to F34x_MMC.c
//
//
// FID:            34X000047
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

#ifndef _MSD_H_
#define _MSD_H_

#include "F34x_MSD_USB_Main.h"

//-----------------------------------------------------------------------------
// Structure Prototypes
//-----------------------------------------------------------------------------

typedef struct _CBW {
	DWORD dCBWSignature;
	DWORD dCBWTag;
	DWORD dCBWDataTransferLength;
	BYTE  bmCBWFlags;
	BYTE  bCBWLUN;
	BYTE  bCBWCBLength;
	BYTE  CBWCB[16];
} CBW;

typedef struct _CSW {
	DWORD dCSWSignature;
	DWORD dCSWTag;
	DWORD dCSWDataResidue;
	BYTE  bCSWStatus;
} CSW;

extern CBW xdata cbw;
extern CSW xdata csw;

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void Msd_Step(void);

#endif