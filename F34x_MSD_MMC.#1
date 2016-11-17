//-----------------------------------------------------------------------------
// F34x_MSD_MMC.c
//-----------------------------------------------------------------------------
// Copyright 2006 Silicon Laboratories, Inc.
// http://www.silabs.com
//
// Program Description:
//
// MMC FLASH is used for storing the log entries.  Each entry contains
// the temperature in hundredths of a degree C, the day, hour, minute, and
// second that the reading was taken.  The LogUpdate function stores log
// entries in an external memory buffer and then writes that buffer out to the
// MMC when it is full.  Communication with the MMC is performed through the 
// MMC access functions.  These functions provide transparent MMC access to 
// the higher level functions (Logging functions).  The MMC interface is broken
// into two pieces.  The high level piece consists of the user callable MMC
// access functions (MMC_FLASH_Read, MMC_FLASH_Write, MMC_FLASH_Clear, 
// MMC_FLASH_MassErase).  These functions are called by the user to execute
// data operations on the MMC.  They break down the data operations into MMC
// commands.  The low level piece consists of a single command execution
// function (MMC_Command_Exec) which is called by the MMC data manipulation
// functions.  This function is called every time a command must be sent to the
// MMC.  It handles all of the required SPI traffic between the Silicon 
// Laboratories device and the MMC.
//
//
// How To Test:    See Readme.txt
//
//
// FID:            34X000043
// Target:         C8051F34x
// Tool chain:     Keil
// Command Line:   See Readme.txt
// Project Name:   F34x_USB_MSD
//
// Release 1.1
//    -All changes by PKC
//    -09 JUN 2006
//    -Replaced SFR definitions file "c8051f320.h" with "c8051f340.h"
//    -Corrected "SPIDAT" to "SPI0DAT" in function Write_Read_Spi_Byte
//
// Release 1.0
//    -Initial Release
//


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------

#include "F34x_MSD_Definitions.h"
#include "c8051f340.h"                 // SFR declarations
#include <stdio.h>
#include "F34x_MSD_USB_Main.h"					// Has SYSCLK #define'd
#include "F34x_MSD_Physical_Settings.h"
#include "F34x_MSD_MMC.h"
#include <intrins.h>


// Constants that define available card sizes, 8MB through 128MB                                       
#define PS_8MB       8388608L
#define PS_16MB      16777216L
#define PS_32MB      33554432L
#define PS_64MB      67108864L
#define PS_128MB     134217728L

#define ERROR_CODE	0xFFFF
#define BUFFER_SIZE 16

//#define NSSMD0 SLVSEL

// Erase group size = 16 MMC FLASH sectors
#define PHYSICAL_GROUP_SIZE     (PHYSICAL_BLOCK_SIZE * 16)

// Command table value definitions
// Used in the MMC_Command_Exec function to 
// decode and execute MMC command requests
#define     EMPTY  0
#define     YES   1
#define     NO    0
#define     CMD   0
#define     RD    1
#define     WR    2
#define     R1    0
#define     R1b   1
#define     R2    2
#define     R3    3

// Start and stop data tokens for single and multiple
// block MMC data operations
#define     START_SBR      0xFE
#define     START_MBR      0xFE
#define     START_SBW      0xFE
#define     START_MBW      0xFC
#define     STOP_MBW       0xFD

// Mask for data response Token after an MMC write
#define     DATA_RESP_MASK 0x11

// Mask for busy Token in R1b response
#define     BUSY_BIT       0x80

// Command Table Index Constants:
// Definitions for each table entry in the command table.
// These allow the MMC_Command_Exec function to be called with a
// meaningful parameter rather than a number.
#define     GO_IDLE_STATE            0
#define     SEND_OP_COND             1
#define     SEND_CSD                 2
#define     SEND_CID                 3
#define     STOP_TRANSMISSION        4
#define     SEND_STATUS              5
#define     SET_BLOCKLEN             6
#define     READ_SINGLE_BLOCK        7
#define     READ_MULTIPLE_BLOCK      8
#define     WRITE_BLOCK              9
#define     WRITE_MULTIPLE_BLOCK    10
#define     PROGRAM_CSD             11
#define     SET_WRITE_PROT          12
#define     CLR_WRITE_PROT          13
#define     SEND_WRITE_PROT         14
#define     TAG_SECTOR_START        15
#define     TAG_SECTOR_END          16
#define     UNTAG_SECTOR            17
#define     TAG_ERASE_GROUP_START   18
#define     TAG_ERASE_GROUP_END     19
#define     UNTAG_ERASE_GROUP       20
#define     ERASE                   21
#define     LOCK_UNLOCK             22
#define     READ_OCR                23
#define     CRC_ON_OFF              24

