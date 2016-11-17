//-----------------------------------------------------------------------------
// F34x_MSD_USB_Std_Req.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// This source file contains the subroutines used to handle incoming setup 
// packets. These are called by Handle_Setup in F34x_USB_ISR.c and used for
// USB chapter 9 compliance.
//
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000066
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

#include "c8051f340.h"
#include "F34x_MSD_USB_Register.h"
#include "F34x_MSD_USB_Main.h"
#include "F34x_MSD_USB_Descriptor.h"
#include "F34x_MSD_Msd.h"

extern device_descriptor Device_Desc;            // These are created in F34x_USB_Descriptor.h
extern configuration_descriptor Config_Desc;
extern interface_descriptor Interface_Desc;
extern endpoint_descriptor Endpoint1_Desc;
extern endpoint_descriptor Endpoint2_Desc;
extern BYTE* String_Desc_Table[];

extern setup_buffer Setup;                      // Buffer for current device request information
extern xdata unsigned int Data_Size; 
extern xdata unsigned int Data_Sent;                         
extern BYTE* Data_Ptr;

extern BYTE Ep_Status[];                        // This array contains status bytes for EP 0-2

code BYTE ONES_PACKET[2] = {0x01, 0x00};        // These are response packets used for
code BYTE ZERO_PACKET[2] = {0x00, 0x00};        // communication with host

extern xdata BYTE USB_State;                          // Determines current USB device state


//----------------------------------------------------------------------------
// Get_Status
//----------------------------------------------------------------------------
//
// This routine returns a two byte status packet
// to the host
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Get_Status(void)
{
                                       
   if (Setup.wValue.c[MSB] || Setup.wValue.c[LSB] || 
                                                // If non-zero return length or data length not
   Setup.wLength.c[MSB]    || (Setup.wLength.c[LSB] != 2))  
                                                // equal to 2 then send a stall 
   {                                            // indicating invalid request
      Force_Stall();
   }

   switch(Setup.bmRequestType)                  // Determine if recipient was device, interface, or EP
   {
      case OUT_DEVICE:                          // If recipient was device
         if (Setup.wIndex.c[MSB] || Setup.wIndex.c[LSB])
         {
            Force_Stall();                      // Send stall if request is invalid
         }
         else
         {
            Data_Ptr = (BYTE*)&ZERO_PACKET;      // Otherwise send 0x00, indicating bus power and no
            Data_Size = 2;                       // remote wake-up supported
         }
         break;
      
      case OUT_INTERFACE:                       // See if recipient was interface
         if ((USB_State != DEV_CONFIGURED) ||  
         Setup.wIndex.c[MSB] || Setup.wIndex.c[LSB]) 
                                                // Only valid if device is configured and non-zero index 
         {
            Force_Stall();                      // Otherwise send stall to host
         }
         else
         {
            Data_Ptr = (BYTE*)&ZERO_PACKET;      // Status packet always returns 0x00
            Data_Size = 2;
         }
         break;
  
      case OUT_ENDPOINT:                        // See if recipient was an endpoint
         if ((USB_State != DEV_CONFIGURED) ||
         Setup.wIndex.c[MSB])                   // Make sure device is configured and index msb = 0x00
         {                                      // otherwise return stall to host
            Force_Stall();                      
         }
         else
         {
            if (Setup.wIndex.c[LSB] == IN_EP1)  // Handle case if request is directed to EP 1
            {
               if (Ep_Status[1] == EP_HALT)
               {                                // If endpoint is halted, return 0x01,0x00
                  Data_Ptr = (BYTE*)&ONES_PACKET;
                  Data_Size = 2;
               }
               else
               {
                  Data_Ptr = (BYTE*)&ZERO_PACKET;// Otherwise return 0x00,0x00 to indicate endpoint active
                  Data_Size = 2;
               }
            }
            else
            {
               if (Setup.wIndex.c[LSB] == OUT_EP2)
                                                // If request is directed to endpoint 2, send either
               {                                // 0x01,0x00 if endpoint halted or 0x00,0x00 if 
                  if (Ep_Status[2] == EP_HALT)  // endpoint is active
                  {
                     Data_Ptr = (BYTE*)&ONES_PACKET;
                     Data_Size = 2;
                  }
                  else
                  {
                     Data_Ptr = (BYTE*)&ZERO_PACKET;
                     Data_Size = 2;
                  }
               }
               else
               {
                  Force_Stall();                // Send stall if unexpected data encountered
               }
            }
         }
         break;

      default:
         Force_Stall();
         break;
   }
   if (Ep_Status[0] != EP_STALL)
   {                            
      POLL_WRITE_BYTE(E0CSR, rbSOPRDY);         // Set serviced Setup Packet, Endpoint 0 in                   
      Ep_Status[0] = EP_TX;                     // transmit mode, and reset Data_Sent counter
      Data_Sent = 0;
   }
}

