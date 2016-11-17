//-----------------------------------------------------------------------------
// F34x_MSD_USB_Descriptor.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Source file for USB firmware. Includes descriptor data.
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000059
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
#include "F34x_MSD_USB_Register.h"
#include "F34x_MSD_USB_Main.h"
#include "F34x_MSD_USB_Descriptor.h"

//---------------------------
// Descriptor Declarations
//---------------------------
const device_descriptor Device_Desc =
{
   0x12,                // bLength
   0x01,                // bDescriptorType
   0x1001,              // bcdUSB
   0x00,                // bDeviceClass
   0x00,                // bDeviceSubClass
   0x00,                // bDeviceProtocol
   EP0_PACKET_SIZE,     // bMaxPacketSize0
   0xC410,              // idVendor
   0x0002,              // idProduct 
   0x0000,              // bcdDevice 
   0x01,                // iManufacturer
   0x00,                // iProduct     
   0x03,                // iSerialNumber
   0x01                 // bNumConfigurations
}; //end of Device_Desc

const configuration_descriptor Config_Desc = 
{
   0x09,                // Length
   0x02,                // Type
   0x2000,              // Totallength
   0x01,                // NumInterfaces
   0x01,                // bConfigurationValue
   0x00,                // iConfiguration
   0x80,                // bmAttributes
   0x0F                 // MaxPower
}; //end of Config_Desc

const interface_descriptor Interface_Desc =
{
   0x09,                // bLength
   0x04,                // bDescriptorType
   0x00,                // bInterfaceNumber
   0x00,                // bAlternateSetting
   0x02,                // bNumEndpoints
   0x08,                // bInterfaceClass // MASS STORAGE DEVICE 
   0x06,                // bInterfaceSubClass // SCSI Transparent command set
   0x50,                // bInterfaceProcotol // BULK-ONLY transport
   0x00                 // iInterface
}; //end of Interface_Desc

const endpoint_descriptor Endpoint1_Desc =
{
   0x07,                // bLength
   0x05,                // bDescriptorType
   0x81,                // bEndpointAddress
   0x02,                // bmAttributes
   EP1_PACKET_SIZE_LE,  // MaxPacketSize (LITTLE ENDIAN)
   0                   // bInterval
}; //end of Endpoint1_Desc

const endpoint_descriptor Endpoint2_Desc =
{
   0x07,                // bLength
   0x05,                // bDescriptorType
#ifdef __F326_VER__
   0x01,
#else
   0x02,		          // bEndpointAddress
#endif
   0x02,                // bmAttributes
   EP2_PACKET_SIZE_LE,  // MaxPacketSize (LITTLE ENDIAN)
   0                   // bInterval
}; //end of Endpoint2_Desc

#define STR0LEN 4

code const BYTE String0_Desc[STR0LEN] =
{
   STR0LEN, 0x03, 0x09, 0x04
}; //end of String0_Desc

#define STR1LEN sizeof("Silicon Laboratories")*2

code const BYTE String1_Desc[STR1LEN] =
{
   STR1LEN, 0x03,
   'S', 0,
   'i', 0,
   'l', 0,
   'i', 0,
   'c', 0,
   'o', 0,
   'n', 0,
   ' ', 0,
   'L', 0,
   'a', 0,
   'b', 0,
   'o', 0,
   'r', 0,
   'a', 0,
   't', 0,
   'o', 0,
   'r', 0,
   'i', 0,
   'e', 0,
   's', 0,
}; //end of String1_Desc

#define STR2LEN sizeof("C8051Fxxx Development Board")*2

code const BYTE String2_Desc[STR2LEN] =
{
   STR2LEN, 0x03,
   'C', 0,
   '8', 0,
   '0', 0,
   '5', 0,
   '1', 0,
   'F', 0,
   'x', 0,
   'x', 0,
   'x', 0,
   ' ', 0,
   'D', 0,
   'e', 0,
   'v', 0,
   'e', 0,
   'l', 0,
   'o', 0,
   'p', 0,
   'm', 0,
   'e', 0,
   'n', 0,
   't', 0,
   ' ', 0,
   'B', 0,
   'o', 0,
   'a', 0,
   'r', 0,
   'd', 0
}; //end of String2_Desc

#define STR3LEN sizeof("0079876543210")*2

code const BYTE String3_Desc[STR3LEN] =
{
   STR3LEN, 0x03,
   '0', 0,
   '0', 0,
   '7', 0,
   '9', 0,
   '8', 0,
   '7', 0,
   '6', 0,
   '5', 0,
   '4', 0,
   '3', 0,
   '2', 0,
   '1', 0,
   '0', 0
}; //end of String2_Desc

BYTE* const String_Desc_Table[] =
{
   String0_Desc,
   String1_Desc,
   String2_Desc,
   String3_Desc
};