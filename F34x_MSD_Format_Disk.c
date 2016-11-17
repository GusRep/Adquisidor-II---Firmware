//-----------------------------------------------------------------------------
// F34x_MSD_Format_Disk.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This file contains the functions used by the USB MSD RD example application
// to clean exissting partition
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000037
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
// Includes
//-----------------------------------------------------------------------------

#include "F34x_MSD_Definitions.h"
#include "c8051f340.h"  
#include "F34x_MSD_Format_Disk.h"
#include "F34x_MSD_Sect_Serv.h"
#include <stdio.h>
#include <string.h>
#include "F34x_MSD_Util.h"

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

// See F34x_Format_Disk.h for other function prototypes

static void Clear_FATs();
static void Clear_Dir_Entries();


//-----------------------------------------------------------------------------
// Function Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Format_Disk
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Function is used to formatting disk to FAT16
//-----------------------------------------------------------------------------

void Format_Disk()
{
	Clear_FATs();
	Clear_Dir_Entries();
	return;
}



//-----------------------------------------------------------------------------
// Clear_FATs
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Function clears all FAT
//-----------------------------------------------------------------------------

void Clear_FATs()
{
	unsigned sector;
	Sect_Read(0);
	Sect_Validate();
	memset(Scratch,0,PHYSICAL_BLOCK_SIZE);
	for(sector = Sect_Fat1()+1;sector < Sect_Root_Dir();sector++)
	{
		Sect_Write(sector);
	}
	Scratch[0] = 0xf8;
	Scratch[1] = 0xff;
	Scratch[2] = 0xff;
	Scratch[3] = 0xff;
	Sect_Write(Sect_Fat1());
}

//-----------------------------------------------------------------------------
// Clear_Dir_Entries
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Function clears root directory entries 
//-----------------------------------------------------------------------------

void Clear_Dir_Entries()
{
	unsigned sector;
	memset(Scratch,0,PHYSICAL_BLOCK_SIZE);
	for(sector=Sect_Root_Dir();sector <= Sect_Root_Dir_Last() ;sector++)
	{
		Sect_Write(sector);
	}
}

//-----------------------------------------------------------------------------
// End Of File
//-----------------------------------------------------------------------------