//-----------------------------------------------------------------------------
// UNIONs, STRUCTUREs, and ENUMs
//-----------------------------------------------------------------------------
typedef union LONG {                   // byte-addressable LONG
  long l;
  unsigned char b[4];
} LONG;

typedef union INT {                    // byte-addressable INT
  int i;
  unsigned char b[2];
} INT;

typedef union {                        // byte-addressable unsigned long
    unsigned long l;
    unsigned char b[4];
              } ULONG;

typedef union {                        // byte-addressable unsigned int
    unsigned int i;
    unsigned char b[2];
              } UINT;

// This structure defines entries into the command table;
typedef struct {
    unsigned char command_byte;      // OpCode;
    unsigned char arg_required;      // Indicates argument requirement;
    unsigned char CRC;               // Holds CRC for command if necessary;
    unsigned char trans_type;        // Indicates command transfer type;
    unsigned char response;          // Indicates expected response;
    unsigned char var_length;        // Indicates varialble length transfer;
               } COMMAND;

// Command table for MMC.  This table contains all commands available in SPI
// mode;  Format of command entries is described above in command structure
// definition;
COMMAND code command_list[25] = {
    { 0,NO ,0x95,CMD,R1 ,NO },    // CMD0;  GO_IDLE_STATE: reset card;
    { 1,NO ,0xFF,CMD,R1 ,NO },    // CMD1;  SEND_OP_COND: initialize card;
    { 9,NO ,0xFF,RD ,R1 ,NO },    // CMD9;  SEND_CSD: get card specific data;
    {10,NO ,0xFF,RD ,R1 ,NO },    // CMD10; SEND_CID: get card identifier;
    {12,NO ,0xFF,CMD,R1 ,NO },    // CMD12; STOP_TRANSMISSION: end read;
    {13,NO ,0xFF,CMD,R2 ,NO },    // CMD13; SEND_STATUS: read card status;
    {16,YES,0xFF,CMD,R1 ,NO },    // CMD16; SET_BLOCKLEN: set block size;
    {17,YES,0xFF,RD ,R1 ,NO },    // CMD17; READ_SINGLE_BLOCK: read 1 block;
    {18,YES,0xFF,RD ,R1 ,YES},    // CMD18; READ_MULTIPLE_BLOCK: read > 1;
    {24,YES,0xFF,WR ,R1 ,NO },    // CMD24; WRITE_BLOCK: write 1 block;
    {25,YES,0xFF,WR ,R1 ,YES},    // CMD25; WRITE_MULTIPLE_BLOCK: write > 1;
    {27,NO ,0xFF,CMD,R1 ,NO },    // CMD27; PROGRAM_CSD: program CSD;
    {28,YES,0xFF,CMD,R1b,NO },    // CMD28; SET_WRITE_PROT: set wp for group;
    {29,YES,0xFF,CMD,R1b,NO },    // CMD29; CLR_WRITE_PROT: clear group wp;
    {30,YES,0xFF,CMD,R1 ,NO },    // CMD30; SEND_WRITE_PROT: check wp status;
    {32,YES,0xFF,CMD,R1 ,NO },    // CMD32; TAG_SECTOR_START: tag 1st erase;
    {33,YES,0xFF,CMD,R1 ,NO },    // CMD33; TAG_SECTOR_END: tag end(single);
    {34,YES,0xFF,CMD,R1 ,NO },    // CMD34; UNTAG_SECTOR: deselect for erase;
    {35,YES,0xFF,CMD,R1 ,NO },    // CMD35; TAG_ERASE_GROUP_START;
    {36,YES,0xFF,CMD,R1 ,NO },    // CMD36; TAG_ERASE_GROUP_END;
    {37,YES,0xFF,CMD,R1 ,NO },    // CMD37; UNTAG_ERASE_GROUP;
    {38,YES,0xFF,CMD,R1b,NO },    // CMD38; ERASE: erase all tagged sectors;
    {42,YES,0xFF,CMD,R1b,NO },    // CMD42; LOCK_UNLOCK;
    {58,NO ,0xFF,CMD,R3 ,NO },    // CMD58; READ_OCR: read OCR register;
    {59,YES,0xFF,CMD,R1 ,NO }    // CMD59; CRC_ON_OFF: toggles CRC checking;
                              };

//-----------------------------------------------------------------------------
// Global VARIABLES
//-----------------------------------------------------------------------------

// Removed these. It doesn't work correctly on every MMC card, and we need
// all the resources we can get.
xdata unsigned long PHYSICAL_SIZE;     // MMC size variable;  Set during
                                       // initialization;

xdata unsigned long PHYSICAL_BLOCKS;   // MMC block number;  Computed during
                                       // initialization;

bdata bit Is_Initialized;
 