//----------------------------------------------------------------------------
// Clear_Feature
//----------------------------------------------------------------------------
//
// This routine can clear Halt Endpoint features
// on endpoint 1 and 2.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Clear_Feature()
{

   if ((USB_State != DEV_CONFIGURED)          ||// Send procedural stall if device isn't configured
   (Setup.bmRequestType == IN_DEVICE)         ||// or request is made to host(remote wakeup not supported)
   (Setup.bmRequestType == IN_INTERFACE)      ||// or request is made to interface
   Setup.wValue.c[MSB]  || Setup.wIndex.c[MSB]||// or msbs of value or index set to non-zero value
   Setup.wLength.c[MSB] || Setup.wLength.c[LSB])// or data length set to non-zero.
   {
      Force_Stall();
   }

   else
   {             
      if ((Setup.bmRequestType == IN_ENDPOINT)&&// Verify that packet was directed at an endpoint
      (Setup.wValue.c[LSB] == ENDPOINT_HALT)  &&// the feature selected was HALT_ENDPOINT
      ((Setup.wIndex.c[LSB] == IN_EP1) ||       // and that the request was directed at EP 1 in
      (Setup.wIndex.c[LSB] == OUT_EP2)))        // or EP 2 out
      {
         if (Setup.wIndex.c[LSB] == IN_EP1) 
         {
            POLL_WRITE_BYTE (INDEX, EP1_IN_IDX);         // Clear feature endpoint 1 halt
            POLL_WRITE_BYTE (EINCSR1, rbInCLRDT);       
            Ep_Status[1] = EP_IDLE;             // Set endpoint 1 status back to idle                    
         }
         else
         {
            POLL_WRITE_BYTE (INDEX, EP2_OUT_IDX);         // Clear feature endpoint 2 halt
            POLL_WRITE_BYTE (EOUTCSR1, rbOutCLRDT);         
            Ep_Status[2] = EP_IDLE;             // Set endpoint 2 status back to idle
         }
      }
      else
      { 
         Force_Stall();                         // Send procedural stall
      }
   }
   POLL_WRITE_BYTE(INDEX, EP0_IDX);                   // Reset Index to 0
   if (Ep_Status[0] != EP_STALL)
   {
      POLL_WRITE_BYTE(E0CSR, (rbSOPRDY | rbDATAEND));
	                                            // Set Serviced Out packet ready and data end to 
                                                // indicate transaction is over
   }
}

