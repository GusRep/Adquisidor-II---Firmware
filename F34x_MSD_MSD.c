//-----------------------------------------------------------------------------
// F34x_MSD_MSD.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This module contains the crank function. This function is used to checking
// the request and preparing the response action for its. The fact that it's 
// crank function means that in one cycle this function can make only one step.
// That means it can't check and response in this same cycle. It works more 
// like state machine. One step can only repose for actual state and change 
// this state to another. The response for other state occurs in next step.
//
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000046
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

#include "F34x_MSD_USB_Main.h"
#include "F34x_MSD_Msd.h"
#include "F34x_MSD_Scsi.h"
#include "F34x_MSD_Util.h"
#include <stdio.h>

#define DIRECTION_IN	0x80
#define DIRECTION_OUT	0x00

#define CBW_SIGNATURE 0x55534243
#define CSW_SIGNATURE 0x55534253

#define MSD_READY 					0x00
#define MSD_COMMAND_TRANSPORT 		0x01
#define MSD_DATA_IN					0x02
#define MSD_DATA_OUT				0x03
#define MSD_STATUS_TRANSPORT		0x04
#define MSD_DATA					0x05
#define MSD_DO_RESET				0xFF

BYTE xdata Msd_State=MSD_READY;

CBW xdata cbw;
CSW xdata csw;

/*
/* Removed these to safe some memory space (not called on Windows or MAC platform anyway).
/*
void Msd_Reset_(unsigned char itf) {
	itf=0; // Get rid of compiler warning
	Msd_State=MSD_DO_RESET;
}

unsigned char Msd_GetMaxLUN(unsigned char itf) {
	return itf&0; // Only 1 LUN supported (itf&0 gets rid of compiler warning).
}
*/

//----------------------------------------------------------------------------
// Msd_Step
//----------------------------------------------------------------------------
//
// This is a crank function. It checks if something is received and calls the
// responding functions (USB).
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Msd_Step()
{
  switch(Msd_State) {
    case MSD_READY:
      if(Out_Count) {
      // Look for a "valid" and "meaningful" CBW, as defined in the spec:
      // Check size
        if(Out_Count!=sizeof(CBW)) {
          Out2_Get_Data(Out_Packet);
          Out2_Done();
          return;
        }

        Out2_Get_Data((BYTE*)&cbw);
        Out2_Done();

        // Check signature, reserved bits & LUN
        if((cbw.dCBWSignature!=CBW_SIGNATURE) ||
          ((cbw.bmCBWFlags!=DIRECTION_IN && cbw.bmCBWFlags!=DIRECTION_OUT) || (cbw.bCBWLUN&0xF0) || (cbw.bCBWCBLength>16)) ||
          (cbw.bCBWLUN!=0x00)) {
          return;
        }
				
        Msd_State=MSD_DATA;
      }
		  break;

    case MSD_DATA:
      Scsi_Rx();
      Msd_State=MSD_STATUS_TRANSPORT;
      break;

		case MSD_STATUS_TRANSPORT:
			// Reply with a CSW:
      csw.dCSWSignature=CSW_SIGNATURE;
      csw.dCSWTag=cbw.dCBWTag;
      csw.bCSWStatus=Scsi_Status;
      csw.dCSWDataResidue=ntohl(Scsi_Residue);

      USB_In((BYTE*)&csw,sizeof(CSW));

      Msd_State=MSD_READY;
      break;

    case MSD_DO_RESET:
      //printf("RESET! ");
      // Fall-through
    default:
			//printf("Unexpected MSD state!\n");
      Msd_State=MSD_READY;
      break;
  }
}