xdata char LOCAL_BLOCK[BUFFER_SIZE]; 
#define SEND__IN_FUNCTION

#ifdef __F326_VER__ 
static unsigned char Write_Read_Spi_Byte(unsigned char byte);
#endif

#ifdef SEND__IN_FUNCTION 
static unsigned char Write_Read_Spi_Byte(unsigned char byte);
#endif

void Wait_ms(unsigned int count);
void Wait_ns(unsigned int count);

extern BYTE xdata Scratch[PHYSICAL_BLOCK_SIZE];
extern READ_BYTES(unsigned char* pchar,unsigned int len);
extern WRITE_BYTES(unsigned char* pchar,unsigned int len);

#ifdef __F326_VER__ 
#define BACK_FROM_ERROR \
{\
  Write_Read_Spi_Byte(0xFF);\
  SCS = 1;\
  Write_Read_Spi_Byte(0xFF);\
  return ERROR_CODE; \
}
#else 
#define BACK_FROM_ERROR \
{\
  SPI0DAT = 0xFF; \
  while(!SPIF){} \
  SPIF = 0; \
  NSSMD0 = 1;\
  SPI0DAT = 0xFF;\
  while(!SPIF){}  \
  SPIF = 0;  \                      
  return ERROR_CODE; \
}
#endif 
	

//----------------------------------------------------------------------------
// SPI0_Init
//----------------------------------------------------------------------------
//
// Configure SPI0 for 8-bit, 2MHz SCK, Master mode, polled operation, data
// sampled on 1st SCK rising edge.
//
// Parameters   :
// Return Value :
//----------------------------------------------------------------------------

void SPI_Init (void)
{
#ifdef __F326_VER__
#else
  Is_Initialized = 0;
  SPI0CFG = 0x70;                     // data sampled on rising edge, clk
                                       // active low,
                                       // 8-bit data words, master mode;

  SPI0CN = 0x0F;                      // 4-wire mode; SPI enabled; flags
                                       // cleared

#ifdef __F340_VER__
  SPI0CKR = 119;
  NSSMD0 = 1;
#else
  SPI0CKR = SYSCLK/2/10000000;      //119;  // SPI clock <= 10MHz
#endif
#endif
}

sbit Led1 = P2^2;
sbit Led2 = P2^3;



#define MMC_Show_Activity() { if(Led1) { Led1=0;Led2=1;} else { Led1=1;Led2=0; } }

//-----------------------------------------------------------------------------
// MMC_Command_Exec
//-----------------------------------------------------------------------------
//
// This function generates the necessary SPI traffic for all MMC SPI commands.
// The three parameters are described below:
// 
// Cmd:      This parameter is used to index into the command table and read
//           the desired command.  The Command Table Index Constants allow the
//           caller to use a meaningful constant name in the Cmd parameter 
//           instead of a simple index number.  For example, instead of calling 
//           MMC_Command_Exec (0, argument, pchar) to send the MMC into idle 
//           state, the user can call 
//           MMC_Command_Exec (GO_IDLE_STATE, argument, pchar);
//
// argument: This parameter is used for MMC commands that require an argument.
//           MMC arguments are 32-bits long and can be values such as an
//           an address, a block length setting, or register settings for the
//           MMC.
//
// pchar:    This parameter is a pointer to the local data location for MMC 
//           data operations.  When a read or write occurs, data will be stored
//           or retrieved from the location pointed to by pchar.
//
// The MMC_Command_Exec function indexes the command table using the Cmd 
// parameter. It reads the command table entry into memory and uses information
// from that entry to determine how to proceed.  Returns the 16-bit card 
// response value;
//

