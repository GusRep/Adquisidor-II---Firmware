//-----------------------------------------------------------------------------
// F34x_MSD_Sect_Serv.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This file contains basic functions for file system control. These low level 
// functions are for FAT file system.
//
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000053
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
// Includes
//-----------------------------------------------------------------------------

#include "F34x_MSD_Definitions.h"
#include "F34x_MSD_Sect_Serv.h"
#include "F34x_MSD_Util.h"
#include <stdio.h>
#include "F34x_MSD_MMC.h"
#include "F34x_MSD_CF_Basic_Functions.h"

static xdata char Is_Compact_Flash;




// Buffer for read/write transfers:
BYTE xdata Scratch[PHYSICAL_BLOCK_SIZE];

// Permanent copy of important fields from bootrecord:
bootrecord_small xdata MBR;

extern xdata unsigned long PHYSICAL_BLOCKS;
// MMC-specific:
#define HIDDEN_SECTORS 0x00 
//0x20
// Size of an entry in the root directory
#define DIRENTRY_SIZE 0x20

//-----------------------------------------------------------------------------
// Functions
//-----------------------------------------------------------------------------

extern void print_scratch();


//----------------------------------------------------------------------------
// Sect_Validate
//----------------------------------------------------------------------------
//
// Function checks the validate of bootrecord.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Sect_Validate(void) reentrant
{
  unsigned  fat_sec = 0;

  bootrecord_large* bootrecord=Scratch;
  MBR.valid=0;
  MBR.hidden_sectors = 0;

  if((bootrecord->signature[0]!=0x55) || (bootrecord->signature[1]!=0xAA)) {
    return;
  }
  if(PHYSICAL_BLOCK_SIZE != ntohs(bootrecord->bytes_per_sector)) {
    goto Check_MBR;
  }
  if(bootrecord->filesystem[0]!='F' || bootrecord->filesystem[1]!='A' || bootrecord->filesystem[2]!='T' || bootrecord->filesystem[3]!='1' || bootrecord->filesystem[4]!='6') {
    goto Check_MBR;
  }

	// Make a permanent copy of the important fields of the bootrecord:
  MBR.fat_copies = bootrecord->fat_copies;
  MBR.root_directory_entries = ntohs(bootrecord->root_directory_entries);
  MBR.number_of_sectors = ntohs(bootrecord->number_of_sectors);
  MBR.sectors_per_fat = ntohs(bootrecord->sectors_per_fat);
  MBR.total_sectors = ntohl(bootrecord->total_sectors);
  MBR.reserved_sectors = ntohs(bootrecord->reserved_sectors);
  MBR.sectors_per_cluster = bootrecord->sectors_per_cluster;
  MBR.valid=1;
  return;
Check_MBR:
	// checks if this sector is not a MBR
  if((Scratch[0x1Be] == 0x80) || (Scratch[0x1Be] == 0x00)) {
	//	partition is active
    fat_sec = *(unsigned*)&Scratch[0x1c6];
    fat_sec = ntohs(fat_sec);
    Sect_Read(fat_sec);
    Sect_Validate();
    MBR.hidden_sectors = fat_sec;		
  }
}

//----------------------------------------------------------------------------
// Sect_Init
//----------------------------------------------------------------------------
//
// Function initializes memory card (Compact Flash or MMC), reads sector 0 and 
// checks the validate of bootrecord.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Sect_Init(void)
{
  unsigned char xdata time_out = 0;
#ifdef __F340_VER__
  int xdata sizel,sizeh;
  Is_Compact_Flash = 0;
  if(Init_CF() != CF_NO_CARD) {
    if(Identify_Drive(Scratch)!= CF_NO_CARD)
    {
      Is_Compact_Flash = 1;
      sizel = (Scratch[115] << 8) | Scratch[114];
      sizeh = (Scratch[117] << 8) | Scratch[116];
      PHYSICAL_BLOCKS = ((unsigned long)sizeh << 16 ) | (sizel&0x0ffff);
    }
  }
  if(!Is_Compact_Flash) {
#else
    Is_Compact_Flash = 0;
#endif
    MMC_FLASH_Init(); 
#ifdef __F340_VER__
  }
#endif
	Sect_Read(0);	
	Sect_Validate();
}


//----------------------------------------------------------------------------
// Sect_Sectors
//----------------------------------------------------------------------------
//
// Returns number of sectors.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

unsigned long Sect_Sectors(void)
{
  return PHYSICAL_BLOCKS;//MBR.number_of_sectors;
}


//----------------------------------------------------------------------------
// Sect_Print
//----------------------------------------------------------------------------
//
// Dumps some information (size, bootrecord, filesystem, etc.)
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Sect_Print(void)
{
  bootrecord_large* xdata bootrecord=Scratch;
  if(!MBR.valid)  {
    printf("ERROR: Bootrecord invalid." ENDLINE);
    return;
  }

  printf("%s size = %lu bytes" ENDLINE,Is_Compact_Flash ? "CF memory" : "Memory", (DWORD)Sect_Sectors()*Sect_Block_Size());

}