//----------------------------------------------------------------------------
// Set_Feature
//----------------------------------------------------------------------------
//
// This routine will set the EP Halt feature for
// endpoints 1 and 2
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Set_Feature(void)
{

   if ((USB_State != DEV_CONFIGURED)          ||// Make sure device is configured, setup data
   (Setup.bmRequestType == IN_DEVICE)         ||// is all valid and that request is directed at
   (Setup.bmRequestType == IN_INTERFACE)      ||// an endpoint
   Setup.wValue.c[MSB]  || Setup.wIndex.c[MSB]|| 
   Setup.wLength.c[MSB] || Setup.wLength.c[LSB])
   {
      Force_Stall();                            // Otherwise send stall to host
   }

   else
   {             
      if ((Setup.bmRequestType == IN_ENDPOINT)&&// Make sure endpoint exists and that halt
      (Setup.wValue.c[LSB] == ENDPOINT_HALT)  &&// endpoint feature is selected
      ((Setup.wIndex.c[LSB] == IN_EP1)        || 
      (Setup.wIndex.c[LSB] == OUT_EP2)))
      {
         if (Setup.wIndex.c[LSB] == IN_EP1) 
         {
            POLL_WRITE_BYTE (INDEX, EP1_IN_IDX);         // Set feature endpoint 1 halt
            POLL_WRITE_BYTE (EINCSR1, rbInSDSTL);       
            Ep_Status[1] = EP_HALT;                                  
         }
         else
         {
            POLL_WRITE_BYTE (INDEX, EP2_OUT_IDX);         // Set feature Ep2 halt
            POLL_WRITE_BYTE (EOUTCSR1, rbOutSDSTL);         
            Ep_Status[2] = EP_HALT;  		    
         }
      }
      else
      { 
         Force_Stall();                         // Send procedural stall
      }
   }   
   POLL_WRITE_BYTE(INDEX, EP0_IDX);
   if (Ep_Status[0] != EP_STALL)
   {
      POLL_WRITE_BYTE(E0CSR, (rbSOPRDY | rbDATAEND)); 
                                                // Indicate setup packet has been serviced
   }
}

//----------------------------------------------------------------------------
// Set_Address
//----------------------------------------------------------------------------
//
// Set new function address
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Set_Address(void)
{  
   if ((Setup.bmRequestType != IN_DEVICE)     ||// Request must be directed to device
   Setup.wIndex.c[MSB]  || Setup.wIndex.c[LSB]||// with index and length set to zero.
   Setup.wLength.c[MSB] || Setup.wLength.c[LSB]|| 
   Setup.wValue.c[MSB]  || (Setup.wValue.c[LSB] & 0x80))
   {
     Force_Stall();                             // Send stall if setup data invalid
   }

   Ep_Status[0] = EP_ADDRESS;                   // Set endpoint zero to update address next status phase
   if (Setup.wValue.c[LSB] != 0) 
   {
      USB_State = DEV_ADDRESS;                  // Indicate that device state is now address
   }
   else 
   {
      USB_State = DEV_DEFAULT;                  // If new address was 0x00, return device to default
   }                                            // state
   if (Ep_Status[0] != EP_STALL)
   {    
      POLL_WRITE_BYTE(E0CSR, (rbSOPRDY | rbDATAEND)); 
                                                // Indicate setup packet has been serviced
   }
}

//----------------------------------------------------------------------------
// Get_Descriptor
//----------------------------------------------------------------------------
//
// This routine sets the data pointer and size to correct
// descriptor and sets the endpoint status to transmit
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Get_Descriptor(void)
{

   switch(Setup.wValue.c[MSB])                  // Determine which type of descriptor
   {                                            // was requested, and set data ptr and 
      case DSC_DEVICE:                          // size accordingly
         Data_Ptr = (BYTE*) &Device_Desc;
         Data_Size = Device_Desc.bLength;
         break;
      
      case DSC_CONFIG:
         Data_Ptr = (BYTE*) &Config_Desc;
                                                // Compiler Specific - The next statement reverses the
                                                // bytes in the configuration descriptor for the compiler
         Data_Size = Config_Desc.wTotalLength.c[MSB] + 256*Config_Desc.wTotalLength.c[LSB];
         break;
      
	  case DSC_STRING:
         Data_Ptr = String_Desc_Table[Setup.wValue.c[LSB]];
		                                        // Can have a maximum of 255 strings
         Data_Size = *Data_Ptr;
         break;
      
      case DSC_INTERFACE:
         Data_Ptr = (BYTE*) &Interface_Desc;
         Data_Size = Interface_Desc.bLength;
         break;
      
      case DSC_ENDPOINT:
         if ((Setup.wValue.c[LSB] == IN_EP1) || 
         (Setup.wValue.c[LSB] == OUT_EP2))
         {
            if (Setup.wValue.c[LSB] == IN_EP1)
            {
               Data_Ptr = (BYTE*) &Endpoint1_Desc;
               Data_Size = Endpoint1_Desc.bLength;
            }
            else
            {
               Data_Ptr = (BYTE*) &Endpoint2_Desc;
               Data_Size = Endpoint2_Desc.bLength;
            }
         }
         else
         {
            Force_Stall();
         }
         break;
      
      default:
         Force_Stall();                         // Send Stall if unsupported request
         break;
   }
   
   if (Setup.wValue.c[MSB] == DSC_DEVICE ||     // Verify that the requested descriptor is 
   Setup.wValue.c[MSB] == DSC_CONFIG     ||     // valid
   Setup.wValue.c[MSB] == DSC_STRING     ||
   Setup.wValue.c[MSB] == DSC_INTERFACE  ||
   Setup.wValue.c[MSB] == DSC_ENDPOINT)
   {
      if ((Setup.wLength.c[LSB] < Data_Size) && 
      (Setup.wLength.c[MSB] == 0))
      {
         Data_Size = Setup.wLength.i;       // Send only requested amount of data
      }
   }
   if (Ep_Status[0] != EP_STALL)                // Make sure endpoint not in stall mode
   {
     POLL_WRITE_BYTE(E0CSR, rbSOPRDY);          // Service Setup Packet
     Ep_Status[0] = EP_TX;                      // Put endpoint in transmit mode
     Data_Sent = 0;                              // Reset Data Sent counter
   }
}


