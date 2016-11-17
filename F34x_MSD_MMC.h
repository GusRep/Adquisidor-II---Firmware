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
// FID:            34X000044
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

#ifndef _MMC_H_
#define _MMC_H_

// MMC_FLASH Functions



void MMC_FLASH_Init (void);            // Initializes MMC and configures it to 
                                       // accept SPI commands;

unsigned int MMC_FLASH_Block_Read(unsigned long address, unsigned char *pchar);
unsigned char MMC_FLASH_Block_Write(unsigned long address,unsigned char *wdata);

#ifdef __F340_VER__
void Get_Status_MMC();
#endif

#endif