//----------------------------------------------------------------------------
// Sect_Read
//----------------------------------------------------------------------------
//
// Reads one sector into Scratch buffer
//
// Parameters   : sector - sector's number
// Return Value : error number
//----------------------------------------------------------------------------

unsigned  Sect_Read(unsigned long sector) 
{
  unsigned xdata error;
#ifdef __F340_VER__
  if(!Is_Compact_Flash) {
#endif
    unsigned char xdata loopguard = 0;
    while((error = MMC_FLASH_Block_Read(sector+HIDDEN_SECTORS,Scratch)) != 0) 
    if(!++loopguard) {
      printf("Unable to Read sector %ld" ENDLINE , sector);
      break;
    }
#ifdef __F340_VER__
  } else {
    error = Read_Sector(sector,Scratch);
  }
#endif
  return error;
}

//----------------------------------------------------------------------------
// Sect_Write
//----------------------------------------------------------------------------
//
// It writes one sector from Scratch buffer
//
// Parameters   : sector - sector's number
// Return Value :
//----------------------------------------------------------------------------

void Sect_Write(unsigned long sector) 
{
  int xdata error;
#ifdef __F340_VER__
  if(!Is_Compact_Flash) {
#endif
    MMC_FLASH_Block_Write(sector+HIDDEN_SECTORS,Scratch); // wozb - 27-09-2005 instead of this write data from uart
#ifdef __F340_VER__
  } else {
    error = Write_Sector(sector,Scratch);
  }
#endif
	// After reformatting by the PC, we must re-init the sector server.
  if(sector==0) {
    Sect_Validate();
  }
}

//----------------------------------------------------------------------------
// Sect_Write_Multi_Fat
//----------------------------------------------------------------------------
//
// Automatically handle multiple FAT copies
//
// Parameters   : sector - sector's number
// Return Value :
//----------------------------------------------------------------------------

void Sect_Write_Multi_Fat(unsigned long sector)
{
  if(sector<Sect_Fat1() || sector>=Sect_Root_Dir()) {
		// This is a 'normal' block, not in the FAT:
    Sect_Write(sector);
  } else {
		// Writing to one of the FAT's will automagically write the same block to the 
		// other FAT copies.
    while(sector>=Sect_Fat2()) // Decrement 'sector' to refer to 1st FAT copy.
		  sector-=MBR.sectors_per_fat;
    while(sector<Sect_Root_Dir()){ // Write same data to each FAT copy.
      Sect_Write(sector);
      sector+=MBR.sectors_per_fat;
    }
  }
}

//----------------------------------------------------------------------------
// Sect_Root_Dir
//----------------------------------------------------------------------------
//
// Returns number of sector for root directory
//
// Parameters   :
// Return Value : number of sectors
//----------------------------------------------------------------------------

unsigned Sect_Root_Dir(void) 
{
  return MBR.hidden_sectors + 
  MBR.reserved_sectors + /* Boot record followed by FATs */ 
  MBR.fat_copies*MBR.sectors_per_fat;
}

//----------------------------------------------------------------------------
// Sect_Root_Dir_Last
//----------------------------------------------------------------------------
//
// Returns number of last sector for root directory
//
// Parameters   :
// Return Value : number of last sector
//----------------------------------------------------------------------------

unsigned Sect_Root_Dir_Last(void)
{
  return Sect_Root_Dir() - 1
      + (MBR.root_directory_entries*DIRENTRY_SIZE)/PHYSICAL_BLOCK_SIZE;
}

//----------------------------------------------------------------------------
// Sect_File_Data
//----------------------------------------------------------------------------
//
// Returns first sector of file data
//
// Parameters   :
// Return Value : number of sector
//----------------------------------------------------------------------------

unsigned Sect_File_Data(void)
{
  return Sect_Root_Dir_Last() + 1 - (MBR.sectors_per_cluster*2); // First file data block is called "number 2".
}


//----------------------------------------------------------------------------
// Sect_Fat1
//----------------------------------------------------------------------------
//
// Returns first sector of 1-st FAT
//
// Parameters   :
// Return Value : number of sector
//
// NOTE: Changed this function in a #define, to safe code memory
//----------------------------------------------------------------------------

unsigned Sect_Fat1(void)
{
  return MBR.hidden_sectors + MBR.reserved_sectors;
}

//----------------------------------------------------------------------------
// Sect_Fat2
//----------------------------------------------------------------------------
//
// Returns first sector of 2-st FAT
//
// Parameters   :
// Return Value : number of sector
//
// NOTE: Changed this function in a #define, to safe code memory
//----------------------------------------------------------------------------

unsigned Sect_Fat2(void)
{
  return MBR.hidden_sectors + MBR.reserved_sectors+MBR.sectors_per_fat;
}

