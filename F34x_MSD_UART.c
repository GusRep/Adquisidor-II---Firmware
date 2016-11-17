//-----------------------------------------------------------------------------
// F34x_MSD_UART.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// File contains initialization functions for UART interface.
//
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000057
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
#include "F34x_MSD_UART.h"
#include "c8051f340.h"
#include "F34x_MSD_Put_Char.h"
#include <stdio.h>

#ifdef __F340_VER__
#ifdef F340_24M
#define START_SYSCLK 24000000
#else
#define START_SYSCLK 48000000
#endif
//#define START_SYSCLK 12000000
#else
#ifdef __F326_VER__
#define START_SYSCLK 24000000
#else
#define START_SYSCLK 12000000
#endif
#endif
#define SYSCLK       START_SYSCLK /** 2*/  // SYSCLK frequency in Hz

#define BAUDRATE     115200            // Baud rate of UART in bps

#define USBCLK		 48000000


//----------------------------------------------------------------------------
// UART0_Init
//----------------------------------------------------------------------------
//
// This function initializes UART 0 interface.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void UART0_Init(void) {
   int xdata sbrl ;
   sbrl =  (0xFFFF - (SYSCLK/BAUDRATE/2)) + 1;
   SCON0 = 0x10;                       // SCON0: 8-bit variable bit rate
                                       //        level of STOP bit is ignored
                                       //        RX enabled
                                       //        ninth bits are zeros
                                       //        clear RI0 and TI0 bits

#ifdef __F326_VER__
   SMOD0 = 0x0c;
   
   SBRLH0 = (sbrl & 0xFF00) >> 8;
   SBRLL0 = sbrl & 0x00ff; 

   SBCON0 = 0x43; 

  // ES0 = 1;
  // PS0 = 1;
   TI0 = 1;
#else
   if (SYSCLK/BAUDRATE/2/256 < 1) 
   {
      TH1 = -(SYSCLK/BAUDRATE/2);
      CKCON |=  0x08;                  // T1M = 1; SCA1:0 = xx
   } 
   else if (SYSCLK/BAUDRATE/2/256 < 4) 
   {
      TH1 = -(SYSCLK/BAUDRATE/2/4);
      CKCON &= ~0x0B;                  
      CKCON |=  0x01;                  // T1M = 0; SCA1:0 = 01
   } 
   else if (SYSCLK/BAUDRATE/2/256 < 12) 
   {
      TH1 = -(SYSCLK/BAUDRATE/2/12);
      CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 00
   } 
   else 
   {
      TH1 = -(SYSCLK/BAUDRATE/2/48);
      CKCON &= ~0x0B;                  // T1M = 0; SCA1:0 = 10
      CKCON |=  0x02;
   }

   TL1 = TH1;                          // init Timer1
   TMOD &= ~0xf0;                      // TMOD: timer 1 in 8-bit autoreload
   TMOD |=  0x20;
   TR1 = 1;                            // START Timer1
   TI0 = 1;                            // Indicate TX0 ready
#endif
	printf("UART INIT"ENDLINE);
}

//----------------------------------------------------------------------------
// key_available
//----------------------------------------------------------------------------
//
// Function returns state of UART receive interrupt flag "RI0"
//
// Parameters   :
// Return Value : TRUE if RI0 flag is set and FALSE if not.
//----------------------------------------------------------------------------

unsigned char key_available() {
	return RI0?1:0;
}
