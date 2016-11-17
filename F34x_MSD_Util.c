//-----------------------------------------------------------------------------
// F34x_MSD_Util.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This file contains a function which is used to searching the string for
// the first occurrence of whitespace. The whitespace is a special sign like 
// new line, space, etc.
//
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000067
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
#include "F34x_MSD_Util.h"
#include <string.h>
#include "c8051f340.h"

DWORD htonl(DWORD d) {
	DWORD rtn=0;
	rtn|=((d&0xFF000000L)>>24);
	rtn|=((d&0x00FF0000L)>> 8);
	rtn|=((d&0x0000FF00L)<< 8);
	rtn|=((d&0x000000FFL)<<24);
	return rtn;
}

unsigned htons(unsigned w) {
	unsigned rtn=0;
	rtn|=((w&0xFF00u)>>8);
	rtn|=((w&0x00FFu)<<8);
	return rtn;
}

#define whitespace " \t\0\n\r"

//----------------------------------------------------------------------------
// Str_Token
//----------------------------------------------------------------------------
//
// This function searches the whitespaces and returns the address of first
// found whitespace sign.
//
// Parameters   : str - command string
// Return Value : address of found whitespace
//----------------------------------------------------------------------------

char* Str_Token(char* str) {
	static char* s;
	static char old;
	char*rtn;

	if(str) s=str; else *s=old;
	while(*s && strchr(whitespace,*s)) 	// Skip leading whitespace
		s++;
	if(!*s) return 0;
	rtn=s;
	while(*s && !strchr(whitespace,*s))	// Find next whitespace
		s++;
	old=*s;
	*s='\0';
	return rtn;
}