//----------------------------------------------------------------------------
// Get_Configuration
//----------------------------------------------------------------------------
//
// This routine returns current configuration value
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Get_Configuration(void)
{
   if ((Setup.bmRequestType != OUT_DEVICE)    ||// This request must be directed to the device
   Setup.wValue.c[MSB]  || Setup.wValue.c[LSB]||// with value word set to zero
   Setup.wIndex.c[MSB]  || Setup.wIndex.c[LSB]||// and index set to zero
   Setup.wLength.c[MSB] || (Setup.wLength.c[LSB] != 1))// and setup length set to one
   {
      Force_Stall();                            // Otherwise send a stall to host
   }

   else 
   {
      if (USB_State == DEV_CONFIGURED)          // If the device is configured, then return value 0x01
      {                                         // since this software only supports one configuration
         Data_Ptr = (BYTE*)&ONES_PACKET;
         Data_Size = 1;
      }
      if (USB_State == DEV_ADDRESS)             // If the device is in address state, it is not
      {                                         // configured, so return 0x00
         Data_Ptr = (BYTE*)&ZERO_PACKET;
         Data_Size = 1;
      }
   }
   if (Ep_Status[0] != EP_STALL)
   {
      POLL_WRITE_BYTE(E0CSR, rbSOPRDY);         // Set Serviced Out Packet bit
      Ep_Status[0] = EP_TX;                     // Put endpoint into transmit mode
      Data_Sent = 0;                             // Reset Data Sent counter to zero
   }
}

//----------------------------------------------------------------------------
// Set_Configuration
//----------------------------------------------------------------------------
//
// This routine allows host to change current
// device configuration value
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Set_Configuration(void)
{

   if ((USB_State == DEV_DEFAULT)             ||// Device must be addressed before configured
   (Setup.bmRequestType != IN_DEVICE)         ||// and request recipient must be the device
   Setup.wIndex.c[MSB]  || Setup.wIndex.c[LSB]||// the index and length words must be zero
   Setup.wLength.c[MSB] || Setup.wLength.c[LSB] || 
   Setup.wValue.c[MSB]  || (Setup.wValue.c[LSB] > 1))// This software only supports config = 0,1
   {
      Force_Stall();                            // Send stall if setup data is invalid
   }

   else
   {
      if (Setup.wValue.c[LSB] > 0)              // Any positive configuration request
      {                                         // results in configuration being set to 1
         USB_State = DEV_CONFIGURED;
         Ep_Status[1] = EP_IDLE;                // Set endpoint status to idle (enabled)
         Ep_Status[2] = EP_IDLE;
         POLL_WRITE_BYTE(INDEX, EP1_IN_IDX);             // Change index to endpoint 1
         POLL_WRITE_BYTE(EINCSR2, rbInDIRSEL);  // Set DIRSEL to indicate endpoint 1 is IN
//	Not necessary for Mass Storage Device (bulk data)
//         Handle_In1();                          // Put first data packet on fifo
//
         POLL_WRITE_BYTE(INDEX, EP0_IDX);             // Set index back to endpoint 0
      }
      else
      {
         USB_State = DEV_ADDRESS;               // Unconfigures device by setting state to 
         Ep_Status[1] = EP_HALT;                // address, and changing endpoint 1 and 2 
         Ep_Status[2] = EP_HALT;                // status to halt
      }
   }     
   if (Ep_Status[0] != EP_STALL)
   {
      POLL_WRITE_BYTE(E0CSR, (rbSOPRDY | rbDATAEND)); 
                                                // Indicate setup packet has been serviced
   }
}

