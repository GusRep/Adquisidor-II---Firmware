//-----------------------------------------------------------------------------
// F34x_MSD_USB_ISR.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// Source file for USB firmware. Includes top level isr with Setup,
// and Endpoint data handlers.  Also includes routine for USB suspend,
// reset, and procedural stall.
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000061
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
#include <stdio.h>

xdata BYTE USB_State;                         // Holds the current USB State def. in F34x_USB_Main.h

setup_buffer Setup;                     // Buffer for current device request information

xdata unsigned int Data_Size;                  // Size of data to return
xdata unsigned int Data_Sent;                  // Amount of data sent so far
BYTE* Data_Ptr;                          // Pointer to data to return

BYTE Ep_Status[3] = {EP_IDLE, EP_IDLE, EP_IDLE};             
                                        // Holds the status for each endpoint

unsigned xdata Out_Count;
BYTE xdata Out_Packet[EP2_PACKET_SIZE];

BYTE xdata In_count;
BYTE xdata In_Packet[EP1_PACKET_SIZE];
//BYTE xdata In_Overrun;

//----------------------------------------------------------------------------
// USB_ISR
//----------------------------------------------------------------------------
//
// Called after any USB type interrupt, this handler determines which type
// of interrupt occurred, and calls the specific routine to handle it.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void USB_ISR(void) interrupt 8          // Top-level USB ISR
{
   BYTE bCommon, bIn, bOut;
   POLL_READ_BYTE(CMINT, bCommon);      // Read all interrupt registers
   POLL_READ_BYTE(IN1INT, bIn);         // this read also clears the register
   POLL_READ_BYTE(OUT1INT, bOut);
   {
      if (bCommon & rbRSUINT)           // Handle Resume interrupt
      {
         USB_Resume();
      }
      if (bCommon & rbRSTINT)           // Handle Reset interrupt
      {
         USB_Reset();
      }
      if (bIn & rbEP0)                  // Handle Setup packet received
      {                                 // or packet transmitted if Endpoint 0 is
         Handle_Setup();                // transmit mode
      }
      if (bIn & rbIN1)                  // Handle In Packet sent, put new data on
      {                                 // endpoint 1 fifo
		 In_count=0;
      }
      if (bOut & rbOUT2)                // Handle Out packet received, take data off
      {                                 // endpoint 2 fifo
	 	Handle_Out2();
      }   
	  if (bCommon & rbSUSINT)           // Handle Suspend interrupt
      {
         USB_Suspend();
      }
   }
} 

//----------------------------------------------------------------------------
// USB_Resume
//----------------------------------------------------------------------------
//
// Resume normal USB operation
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void USB_Resume(void)                   // Add code to turn on anything turned off when
{                                       // entering suspend mode
   volatile int k;
   k++;
}


//----------------------------------------------------------------------------
// USB_Reset
//----------------------------------------------------------------------------
//
// - Set state to default
// - Clear USB Inhibit bit
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void USB_Reset(void)
{
   USB_State = DEV_DEFAULT;             // Set device state to default

   POLL_WRITE_BYTE(POWER, 0x01);        // Clear USB inhibit bit to enable USB
                                        // suspend detection

   Ep_Status[0] = EP_IDLE;              // Set default Endpoint Status
   Ep_Status[1] = EP_HALT;
   Ep_Status[2] = EP_HALT;
}


