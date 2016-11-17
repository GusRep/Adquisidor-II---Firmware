//-----------------------------------------------------------------------------
// F34x_MSD_Scsi.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This file contains functions which responses to requests from USB device
//
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000051
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

#include "F34x_MSD_Scsi.h"
#include "F34x_MSD_Msd.h"
#include "F34x_MSD_Util.h"
#include "F34x_MSD_Sect_Serv.h"
#include <stdio.h>

#define SCSI_TEST_UNIT_READY 				0x00
#define SCSI_REQUEST_SENSE 					0x03
#define SCSI_FORMAT_UNIT					0x04
#define SCSI_SEND_DIAGNOSTIC				0x10
#define SCSI_INQUIRY						0x12
#define SCSI_MODE_SELECT_6					0x15
#define SCSI_MODE_SENSE_6					0x1A
#define SCSI_START_STOP_UNIT				0x1B
#define SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL 	0x1E
#define SCSI_READ_CAPACITY_10				0x25
#define SCSI_READ_CAPACITY_16				0x9E
#define SCSI_READ_6							0x08
#define SCSI_READ_10						0x28
#define SCSI_READ_16						0x88
#define SCSI_WRITE_10						0x2A
#define SCSI_VERIFY_10						0x2F
#define SCSI_READ_FORMAT_CAPACITIES 		0x23

BYTE  xdata Scsi_Status;
DWORD xdata Scsi_Residue;

code const BYTE Scsi_Standard_Inquiry_Data[28]= {
  0x00, // Peripheral qualifier & peripheral device type
  0x80, // Removable medium
  0x05, // Version of the standard (2=obsolete, 5=SPC-3)
  0x02, // No NormACA, No HiSup, response data format=2
  0x1F, // No extra parameters
  0x00, // No flags
  0x80, // 0x80 => BQue => Basic Task Management supported
  0x00, // No flags
  'S','i','L','a','b','s',' ',' ', // Requested by Dekimo via www.t10.org
  'M','a','s','s',' ','S','t','o','r','a','g','e'
};

BYTE xdata Scsi_Read_Capacity_10[8]={
  0x00,0x00,0xF4,0x5F, 	// Last logical block address
  0x00,0x00,msb(Sect_Block_Size()),lsb(Sect_Block_Size())	// Block length
};

code const BYTE Scsi_Mode_Sense_6[4]= { 0x03,0,0,0 }; // No mode sense parameter

//----------------------------------------------------------------------------
// Scsi_Send
//----------------------------------------------------------------------------
//
// This function sends defined numbers of bytes via USB
//
// Parameters   : ptr - poiter to sending bytes
//                count - number of sending bytes
// Return Value :
//----------------------------------------------------------------------------

static void Scsi_Send(BYTE* ptr,unsigned count)
{
  if(Scsi_Residue<count) {
  // Under the "thin diagonal":
    Scsi_Status=SCSI_PHASE_ERROR;
    return;
  }
  Scsi_Residue-=count;
  USB_In(ptr,count);
}

//----------------------------------------------------------------------------
// Scsi_Inquiry
//----------------------------------------------------------------------------
//
// This function responses to inquiry from other USB device
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Scsi_Inquiry()
{
  Scsi_Status=SCSI_PASSED;
  Scsi_Send(Scsi_Standard_Inquiry_Data,sizeof(Scsi_Standard_Inquiry_Data));
}

//----------------------------------------------------------------------------
// Scsi_Read_Capacity10
//----------------------------------------------------------------------------
//
// This function responses to capacity informations request
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Scsi_Read_Capacity10()
{
  unsigned int s;
  unsigned long size = Sect_Sectors();
  size-=1;
  s = ((size&0xFFFF0000) >> 16);
  Scsi_Read_Capacity_10[0]=msb((s));
  Scsi_Read_Capacity_10[1]=lsb((s));
  Scsi_Read_Capacity_10[2]=msb(size);
  Scsi_Read_Capacity_10[3]=lsb(size);

  Scsi_Status=SCSI_PASSED;
  Scsi_Send(Scsi_Read_Capacity_10,sizeof(Scsi_Read_Capacity_10));
}

