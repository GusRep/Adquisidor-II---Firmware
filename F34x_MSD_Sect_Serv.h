//-----------------------------------------------------------------------------
// F34x_MSD_Sect_Serv.h
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file with function prototypes relevant to F34x_Sect_Serv.c
//
//
// FID:            34X000054
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

#ifndef _SECT_SERV_H_
#define _SECT_SERV_H_

#include "F34x_MSD_USB_Main.h"
#include "F34x_MSD_Physical_Settings.h"
//#include "F34x_MMC.h"

//-----------------------------------------------------------------------------
// Structure Prototypes
//-----------------------------------------------------------------------------

// This large bootrecord will be overlayed on the physical block that was read.
typedef struct {
  BYTE jmp[3];
  char oem_name[8];
  unsigned bytes_per_sector;
  BYTE sectors_per_cluster;
  unsigned reserved_sectors;
  BYTE fat_copies;
  unsigned root_directory_entries;
  unsigned number_of_sectors;
  BYTE media_descriptor;
  unsigned sectors_per_fat;
  unsigned sectors_per_track;
  unsigned heads;
  DWORD hidden_sectors;
  DWORD total_sectors;
  BYTE drive_number;
  BYTE reserved;
  BYTE extended_signature; 		// 0x29
  BYTE serial_number[4];
  char volume_label[11];
  char filesystem[8];				// "FAT16   "
  BYTE bootstrap[448];
  BYTE signature[2]; 				// 0x55 0xAA
} bootrecord_large;

// This small bootrecord will be in memory as long as the program is running
typedef struct {
	BYTE valid;
	BYTE fat_copies;
	unsigned root_directory_entries;
	unsigned number_of_sectors;
	unsigned sectors_per_fat;
	DWORD total_sectors;
	unsigned reserved_sectors;
	unsigned hidden_sectors;
	BYTE sectors_per_cluster;
} bootrecord_small;

// This is a global 512-byte buffer. 
// Sect_Read() puts data in this buffer.
// Sect_Write() gets data from this buffer.
extern BYTE xdata Scratch[];

// Changed these functions into #define, to safe code memory.
#define Sect_Block_Size() ((unsigned)PHYSICAL_BLOCK_SIZE)
//#define Sect_Fat1() ((unsigned)1)

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void Sect_Init(void);				// Must be called before calling any other Sect_Function.
BYTE Sect_Formatted(void); 			// Returns TRUE is MMC card appears to be correctly formatted.
unsigned long Sect_Sectors(void);	// Returns number of sectors
DWORD Sect_Size(void); 				// Returns number of bytes available on MMC card.
void Sect_Print(void);				// Dumps some info (size, bootrecord, filesystem, etc).
unsigned Sect_Read(unsigned long sector);	// Reads one sector into Scratch buffer.
void Sect_Write(unsigned long sector);	// Write one sector from Scratch buffer.
unsigned Sect_Root_Dir(void); 		// Returns sector number of root directory.
unsigned Sect_Root_Dir_Last(void);	// Last sector number of root directory.
unsigned Sect_File_Data(void);		// First sector of file data.
unsigned Sect_Fat1(void);
unsigned Sect_Fat2(void);			// First sector of 2nd FAT.
void Sect_Write_Multi_Fat(unsigned long sector); // Automatically handle multiple FAT copies.
void Sect_Validate(void) reentrant;

#endif