//----------------------------------------------------------------------------
// Handle_Setup
//----------------------------------------------------------------------------
//
// - Decode Incoming Setup requests
// - Load data packets on fifo while in transmit mode
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Handle_Setup(void)
{                                   
   BYTE control_reg,TempReg;             // Temporary storage for EP control register

   POLL_WRITE_BYTE(INDEX, EP0_IDX);     // Set Index to Endpoint Zero
   POLL_READ_BYTE(E0CSR, control_reg);   // Read control register

   if (Ep_Status[0] == EP_ADDRESS)      // Handle Status Phase of Set Address command
   {
      POLL_WRITE_BYTE(FADDR, Setup.wValue.c[LSB]);
      Ep_Status[0] = EP_IDLE;
   }

   if (control_reg & rbSTSTL)            // If last packet was a sent stall, reset STSTL
   {                                    // bit and return EP0 to idle state
      POLL_WRITE_BYTE(E0CSR, 0);
      Ep_Status[0] = EP_IDLE;
      return;
   }

   if (control_reg & rbSUEND)            // If last setup transaction was ended prematurely
   {                                    // then set
      POLL_WRITE_BYTE(E0CSR, rbDATAEND);
      POLL_WRITE_BYTE(E0CSR, rbSSUEND); // Serviced Setup End bit and return EP0
      Ep_Status[0] = EP_IDLE;           // to idle state
   }

   if (Ep_Status[0] == EP_IDLE)         // If Endpoint 0 is in idle mode
   {
      if (control_reg & rbOPRDY)         // Make sure that EP 0 has an Out Packet ready from host
      {                                 // although if EP0 is idle, this should always be the case
         Fifo_ReadC(FIFO_EP0, 8, (BYTE *)&Setup);
                                        // Get Setup Packet off of Fifo, it is currently Big-Endian

                                        // Compiler Specific - these next three statements swap the
										// bytes of the setup packet words to Big Endian so they
										// can be compared to other 16-bit values elsewhere properly
         Setup.wValue.i  = Setup.wValue .c[MSB] + 256*Setup.wValue.c[LSB];
         Setup.wIndex.i  = Setup.wIndex .c[MSB] + 256*Setup.wIndex.c[LSB];
         Setup.wLength.i = Setup.wLength.c[MSB] + 256*Setup.wLength.c[LSB];
              
         switch(Setup.bRequest)         // Call correct subroutine to handle each kind of 
         {                              // standard request
            case GET_STATUS:
               Get_Status();
               break;             
            case CLEAR_FEATURE:
               Clear_Feature();
               break;
            case SET_FEATURE:
               Set_Feature();
               break;
            case SET_ADDRESS:
               Set_Address();
               break;
            case GET_DESCRIPTOR:
               Get_Descriptor();
               break;
            case GET_CONFIGURATION:
               Get_Configuration();
               break;
            case SET_CONFIGURATION:
               Set_Configuration();
               break;
            case GET_INTERFACE:
               Get_Interface();
               break;
            case SET_INTERFACE:
               Set_Interface();
               break;
/*
/* Removed these to safe some memory space (not called on Windows platform anyway).
/*
			case MSD_RESET:
				Reset_Msd();
				break;
			case MSD_GET_MAX_LUN:
				Get_MaxLUN();
				break;
*/
            default:
               Force_Stall();           // Send stall to host if invalid request
               break;
         }
      }
   } 

   if (Ep_Status[0] == EP_TX)           // See if the endpoint has data to transmit to host
   {
      if (!(control_reg & rbINPRDY))     // Make sure you don't overwrite last packet
      {
                                        // Endpoint 0 transmit mode
         //Delay();
         POLL_READ_BYTE(E0CSR, control_reg);
                                        // Read control register
		 
         if ((!(control_reg & rbSUEND)) || (!(control_reg & rbOPRDY)))
                                        // Check to see if Setup End or Out Packet received, if so			                      
                                        // do not put any new data on FIFO
         {     
            TempReg = rbINPRDY;         // Add In Packet ready flag to E0CSR bitmask              
			 
			                            // Break Data into multiple packets if larger than Max Packet
            if (Data_Size >= EP0_PACKET_SIZE)
            {
               Fifo_Write(FIFO_EP0, EP0_PACKET_SIZE, (BYTE *)Data_Ptr);// Put Data on Fifo
               Data_Ptr  += EP0_PACKET_SIZE;                           // Advance data pointer
               Data_Size -= EP0_PACKET_SIZE;                           // Decrement data size
               Data_Sent += EP0_PACKET_SIZE;                           // Increment data sent counter
            }
			else                        // If data is less than Max Packet size or zero
            {
               Fifo_Write(FIFO_EP0, Data_Size, (BYTE *)Data_Ptr);       // Put Data on Fifo
               TempReg |= rbDATAEND;                                  // Add Data End bit to bitmask
               Ep_Status[0] = EP_IDLE;                                // Return EP 0 to idle state
            }
            if (Data_Sent == Setup.wLength.i)
			                            // This case exists when the host requests an even multiple of
                                        // your endpoint zero max packet size, and you need to exit
                                        // transmit mode without sending a zero length packet
            {
               TempReg |= rbDATAEND;    // Add Data End bit to mask
               Ep_Status[0] = EP_IDLE;  // and return Endpoint 0 to an idle state
            }
            POLL_WRITE_BYTE(E0CSR, TempReg);                          // Write mask to E0CSR
         }
      }
   }
}


