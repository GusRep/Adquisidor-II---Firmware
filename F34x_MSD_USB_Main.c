//-----------------------------------------------------------------------------
// F34x_MSD_USB_Main.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// File contains the main loop of application. It also contains small delay 
// function and some simple initializations (CPU, Ports)
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000062
// Target:         C8051F34x
// Tool chain:     Keil
// Command Line:   See Readme.txt
// Project Name:   F34x_USB_MSD
//
// REVISIONS:  11/22/02 - DM:  Added support for switches and sample USB
// interrupt application.
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
#include "F34x_MSD_Physical_Settings.h"
#include "F34x_MSD_CF_Basic_Functions.h"
#include "c8051f340.h"
#include "F34x_MSD_USB_Register.h"
#include "F34x_MSD_USB_Main.h"
#include "F34x_MSD_USB_Descriptor.h"
#include "F34x_MSD_VBUS_Functions.h"

#include "F34x_MSD_Sect_Serv.h"
#include "F34x_MSD_Msd.h"
#include "F34x_MSD_UART.h"
#include "F34x_MSD_File_System.h"
#include <stdio.h>


#include "F34x_MSD_Cmd.h" //wozb - 27-09-2005 - it's not need it was used to communicate via uart
#include "F34x_MSD_Log.h" //wozb - 27-09-2005 - it's not need it was used to communicate via uart

#include "F34x_MSD_Temp_Sensor.h"

//-----------------------------------------------------------------------------
// 16-bit SFR Definitions for 'F34x
//-----------------------------------------------------------------------------

sfr16 TMR2RL   = 0xca;                   // Timer2 reload value
sfr16 TMR2     = 0xcc;                   // Timer2 counter     

sbit C_PWR = P1^7;
//-----------------------------------------------------------------------------
// Main Routine
//-----------------------------------------------------------------------------
volatile DWORD xdata tickcount=0;

extern void Wait_ms(unsigned int count);

//----------------------------------------------------------------------------
// main
//----------------------------------------------------------------------------
//
// Main loop
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void main(void) {
	PCA0MD &= ~0x40;                       // Disable Watchdog timer

	Sys_Clk_Init();                         // Initialize oscillator

	Port_Init();                           // Initialize crossbar and GPIO



	UART0_Init();

#ifdef __F340_VER__	
	Init_Temp_Sensor();						// initialize temerature sensor
#endif
	Sect_Init();

	Timer_Init();                          // Initialize timer2

	USB0_Init();                           // Initialize USB0

	

#ifdef __F340_VER__
	FileSys_Init();
#endif

	Cmd_Init(); 



	USB_Bulk_Init();
	
	while (1) { 
		Cmd_Step(); 
		Msd_Step();
		Log_Step(); 
#ifdef __F340_VER__
		Temp_Log_Step();
		Switch_On_Off_UART();
#endif
	}
}

//-----------------------------------------------------------------------------
// Initialization Subroutines
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Sys_Clk_Init
//----------------------------------------------------------------------------
//
// This routine initilizes the system clock and USB clock
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Sys_Clk_Init(void)
{
#ifdef _USB_LOW_SPEED_

   OSCICN |= 0x03;                       // Configure internal oscillator for
                                         // its maximum frequency and enable
                                         // missing clock detector

   CLKSEL  = SYS_INT_OSC;                // Select System clock
   CLKSEL |= USB_INT_OSC_DIV_2;          // Select USB clock
#else
   OSCICN |= 0x03;                       // Configure internal oscillator for
                                         // its maximum frequency and enable
                                         // missing clock detector

   CLKMUL  = 0x00;                       // Select internal oscillator as 
                                         // input to clock multiplier

   CLKMUL |= 0x80;                       // Enable clock multiplier
   Delay();                              // Delay for clock multiplier to begin
   CLKMUL |= 0xC0;                       // Initialize the clock multiplier
   Delay(); 

   while(!(CLKMUL & 0x20));                // Wait for multiplier to lock
#ifdef __F340_VER__
#ifdef F340_24M
  CLKSEL = SYS_4X_DIV_2;
#else
  CLKSEL = SYS_4X_MUL;
#endif
#else
#ifdef __F326_VER__
   CLKSEL  = SYS_4X_DIV_2;
#else
   CLKSEL  = SYS_INT_OSC;                // Select system clock  
#endif
#endif
   CLKSEL |= USB_4X_CLOCK;               // Select USB clock
#endif  /* _USB_LOW_SPEED_ */
}


//----------------------------------------------------------------------------
// Port_Init
//----------------------------------------------------------------------------
//
// Configure the Crossbar and GPIO ports.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Port_Init(void) {
	// Default values on reset:
	// P0MDIN=0xFF;		P1MDIN=0xFF;	P2MDIN=0xFF;	P3MDIN=0xFF; 
	// P0MDOUT=0x00;	P1MDOUT=0x00;	P2MDOUT=0x00;	P3MDOUT=0x00;
	// P0SKIP=0x00;		P1SKIP=0x00;	P2SKIP=0x00;
	// XBR0=0x00;
	data	int i;
#ifdef __F326_VER__

	GPIOCN |= 0x40;
	P0MDOUT = 0x0d;
	P0 |= 0x02;
	SCS = 1;
	SCLK = 1;