unsigned int MMC_Command_Exec (unsigned char cmd_loc, unsigned long argument,
                           unsigned char *pchar)
{
  xdata unsigned char loopguard;
  idata COMMAND current_command;      // Local space for the command table 
                                       // entry;
  idata ULONG long_arg;               // Union variable for easy byte 
                                       // transfers of the argument;
                                       // Static variable that holds the 
                                       // current data block length;
  static xdata unsigned int current_blklen = 512;
  unsigned long xdata old_blklen = 512;     // Temp variable to preserve data block
                                       // length during temporary changes;
  idata unsigned int counter = 0;     // Byte counter for multi-byte fields;
  idata UINT card_response;           // Variable for storing card response;
  idata unsigned char data_resp;      // Variable for storing data response;
  idata unsigned char dummy_CRC;      // Dummy variable for storing CRC field;
  unsigned char *plast; 
   
//   xdata unsigned char c;
                                     
  current_command = command_list[cmd_loc]; // Retrieve desired command table entry
										// from code space;
  card_response.i = 0;
#ifdef __F326_VER__
  Write_Read_Spi_Byte(0xFF);
  SCS = 0;
  Write_Read_Spi_Byte(0xFF);
  Write_Read_Spi_Byte(current_command.command_byte | 0x40);
#else
#ifdef SEND__IN_FUNCTION
  Write_Read_Spi_Byte(0xFF);
  NSSMD0 = 0;
  Write_Read_Spi_Byte(0xFF);
  Write_Read_Spi_Byte(current_command.command_byte | 0x40);
#else

  SPI0DAT = 0xFF;                     // Send buffer SPI clocks to ensure no
  while(!SPIF){}                      // MMC operations are pending;
  SPIF = 0;
  NSSMD0 = 0;                         // Select MMC by pulling CS low;
  SPI0DAT = 0xFF;                     // Send another byte of SPI clocks;
  while(!SPIF){}
  SPIF = 0;
                                       // Issue command opcode;
  SPI0DAT = (current_command.command_byte | 0x40);
#endif
#endif
  long_arg.l = argument;              // Make argument byte addressable;
                                      // If current command changes block
                                      // length, update block length variable
                                      // to keep track;
                                      // Command byte = 16 means that a set
                                      // block length command is taking place
                                      // and block length variable must be
                                      // set;
  if(current_command.command_byte == 16) {
    current_blklen = argument;       
  }                                
                                       // Command byte = 9 or 10 means that a
                                       // 16-byte register value is being read
                                       // from the card, block length must be
                                       // set to 16 bytes, and restored at the
                                       // end of the transfer;
  if((current_command.command_byte == 9)||
     (current_command.command_byte == 10)) {
    old_blklen = current_blklen;     // Command is a GET_CSD or GET_CID,
    current_blklen = 16;             // set block length to 16-bytes;
  }

#ifndef __F326_VER__
#ifdef SEND__IN_FUNCTION
#else
  while(!SPIF){}                      // Wait for initial SPI transfer to end;
  SPIF = 0;                           // Clear SPI Interrupt flag;
#endif
#endif

                                       // If an argument is required, transmit
                                       // one, otherwise transmit 4 bytes of
                                       // 0x00;
  plast = &pchar[current_blklen];
  if(current_command.arg_required == YES) {
    counter = 0;
    while(counter <= 3) {
#ifdef __F326_VER__	
		  Write_Read_Spi_Byte(long_arg.b[counter]);
		  counter++;
#else
#ifdef SEND__IN_FUNCTION
	    Write_Read_Spi_Byte(long_arg.b[counter]);
	    counter++;
#else
      SPI0DAT = long_arg.b[counter];
      counter++;
      while(!SPIF){}
      SPIF = 0;
#endif
#endif
    }
  } else {
    counter = 0;
    while(counter <= 3) {
#ifdef __F326_VER__	
      Write_Read_Spi_Byte(0x00);
		  counter++;
#else
#ifdef SEND__IN_FUNCTION
      Write_Read_Spi_Byte(0x00);
      counter++;
#else
      SPI0DAT = 0x00;
      counter++;
      while(!SPIF){}
      SPIF = 0;
#endif
#endif
    }
  }
#ifdef __F326_VER__	
  Write_Read_Spi_Byte(current_command.CRC);
#else
#ifdef SEND__IN_FUNCTION
  Write_Read_Spi_Byte(current_command.CRC);
#else
  SPI0DAT = current_command.CRC;      // Transmit CRC byte;  In all cases
  while(!SPIF){}                      // except CMD0, this will be a dummy
  SPIF = 0;                           // character;
#endif
#endif
                                       // The command table entry will indicate
                                       // what type of response to expect for
                                       // a given command;  The following 
                                       // conditional handles the MMC response;
  if(current_command.response == R1) { // Read the R1 response from the card;
    loopguard=0;
    do {
#ifdef __F326_VER__
      card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else

#ifdef SEND__IN_FUNCTION
		  card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else
      SPI0DAT = 0xFF;               // Write dummy value to SPI so that 
      while(!SPIF){}                // the response byte will be shifted in;
      SPIF = 0;
      card_response.b[0] = SPI0DAT; // Save the response;
#endif
#endif

      if(!++loopguard) break;
      if(card_response.b[0] & BUSY_BIT) { 
		 //	printf("R1 response 0x%02bX\r\n",card_response.b[0]);
        Wait_ns(700);
		  }
    } while((card_response.b[0] & BUSY_BIT));
	  if(!loopguard) { 
    /*printf("R1 response 0x%02bX\r\n",card_response.b[0]);*/
      return card_response.b[0];}//{ return 0; }
    }                                     // Read the R1b response;
    else if(current_command.response == R1b) {
      loopguard = 0;	
      do {
#ifdef __F326_VER__
        card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
        card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else
        SPI0DAT = 0xFF;               // Start SPI transfer;
        while(!SPIF){}
        SPIF = 0;
        card_response.b[0] = SPI0DAT; // Save card response
#endif
#endif
        if(!++loopguard) break;
      }
      while((card_response.b[0] & BUSY_BIT));
#ifdef __F326_VER__
      while(Write_Read_Spi_Byte(0xff) == 0x00);
#else
#ifdef SEND__IN_FUNCTION
	    while(Write_Read_Spi_Byte(0xff) == 0x00);
#else
      loopguard = 0;
      do {                              // Wait for busy signal to end;
        SPI0DAT = 0xFF;               
        while(!SPIF){}
        SPIF = 0;
  		  if(!++loopguard) break;
      } while(SPI0DAT == 0x00);          // When byte from card is non-zero,
	    if(!loopguard) {BACK_FROM_ERROR;}
#endif
#endif
    }                                   // card is no longer busy;
                                       // Read R2 response
    else if(current_command.response == R2) {
      loopguard=0;
      do {
#ifdef __F326_VER__
		    card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
        card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else
        SPI0DAT = 0xFF;               // Start SPI transfer;
        while(!SPIF){}
        SPIF = 0;
        card_response.b[0] = SPI0DAT; // Read first byte of response;
#endif
#endif
        if(!++loopguard) break;
      } while((card_response.b[0] & BUSY_BIT));

	    if(!loopguard) { BACK_FROM_ERROR; }
#ifdef __F326_VER__
	    card_response.b[1] = Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
      card_response.b[1] = Write_Read_Spi_Byte(0xFF);
#else
      SPI0DAT = 0xFF;
      while(!SPIF){}
      SPIF = 0;
      card_response.b[1] = SPI0DAT;    // Read second byte of response;
#endif
#endif
    } else {                               // Read R3 response;
      loopguard=0;
      do {
#ifdef __F326_VER__
        card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
        card_response.b[0] = Write_Read_Spi_Byte(0xFF);
#else
        SPI0DAT = 0xFF;               // Start SPI transfer;
        while(!SPIF){}
        SPIF = 0;
        card_response.b[0] = SPI0DAT; // Read first byte of response;
#endif
#endif
		    if(!++loopguard) break;
      } while((card_response.b[0] & BUSY_BIT));
  
  	  if(!loopguard) { BACK_FROM_ERROR; }
      counter = 0;
      while(counter <= 3)              // Read next three bytes and store them
      {                                // in local memory;  These bytes make up
        counter++;                    // the Operating Conditions Register
#ifdef __F326_VER__
	      *pchar++ = Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
        *pchar++ = Write_Read_Spi_Byte(0xFF);
#else
        SPI0DAT = 0xFF;               // (OCR);
        while(!SPIF){}
        SPIF = 0;
        *pchar++ = SPI0DAT;
#endif
#endif
      }
    }
    switch(current_command.trans_type)  // This conditional handles all data 
    {                                   // operations;  The command entry
                                       // determines what type, if any, data
                                       // operations need to occur;
      case RD:                         // Read data from the MMC;
  		  loopguard = 0;
#ifdef __F326_VER__
	      while(Write_Read_Spi_Byte(0xFF)!=START_SBR) {
    	  	if(!++loopguard) {BACK_FROM_ERROR;}
	      }	
#else
#ifdef SEND__IN_FUNCTION
	      while((Write_Read_Spi_Byte(0xFF))!=START_SBR) {
          Wait_ns(700);
	  	    if(!++loopguard) { 
          /*printf("RD Start response not set ");*/ 
            BACK_FROM_ERROR;}
	        }	
#else
          do                            // Wait for a start read Token from
          {                             // the MMC;
									   // Start a SPI transfer;
			      SPI0DAT = 0xFF;            
            while(!SPIF){}
            SPIF = 0;
			      if(!++loopguard) break;
          } while(SPI0DAT != START_SBR);  // Check for a start read Token;
		      if(!loopguard) { BACK_FROM_ERROR; }
#endif
#endif
		
          counter = 0;                  // Reset byte counter;
                                       // Read <current_blklen> bytes;
	//	 START_SPI_TIMEOUT;
		      READ_BYTES(pchar,current_blklen);
	//	 STOP_SPI_TIME_OUT;
		
#ifdef __F326_VER__
		      dummy_CRC = Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
		      dummy_CRC = Write_Read_Spi_Byte(0xFF);
#else
          SPI0DAT = 0xFF;               // After all data is read, read the two
          while(!SPIF){}                // CRC bytes;  These bytes are not used
          SPIF = 0;                     // in this mode, but the placeholders 
          dummy_CRC = SPI0DAT;          // must be read anyway;
          SPI0DAT = 0xFF;
          while(!SPIF){}
          SPIF = 0;
          dummy_CRC = SPI0DAT;
#endif
#endif
          break;
        case WR: 
	  
#ifdef __F326_VER__
          Write_Read_Spi_Byte(0xFF);
          Write_Read_Spi_Byte(START_SBW);
#else

#ifdef SEND__IN_FUNCTION
          Write_Read_Spi_Byte(0xFF);
          Write_Read_Spi_Byte(START_SBW);
#else										// Write data to the MMC;
          SPI0DAT = 0xFF;               // Start by sending 8 SPI clocks so
          while(!SPIF){}                // the MMC can prepare for the write;
          SPIF = 0;
          SPI0DAT = START_SBW;          // Send the start write block Token;
          while(!SPIF){}
          SPIF = 0;
         					            // Reset byte counter;
                                       // Write <current_blklen> bytes to MMC;
#endif
          Wait_ns(700);
#endif

		 //START_SPI_TIMEOUT;
        WRITE_BYTES(pchar,current_blklen);
 	    // STOP_SPI_TIME_OUT;
	
#ifdef __F326_VER__
        Write_Read_Spi_Byte(0xFF);
        Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
        Write_Read_Spi_Byte(0xFF);
        Write_Read_Spi_Byte(0xFF);
#else
        SPI0DAT = 0xFF;               // Write CRC bytes (don't cares);
        while(!SPIF){}
        SPIF = 0;
        SPI0DAT = 0xFF;
        while(!SPIF){}
        SPIF = 0;
#endif
#endif
        loopguard = 0;
        do                            // Read Data Response from card;
        {  
#ifdef __F326_VER__
          data_resp = Write_Read_Spi_Byte(0xFF);
          if(!++loopguard) break;
#else
#ifdef SEND__IN_FUNCTION
          data_resp = Write_Read_Spi_Byte(0xFF);
          if(!++loopguard) break;
#else
          SPI0DAT = 0xFF;
          while(!SPIF){}
          SPIF = 0;
          data_resp = SPI0DAT;
          if(!++loopguard) break;
#endif
#endif
        }                             // When bit 0 of the MMC response
                                       // is clear, a valid data response
                                       // has been received;
        while((data_resp & DATA_RESP_MASK) != 0x01);
        if(!loopguard) { BACK_FROM_ERROR; }

#ifdef __F326_VER__
        while(Write_Read_Spi_Byte(0xFF)==0x00);
        Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
        while(Write_Read_Spi_Byte(0xFF)==0x00);
        Write_Read_Spi_Byte(0xFF);
#else
        do                            // Wait for end of busy signal;
        {
          SPI0DAT = 0xFF;            // Start SPI transfer to receive
          while(!SPIF){}             // busy tokens;
          SPIF = 0;
        } while(SPI0DAT == 0x00);       // When a non-zero Token is returned,
                                       // card is no longer busy;

        SPI0DAT = 0xFF;               // Issue 8 SPI clocks so that all card
        while(!SPIF){}                // operations can complete;
        SPIF = 0;
#endif
#endif

        break;
      default: break;
    }
#ifdef __F326_VER__
    Write_Read_Spi_Byte(0xFF);
    SCS = 1;
    Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
    Write_Read_Spi_Byte(0xFF);
    NSSMD0 = 1;
    Write_Read_Spi_Byte(0xFF);
#else
    SPI0DAT = 0xFF;
    while(!SPIF){}
    SPIF = 0;

    NSSMD0 = 1;                         // Deselect memory card;
    SPI0DAT = 0xFF;                     // Send 8 more SPI clocks to ensure
    while(!SPIF){}                      // the card has finished all necessary
    SPIF = 0;                           // operations;
                                       // Restore old block length if needed;
#endif
#endif
    if((current_command.command_byte == 9)||
      (current_command.command_byte == 10)) {
      current_blklen = old_blklen;
    }
    return card_response.i;
}