//----------------------------------------------------------------------------
// Scsi_Read10
//----------------------------------------------------------------------------
//
// This function responses to read command
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Scsi_Read10() 
{
  int i,j;
  DWORD xdata d_len = ntohl(cbw.dCBWDataTransferLength);
  DWORD xdata d_LBA =   cbw.CBWCB[2];
  d_LBA<<=8;d_LBA+=cbw.CBWCB[3];
  d_LBA<<=8;d_LBA+=cbw.CBWCB[4];
  d_LBA<<=8;d_LBA+=cbw.CBWCB[5];

  for(i=0;i<(d_len+Sect_Block_Size()-1)/Sect_Block_Size();i++) {
    Sect_Read(d_LBA+i);
    for(j=0;j<(Sect_Block_Size()+EP1_PACKET_SIZE-1)/EP1_PACKET_SIZE;j++) {
      USB_In(Scratch+j*EP1_PACKET_SIZE,EP1_PACKET_SIZE);
      Scsi_Residue-=EP1_PACKET_SIZE;
    }
  }

  Scsi_Status=SCSI_PASSED;
}

//----------------------------------------------------------------------------
// Scsi_Write10
//----------------------------------------------------------------------------
//
// This function responses to write command
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Scsi_Write10() 
{
  int i,j;
  DWORD xdata d_len = ntohl(cbw.dCBWDataTransferLength);
  DWORD xdata d_LBA = cbw.CBWCB[2];
  d_LBA<<=8;d_LBA+=cbw.CBWCB[3];
  d_LBA<<=8;d_LBA+=cbw.CBWCB[4];
  d_LBA<<=8;d_LBA+=cbw.CBWCB[5];

  for(i=0;i<(d_len+Sect_Block_Size()-1)/Sect_Block_Size();i++) {
    START_WRITE_COPY;
    for(j=0;j<(Sect_Block_Size()+EP2_PACKET_SIZE-1)/EP2_PACKET_SIZE;j++) {
      while(!Out_Count);
      Out2_Get_Data(Scratch+j*EP2_PACKET_SIZE);
      Out2_Done();
    }
    STOP_WRITE_COPY;
    Sect_Write(d_LBA+i);
    Scsi_Residue-=Sect_Block_Size();
  }
  Scsi_Status=SCSI_PASSED;
}

//----------------------------------------------------------------------------
// Scsi_Mode_Sense6
//----------------------------------------------------------------------------
//
// This function responses to mode sense information request
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Scsi_Mode_Sense6() 
{
  Scsi_Status=SCSI_PASSED;
  Scsi_Send(Scsi_Mode_Sense_6,sizeof(Scsi_Mode_Sense_6));
}

//----------------------------------------------------------------------------
// Scsi_Rx
//----------------------------------------------------------------------------
//
// This function answers to requests from USB 
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Scsi_Rx() 
{
  int xdata i;

  Scsi_Status=SCSI_FAILED;
  Scsi_Residue=ntohl(cbw.dCBWDataTransferLength);

  if(!cbw.bCBWCBLength) 
    return;

  switch(cbw.CBWCB[0]) { // SCSI Operation code
    case SCSI_TEST_UNIT_READY:
      Scsi_Status=SCSI_PASSED;
      break;
    case SCSI_INQUIRY:
      Scsi_Inquiry();
      break;
    case SCSI_MODE_SENSE_6:
      Scsi_Mode_Sense6();
      break;
    case SCSI_READ_CAPACITY_10:
      Scsi_Read_Capacity10();
      break;
    case SCSI_READ_10:
      Scsi_Read10();
      break;
    case SCSI_WRITE_10:
      Scsi_Write10();
      break;
    case SCSI_VERIFY_10:
      Scsi_Residue=0;
      Scsi_Status=SCSI_PASSED;
      break;
    case SCSI_START_STOP_UNIT:
      Scsi_Status=SCSI_PASSED;
      break;
    case SCSI_PREVENT_ALLOW_MEDIUM_REMOVAL:
      Scsi_Status=SCSI_PASSED;
      break;
    default:
			//printf("Unknown SCSI Cmd (0x%02X).\n",(int)cbw.CBWCB[0]);
      break;
  }

  if(Scsi_Residue && (Scsi_Residue==ntohl(cbw.dCBWDataTransferLength))) {
    for(i=0;i<EP1_PACKET_SIZE;i++) In_Packet[i]=0;
    while(Scsi_Residue) {
      USB_In(In_Packet,(EP1_PACKET_SIZE>Scsi_Residue)?Scsi_Residue:EP1_PACKET_SIZE);
      Scsi_Residue-=((EP1_PACKET_SIZE>Scsi_Residue)?Scsi_Residue:EP1_PACKET_SIZE);
    }
  } 
}
