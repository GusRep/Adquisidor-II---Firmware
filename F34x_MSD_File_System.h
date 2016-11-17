//-----------------------------------------------------------------------------
// F34x_MSD_File_System.h
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Header file with function prototypes relevant to F34x_File_System..c
//
//
// FID:            34X000036
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

#ifndef _FILESYS_H_
#define _FILESYS_H_

#include "F34x_MSD_USB_Main.h"

//-----------------------------------------------------------------------------
// Structure Prototypes
//-----------------------------------------------------------------------------

typedef struct {
	unsigned cluster_start;
	unsigned long sector_current;
	unsigned sector_direntry;
	unsigned offset_direntry;
	BYTE attrib;
	DWORD pos;
	DWORD size;
	BYTE isopen;
} FILE;

typedef WORD UNI; // Unicode character type

typedef union {

struct {
	BYTE seq_nr; // Bit 0..4 = seqnr, ascending, start with 1. Bit 6: final part of name
	UNI unicode1_5[5]; // Little endian
	BYTE attrib; // 0x0F
	BYTE type; // 0x00
	BYTE checksum; // int i;BYTE sum=0;for(i=0;i<11;i++) { sum=(sum>>1)+((sum&1)<<7);sum+=name[i]; }
	UNI unicode6_11[6];
	unsigned starting_cluster; // 0x0000
	UNI unicode12_13[2];
} lfn;

struct {
	char name[11];
	BYTE attrib;
	BYTE reserved[10];
	WORD time; // 5/6/5 bits for h/m/2*sec
	WORD date; // 7/4/5 bits for y-1980/m/d
	unsigned starting_cluster; // 0 for empty file
	DWORD filesize;
} sfn;

} dir_entry;

typedef struct {
	unsigned long block;
	unsigned offset;
	BYTE findempty;
	dir_entry* direntry;
} find_info;



#define ATTRIB_READ_ONLY 	0x01
#define ATTRIB_HIDDEN		0x02
#define ATTRIB_SYSTEM		0x04
#define ATTRIB_LABEL		0x08
#define ATTRIB_SUBDIR		0x10
#define ATTRIB_ARCHIVE		0x20

#define SEEK_CUR 0
#define SEEK_END 1
#define SEEK_SET 2

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

int fopen(FILE* f,char* filename,char* mode);
int fseek(FILE* f,long offset,int origin);
DWORD ftell(FILE* f);
int feof(FILE* f);
unsigned fread(FILE* f,BYTE* buffer,unsigned count);
unsigned fwrite(FILE* f,BYTE* buffer,unsigned count);
void fclose(FILE* f);
int fexists(char* filename);
int fdelete(char* filename);
BYTE findfirst(find_info* findinfo,BYTE empty);
BYTE findnext(find_info* findinfo);


//-------------------------------------------------------------------------------
// Functions only for F340 device
//-------------------------------------------------------------------------------

#ifdef __F340_VER__

#define DIRECTORY_EXISTS		1
#define NO_PLACE_FOR_DIRECTORY	2
#define DIRNAME_LENGTH_ERROR	3

//-----------------------------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------------------------

void FileSys_Init();
BYTE chngdir(char* dirname);
BYTE mkdir(char* dir_name);
BYTE rmdir(char* dir_name) ;
void write_current_dir();

//-----------------------------------------------------------------------------
// Structure Prototypes
//-----------------------------------------------------------------------------

typedef struct 
{ 
	unsigned long block;
	unsigned offset;
}
PREV_SEARCH;


#endif
#endif