//-----------------------------------------------------------------------------
// MMC_FLASH_Init
//-----------------------------------------------------------------------------
//
// This function initializes the flash card, configures it to operate in SPI
// mode, and reads the operating conditions register to ensure that the device
// has initialized correctly.  It also determines the size of the card by 
// reading the Card Specific Data Register (CSD).

void MMC_FLASH_Init(void)
{
  xdata unsigned loopguard;
  xdata int i;
  xdata UINT card_status;             // Stores card status returned from 
                                       // MMC function calls(MMC_Command_Exec);
  xdata unsigned char counter = 0;    // SPI byte counter;
  unsigned char xdata *pchar;         // Xdata pointer for storing MMC 
                                       // register values;
                                       // Transmit at least 64 SPI clocks
                                       // before any bus comm occurs.

  unsigned int c_size,bl_len;
  unsigned char c_mult;
//	PHYSICAL_SIZE=0;
//	PHYSICAL_BLOCKS=0;

  SPI_Init();
  Wait_ms(100);
  pchar = (unsigned char xdata*)LOCAL_BLOCK;
  for(counter = 0; counter < 8; counter++) {
#ifdef __F326_VER__
    Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
    Write_Read_Spi_Byte(0xFF);
#else
    SPI0DAT = 0xFF;
    while(!SPIF){}
    SPIF = 0;
#endif
#endif
  }
  for(counter = 0; counter < 2; counter++) {
#ifdef __F326_VER__
    Write_Read_Spi_Byte(0xFF);
#else
#ifdef SEND__IN_FUNCTION
    Write_Read_Spi_Byte(0xFF);
#else
    SPI0DAT = 0xFF;
    while(!SPIF){}
    SPIF = 0;
#endif
#endif
  }
  
#ifdef __F326_VER__
  SCS = 0;
#else
  NSSMD0 = 0;                         // Select the MMC with the CS pin;
                                       // Send 16 more SPI clocks to 
                                       // ensure proper startup;
#endif
                                       // Send the GO_IDLE_STATE command with
                                       // CS driven low;  This causes the MMC
                                       // to enter SPI mode; 	
  Wait_ms(1);
  card_status.i = MMC_Command_Exec(GO_IDLE_STATE,EMPTY,EMPTY);

  loopguard=0;

                                       // Send the SEND_OP_COND command
  do                                  // until the MMC indicates that it is
  {         

    Wait_ms(1);
    card_status.i = MMC_Command_Exec(SEND_OP_COND,EMPTY,EMPTY);
    if(!++loopguard) break;
  } while ((card_status.b[0] & 0x01));
  printf("count SEND_OP_COND: %d\n",loopguard);

  if(!loopguard) return;

#ifdef __F326_VER__
  Write_Read_Spi_Byte(0xFF);
#else

#ifdef SEND__IN_FUNCTION
  Write_Read_Spi_Byte(0xFF);
#else
  SPI0DAT = 0xFF;                     // Send 8 more SPI clocks to complete
  while(!SPIF){}                      // the initialization sequence;
  SPIF = 0;
#endif
#endif
  loopguard=0;

  do                                  // Read the Operating Conditions 
  {                                   // Register (OCR);
    card_status.i = MMC_Command_Exec(READ_OCR,EMPTY,pchar);
    if(!++loopguard) break;
  } while(!(*pchar&0x80));              // Check the card busy bit of the OCR;
  if(!loopguard) return;

  card_status.i = MMC_Command_Exec(SEND_STATUS,EMPTY,EMPTY);
                                       // Get the Card Specific Data (CSD)
                                       // register to determine the size of the
                                       // MMC;
  for(i=0;i<4;i++) {
    printf("0x%02bX ",pchar[i]);
  }

  card_status.i = MMC_Command_Exec(SEND_CSD,EMPTY,pchar);
	
  if(card_status.i==0) {
    printf("Change speed");
    for(i=0;i<16;i++) {
      printf("0x%02bX ",pchar[i]);
    }
	#ifdef F340_M24
    SPI0CKR = 0;
	#else
    SPI0CKR = 1;
	#endif
    Wait_ms(1);
  } else {
    printf("CARD STATUS 0x%02X:\n",card_status.i);
  	for(i=0;i<16;i++) {
      printf("0x%02bX ",pchar[i]);
    }
    PHYSICAL_BLOCKS = 0;
    PHYSICAL_SIZE = PHYSICAL_BLOCKS * bl_len;
    return;
  }

  card_status.i = MMC_Command_Exec(SET_BLOCKLEN,(unsigned long)PHYSICAL_BLOCK_SIZE,EMPTY);

  bl_len = 1 << (pchar[5] & 0x0f) ;
  c_size = ((pchar[6] & 0x03) << 10) | 
   			(pchar[7] << 2) | ((pchar[8] &0xc0) >> 6);
  c_mult = (((pchar[9] & 0x03) << 1) | ((pchar[10] & 0x80) >> 7));
  
                                       // Determine the number of MMC sectors;
  PHYSICAL_BLOCKS = (unsigned long)(c_size+1)*(1 << (c_mult+2));
  PHYSICAL_SIZE = PHYSICAL_BLOCKS * bl_len;

  loopguard = 0;

  while((MMC_FLASH_Block_Read(0,Scratch)!=0)) {
    if(!++loopguard) break;
  } 
  //printf("Wrong reads %d\n",loopguard);

  Is_Initialized = 1;
	
  Led1=0;Led2=0;
}