//----------------------------------------------------------------------------
// Handle_In1
//----------------------------------------------------------------------------
//
// - This routine loads the current value from In_Packet on the Endpoint 1 
// fifo, after an interrupt is received from the last packet being 
// transmitted
//
// Parameters   : ptr_buf - pointer to buffer with in_pocket
// Return Value :
//----------------------------------------------------------------------------

void Handle_In1(BYTE* ptr_buf)
{
   BYTE control_reg;

   POLL_WRITE_BYTE(INDEX, EP1_IN_IDX);           // Set index to endpoint 1 registers
   POLL_READ_BYTE(EINCSR1, control_reg); // Read contol register for EP 1

   if (Ep_Status[1] == EP_HALT)         // If endpoint is currently halted, send a stall
   {
      POLL_WRITE_BYTE(EINCSR1, rbInSDSTL);
   }

   else                                 // Otherwise send last updated data to host
   {
      if (control_reg & rbInSTSTL)       // Clear sent stall if last packet returned a stall
      {
         POLL_WRITE_BYTE(EINCSR1, rbInCLRDT);
      }

      if (control_reg & rbInUNDRUN)      // Clear underrun bit if it was set
      {
         POLL_WRITE_BYTE(EINCSR1, 0x00);
      }

                                        // Put new data on Fifo
//      Fifo_Write(FIFO_EP1, EP1_PACKET_SIZE, (BYTE *)IN_PACKET);
      Fifo_Write(FIFO_EP1, In_count, (BYTE *)ptr_buf);
      POLL_WRITE_BYTE(EINCSR1, rbInINPRDY);   
                                        // Set In Packet ready bit, indicating fresh data
   }                                    // on Fifo 1
}

//----------------------------------------------------------------------------
// USB_Bulk_Init
//----------------------------------------------------------------------------
//
// Function rests the input and output counters for USB
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void USB_Bulk_Init() {
	In_count=0;
	Out_Count=0;
}

//----------------------------------------------------------------------------
// USB_In
//----------------------------------------------------------------------------
//
// The function sends the series of characters via USB
//
// Parameters   : ptr_buf - pointer to buffer, 
//                count - number of bytes
// Return Value :
//----------------------------------------------------------------------------

void USB_In(BYTE* ptr_buf,unsigned count) {
	DWORD t1=tickcount+500;
	while(In_count && (tickcount<t1))
		;
	if(In_count) {
		return;
	}
	In_count=count;
	Handle_In1(ptr_buf);
}


//----------------------------------------------------------------------------
// Handle_Out2
//----------------------------------------------------------------------------
//
// Take the received packet from the host off the fifo and put it into the 
// Out_Packet array
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Handle_Out2() {
   BYTE count=0;
   BYTE control_reg;

   POLL_WRITE_BYTE(INDEX, EP2_OUT_IDX);    // Set index to endpoint 2 registers
   POLL_READ_BYTE(EOUTCSR1, control_reg);

   if (Ep_Status[2] == EP_HALT)         // If endpoint is halted, send a stall
   {
      POLL_WRITE_BYTE(EOUTCSR1, rbOutSDSTL);
   }

   else                                 // Otherwise read received packet from 
                                        // host
   {
      if (control_reg & rbOutSTSTL)      // Clear sent stall bit if last packet 
                                         //was a stall
      {
         POLL_WRITE_BYTE(EOUTCSR1, rbOutCLRDT);
      }
	  
      POLL_READ_BYTE(EOUTCNTL, count);
	  Out_Count=count;
	  POLL_READ_BYTE(EOUTCNTH, count);
	  Out_Count|=((unsigned)count)<<8;

	 
//		FOR MSD, the host does not send EP2_PACKET_SIZE bytes, but rather 31 bytes
//      if (count != EP2_PACKET_SIZE)     // If host did not send correct packet 
                                          // size, flush buffer
//      {
//         POLL_WRITE_BYTE(EOUTCNTL, rbOutFLUSH); 
//      }
//      else                              // Otherwise get the data packet
//      {
//         Fifo_Read(FIFO_EP2, count, (BYTE*)Out_Packet);
//      }
//      POLL_WRITE_BYTE(EOUTCSR1, 0);     // Clear Out Packet ready bit
   }
}