#else
	P1MDIN = 0xff;

//   P1MDIN  = 0x7F;                        // Port 1 pin 7 set as analog input
   ////p1.0 - out,p1.1-out,p1.2 - out,p1.3 -in,p1.4 - in p1.6 - out
   P1MDOUT = 0xC7;
   P1 = 0x18;
   P3MDIN = 0xff;
   P3MDOUT = 0xFF;
   P3 &= ~(0xe0) ;
   P4MDIN = 0xff;
   P4MDOUT = 0x00;
   P4  = 0xff;
   P0MDOUT = 0x1D;                    // enable TX0,SCK,MOSI as a push-pull
   P2MDOUT = 0x0C;                    // enable LEDs as a push-pull output
   XBR0    = 0x03;                     // UART0 TX and RX pins enabled, SPI enabled
   XBR1=(0x40 | 0x80); // Enable crossbar, disable weak pull-up
  // XBR1=0x40 ;
   CF_OE = 0;
   C_PWR = 1;
	for(i=0;i<5000;i++)
	   Delay();
   XBR1 &= ~ 0x80;
   CF_OE = 1;
   C_PWR = 0;
   	for(i=0;i<5000;i++)
	   Delay();	
   XBR1 |= 0x80; // week pull-up off for CF
#endif

}


//----------------------------------------------------------------------------
// USB0_Init
//----------------------------------------------------------------------------
//
// USB Initialization
// - Initialize USB0
// - Enable USB0 interrupts
// - Enable USB0 transceiver
// - Enable USB0 with suspend detection
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void USB0_Init(void)
{
   POLL_WRITE_BYTE(POWER,  0x08);          // Force Asynchronous USB Reset
   POLL_WRITE_BYTE(IN1IE,  0x07);          // Enable Endpoint 0-2 in interrupts
   POLL_WRITE_BYTE(OUT1IE, 0x07);          // Enable Endpoint 0-2 out interrupts
   POLL_WRITE_BYTE(CMIE,   0x07);          // Enable Reset, Resume, and Suspend interrupts
#ifdef _USB_LOW_SPEED_
   USB0XCN = 0xC0;                         // Enable transceiver; select low speed
   POLL_WRITE_BYTE(CLKREC, 0xA0);          // Enable clock recovery; single-step mode
                                           // disabled; low speed mode enabled
#else                                      
   USB0XCN = 0xE0;                         // Enable transceiver; select full speed
   POLL_WRITE_BYTE(CLKREC, 0x80);          // Enable clock recovery, single-step mode
                                           // disabled
#endif /* _USB_LOW_SPEED_ */

   EIE1 |= 0x02;                           // Enable USB0 Interrupts
   EA = 1;                                 // Global Interrupt enable
                                           // Enable USB0 by clearing the USB Inhibit bit
   POLL_WRITE_BYTE(POWER,  0x01);          // and enable suspend detection


}


//----------------------------------------------------------------------------
// Timer_Init
//----------------------------------------------------------------------------
//
// Timer initialization
// - Timer 2 reload, used to check if switch pressed on overflow and
// used for ADC continuous conversion
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Timer_Init(void)
{
#ifdef __F326_VER__
	TMOD = 0x01;						//Timer 0 - 16 bit timer
	CKCON = 0x00;
	TL0		= 0x2e;
	TH0		= 0xf8;
	ET0		= 1;						// enable timer 0 interrupt
    TR0		= 1;						// start timer 0
#else
	
   TMR2CN  = 0x00;                        // Stop Timer2; Clear TF2;

   CKCON  &= ~0xF0;                       // Timer2 clocked based on T2XCLK;
   TMR2RL  = 0xF000;                      // Initialize reload value
   TMR2    = 0xffff;                      // Set to reload immediately

   ET2     = 1;                           // Enable Timer2 interrupts
   TR2     = 1;                           // Start Timer2
#endif
}


//----------------------------------------------------------------------------
// Timer2_ISR
//----------------------------------------------------------------------------
//
// Timer 2 interrupt routine
// Called when timer 2 overflows
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

#ifndef __F326_VER__
void Timer2_ISR(void) interrupt 5 {
	tickcount++;
#ifdef __F340_VER__
#ifdef F340_24M
	TMR2RL = 0xF82e;
#endif 
	TMR2RL = 0xF05e;
//	TMR2RL = 0xFC16; 
#else
	TMR2RL = 0xFC16; 		// Re-initialize reload value (1kHz, 1ms)
#endif
	//TMR2RL = 0xE0B0; 		// Re-initialize reload value (125Hz, 8ms)
	TF2H=0; 				// Clear interrupt
}

#else

//----------------------------------------------------------------------------
// Timer0_ISR
//----------------------------------------------------------------------------
//
// Timer 0 interrupt routine
// Called when timer 0 overflows
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------
void Timer0_ISR(void) interrupt 1
{

	TL0		+= 0x2e;
	TH0		= 0xf8;
	tickcount++;
}
#endif


//----------------------------------------------------------------------------
// Delay
//----------------------------------------------------------------------------
//
// Used for a small pause, approximately 80 us in Full Speed,
// and 1 ms when clock is configured for Low Speed
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void Delay(void)
{
   data int x;
   for(x = 0;x < 500;x)
      x++;
}