//-----------------------------------------------------------------------------
// MMC_FLASH_Block_Read
//-----------------------------------------------------------------------------
//
// If you know beforehand that you'll read an entire 512-byte block, then
// this function has a smaller ROM footprint than MMC_FLASH_Read.
//
// Parameters   : address - address of block
//				  pchar - pointer to byte 
// Return Value : card status
//----------------------------------------------------------------------------

unsigned int MMC_FLASH_Block_Read(unsigned long address, unsigned char *pchar) 
{
  xdata unsigned int card_status;     // Stores MMC status after each MMC command;
  address *= PHYSICAL_BLOCK_SIZE;
  Led1=1;
/*	card_status = MMC_Command_Exec(SET_BLOCKLEN,(unsigned long)PHYSICAL_BLOCK_SIZE,EMPTY);
  if(card_status!=0) {
    printf("Read Address 0x%02LX\r\n",address);
    printf("Reading ERROR during set blocklen 0x%02BX\n",card_status);
    return card_status;
  }*/
  card_status = MMC_Command_Exec(READ_SINGLE_BLOCK,address,pchar);
	/*if(card_status!=0) {
    printf("Read Address 0x%02LX\r\n",address);
    printf("Reading ERROR during reading data 0x%02BX\n",card_status);
	}*/
	Led1=0;
	return card_status;
}