//----------------------------------------------------------------------------
// Out2_Get_Data
//----------------------------------------------------------------------------
//
// Enter suspend mode after suspend signalling is present on the bus
//
// Parameters   : ptr_buf - pointer to read data destination buffer
// Return Value :
//----------------------------------------------------------------------------

void Out2_Get_Data(BYTE* ptr_buf) {
	Fifo_Read(FIFO_EP2, Out_Count, ptr_buf);
}

//----------------------------------------------------------------------------
// Out2_Done
//----------------------------------------------------------------------------
//
// This routine clears out packet ready bit and out_counter to boot
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Out2_Done() {
	POLL_WRITE_BYTE(EOUTCSR1, 0);     // Clear Out Packet ready bit
	Out_Count=0;
}


//----------------------------------------------------------------------------
// USB_Suspend
//----------------------------------------------------------------------------
//
// Enter suspend mode after suspend signalling is present on the bus
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void USB_Suspend(void)
{                                         // Add power-down features here if 
                                          // you wish to
   volatile int k;                        // reduce power consumption during 
                                          // suspend mode
   k++;
}


//----------------------------------------------------------------------------
// FIFO Read
//----------------------------------------------------------------------------
//
// Read from the selected endpoint FIFO
//
// Parameters   : addr - target address
//                u_num_bytes - number of bytes to unload
//                ptr_data - read data destination
// Return Value :
//----------------------------------------------------------------------------

#if 1
void Fifo_ReadC(BYTE addr, unsigned int u_num_bytes, BYTE * ptr_data) 
{
   int i;

   if (u_num_bytes)                         // Check if >0 bytes requested,
   {      
      USB0ADR = (addr);                   // Set address
      USB0ADR |= 0xC0;                    // Set auto-read and initiate 
                                          // first read      

      // Unload <NumBytes> from the selected FIFO
      for(i=0;i<u_num_bytes;i++)
      {         
         while(USB0ADR & 0x80);           // Wait for BUSY->'0' (data ready)
         ptr_data[i] = USB0DAT;              // Copy data byte
      }

   USB0ADR = 0;                           // Clear auto-read
   }
}
#endif

//----------------------------------------------------------------------------
// FIFO Write
//----------------------------------------------------------------------------
//
// Write to the selected endpoint FIFO
//
// Parameters   : addr - target address
//                u_num_bytes - number of bytes to write
//                ptr_data - location of source data
// Return Value :
//----------------------------------------------------------------------------

#if 0
void Fifo_Write(BYTE addr, unsigned int u_num_bytes, BYTE * ptr_data) reentrant
{
   int i;
   START_SPI_TIMEOUT ;                                      
   // If >0 bytes requested,
   if (u_num_bytes) 
   {
      while(USB0ADR & 0x80);              // Wait for BUSY->'0'
                                          // (register available)
      USB0ADR = (addr);                   // Set address (mask out bits7-6)

      // Write <NumBytes> to the selected FIFO
      for(i=0;i<u_num_bytes;i++)
      {  
         USB0DAT = ptr_data[i];
         while(USB0ADR & 0x80);           // Wait for BUSY->'0' (data ready)
      }
   }
   STOP_SPI_TIME_OUT;
}
#endif


//----------------------------------------------------------------------------
// Force_Stall
//----------------------------------------------------------------------------
//
// Force a procedural stall to be sent to the host
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Force_Stall(void)
{
   POLL_WRITE_BYTE(INDEX, EP0_IDX);
   POLL_WRITE_BYTE(E0CSR, rbSDSTL);       // Set the send stall bit
   Ep_Status[0] = EP_STALL;               // Put the endpoint in stall status
}