//----------------------------------------------------------------------------
// Get_Interface
//----------------------------------------------------------------------------
//
// This routine returns 0x00, since only one interface
// is supported by this firmware
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Get_Interface(void)
{

   if ((USB_State != DEV_CONFIGURED)      ||    // If device is not configured
   (Setup.bmRequestType != OUT_INTERFACE) ||    // or recipient is not an interface
   Setup.wValue.c[MSB]  ||Setup.wValue.c[LSB] ||// or non-zero value or index fields
   Setup.wIndex.c[MSB]  ||Setup.wIndex.c[LSB] ||// or data length not equal to one
   Setup.wLength.c[MSB] ||(Setup.wLength.c[LSB] != 1))    
   {
      Force_Stall();                            // Then return stall due to invalid request
   }

   else
   {
      Data_Ptr = (BYTE*)&ZERO_PACKET;            // Otherwise, return 0x00 to host
      Data_Size = 1;
   }
   if (Ep_Status[0] != EP_STALL)
   {                       
      POLL_WRITE_BYTE(E0CSR, rbSOPRDY);         // Set Serviced Setup packet, put endpoint in transmit
      Ep_Status[0] = EP_TX;                     // mode and reset Data sent counter
      Data_Sent = 0;
   }
}

//----------------------------------------------------------------------------
// Set_Interface
//----------------------------------------------------------------------------
//
// This function sets interface if it's supported
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Set_Interface(void)
{
   if ((Setup.bmRequestType != IN_INTERFACE)  ||// Make sure request is directed at interface
   Setup.wLength.c[MSB] ||Setup.wLength.c[LSB]||// and all other packet values are set to zero
   Setup.wValue.c[MSB]  ||Setup.wValue.c[LSB] || 
   Setup.wIndex.c[MSB]  ||Setup.wIndex.c[LSB])
   {
      Force_Stall();                            // Othewise send a stall to host
   }
   if (Ep_Status[0] != EP_STALL)
   {
      POLL_WRITE_BYTE(E0CSR, (rbSOPRDY | rbDATAEND)); 
                                                // Indicate setup packet has been serviced
   }
}

/*
/* Removed these to safe memory (not called on Windows platforms anyway).
/*
void Reset_Msd(void) {
	// Parse this class-specific request
	if((Setup.bmRequestType==0x21) && (Setup.wValue.i==0x00) && (Setup.wLength.i==0x00)) {
		Msd_Reset_(Setup.wIndex.i);
	} else {
		Force_Stall(); // Not what we expected.
	}
}

void Get_MaxLUN(void) {
	unsigned char maxlun;
	// Parse this class-specific request
	if((Setup.bmRequestType==0xA1) && (Setup.wValue.i==0x00) && (Setup.wLength.i==0x01)) {
		// Return max lun to host:
		maxlun = Msd_GetMaxLUN(Setup.wIndex.i);
		Data_Ptr = (BYTE*)&maxlun;
		Data_Size = 1;
		if (Ep_Status[0] != EP_STALL) {                       
			POLL_WRITE_BYTE(E0CSR, rbSOPRDY);         // Set Serviced Setup packet, put endpoint in transmit
		    Ep_Status[0] = EP_TX;                     // mode and reset Data sent counter
		    Data_Sent = 0;
		}		
	} else {
		Force_Stall(); // Not what we expected.
	}
}
*/