//-----------------------------------------------------------------------------
// MMC_FLASH_Block_Write
//-----------------------------------------------------------------------------
//
// If you know beforehand that you'll write an entire 512-byte block, then
// this function is more RAM-efficient than MMC_FLASH_Write because it 
// doesn't require a 512-byte Scratch buffer (and it's faster too, it doesn't
// require a read operation first). And it has a smaller ROM footprint too.
//
// Parameters   : address - address of block
//				  wdata - pointer to data 
// Return Value : card status
//----------------------------------------------------------------------------

unsigned char MMC_FLASH_Block_Write(unsigned long address,unsigned char *wdata) 
{
  xdata unsigned int card_status;     // Stores status returned from MMC;

  address *= PHYSICAL_BLOCK_SIZE;
  Led2=1;
  card_status = MMC_Command_Exec(WRITE_BLOCK,address ,wdata);
  Led2=0;
  return card_status;
}


#ifdef __F326_VER__

//----------------------------------------------------------------------------
// Write_Read_Spi_Byte
//----------------------------------------------------------------------------
//
// Function sends one byte to spi and reads ony byte from spi
// it will be written with SYSCLK as clk
//
// Parameters   : byte - value to write
// Return Value : SPI byte
//----------------------------------------------------------------------------


unsigned char Write_Read_Spi_Byte(unsigned char byte)
{
  unsigned char i,ret = 0;
  for(i=0;i<8;i++) {
    MOSI = (byte & 0x80) ? 1 : 0;
    SCLK = 0;	
    byte <<= 1;
    ret <<= 1;
    SCLK = 1;
    ret |= MISO;
  }
  return ret;
}

#endif


#ifdef SEND__IN_FUNCTION

//----------------------------------------------------------------------------
// Write_Read_Spi_Byte
//----------------------------------------------------------------------------
//
// Function sends one byte to spi and reads ony byte from spi
// it will be written with SYSCLK as clk
//
// Parameters   : byte - value to write
// Return Value : SPI byte
//----------------------------------------------------------------------------

unsigned char Write_Read_Spi_Byte(unsigned char byte)
{
  unsigned char xdata ret;
  SPI0DAT = byte;
  while(!SPIF);                      
  SPIF = 0;
  ret = SPI0DAT;
  return ret;
}

#endif

//----------------------------------------------------------------------------
// Wait_ms
//----------------------------------------------------------------------------
//
// Delay function with declared wait time in milliseconds
//
// Parameters   : count - time in ms
// Return Value :
//----------------------------------------------------------------------------

void Wait_ms(unsigned int count)
{
  int xdata i,j;
  for(i=0;i<count;i++) {
    for(j=0;j<1000;j++) {
      Wait_ns(1000);
    }
  }
}

//----------------------------------------------------------------------------
// Wait_ns
//----------------------------------------------------------------------------
//
// Delay function with declared wait time in nanoseconds
//
// Parameters   : count - time in ns
// Return Value :
//----------------------------------------------------------------------------

void Wait_ns(unsigned int count)
{

#ifdef F340_M24
  count/=40;
#else
  count/=20;
#endif

  while(count--) ;
}

//----------------------------------------------------------------------------
// Get_Status_MMC
//----------------------------------------------------------------------------
//
// Function returns the status of MMC card
//
// Parameters   : 
// Return Value :
//----------------------------------------------------------------------------

#ifdef __F340_VER__
void Get_Status_MMC()
{
  int xdata status= MMC_Command_Exec(SEND_STATUS,EMPTY,EMPTY);	
  printf("MMC Card Status 0x%02X",status);
}

#endif