$NOMOD51
;------------------------------------
;-  Generated Initialization File  --
;------------------------------------

$include (C8051F340.INC)




;------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
;Author: Ashwin.V
;Country:India
;Code:LCD interface in 4bit mode
;CPU:At89c51@11.0592Mhz
;Tips:All you need to do is call the line where you want to display the message, mov the charecter to lcd_DATO and call datw.
;If you want to display a string, move the address of the hardcodded string into dptr and call datw.
;-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------		
			
;DB0		equ   P4.0
;DB1		equ   P4.1
;DB2		equ   P4.2
;DB3		equ   P4.3
;DB4    equ   P4.4
;DB5    equ   P4.5
;DB6    equ   P4.6
;DB7    equ   P4.7

RS      equ   P3.0
RW      equ   P3.1
EN			equ		P3.2
DATO    equ   P4



org 0000h
  
  ljmp inicio


;__________________________________________________
;-----------------main code------------------
;__________________________________________________

org 100h

inicio:	

;---------------SETEOS-------------------------  
    mov  P0MDOUT,   #008h
    mov  P1MDOUT,   #00Fh
    mov  P3MDOUT,   #0F0h
    mov  XBR0,      #002h
    mov  XBR1,      #040h


Oscillator_Init:
    mov  OSCICN,    #083h

PCA_Init:
    anl  PCA0MD,    #0BFh
    mov  PCA0MD,    #000h

;-----------FIN SETEOS------------------------- 


;---------------------------------------------------------


lcd:
mov		DATO,#00h
clr		RS
clr		RW
clr		EN
mov		P3MDOUT,#00Fh
mov		P4MDOUT,#0FFh


acall		init_lcd

PRUEBA:
acall		clear_lcd

MOV A,#'H'
LCALL WRITE_TEXT
MOV A,#'O'
LCALL WRITE_TEXT
MOV A,#'L'
LCALL WRITE_TEXT
MOV A,#'A'
LCALL WRITE_TEXT
MOV A,#' '
LCALL WRITE_TEXT
MOV A,#'N'
LCALL WRITE_TEXT
MOV A,#'A'
LCALL WRITE_TEXT
MOV A,#'T'
LCALL WRITE_TEXT
MOV A,#'Y'
LCALL WRITE_TEXT

CLR RS
MOV DATO,#0C0h
SETB EN
CLR EN
LCALL WAIT_LCD

MOV A,#'H'
LCALL WRITE_TEXT
MOV A,#'O'
LCALL WRITE_TEXT
MOV A,#'L'
LCALL WRITE_TEXT
MOV A,#'A'
LCALL WRITE_TEXT
MOV A,#' '
LCALL WRITE_TEXT
MOV A,#'G'
LCALL WRITE_TEXT
MOV A,#'A'
LCALL WRITE_TEXT
MOV A,#'S'
LCALL WRITE_TEXT
MOV A,#'T'
LCALL WRITE_TEXT
MOV A,#'Y'
LCALL WRITE_TEXT



ciclo:
ajmp		CICLO

INIT_LCD:

      CLR RS
      MOV DATO,#38h
      SETB EN
			acall	DiezMilis      
			CLR EN
      
			LCALL WAIT_LCD
      CLR RS
      MOV DATO,#0Eh
      SETB EN
			acall	DiezMilis 
      CLR EN
      
			LCALL WAIT_LCD
      CLR RS
      MOV DATO,#06h
      SETB EN
			acall	DiezMilis 
      CLR EN
     
      LCALL WAIT_LCD
      RET


WRITE_TEXT:

      SETB RS
      MOV DATO,A
      SETB EN
      CLR EN
      LCALL WAIT_LCD
      RET


CLEAR_LCD:

      CLR RS
      MOV DATO,#01h
      SETB EN
      CLR EN
      LCALL WAIT_LCD
      RET


WAIT_LCD:
			mov		P4MDOUT,#00h
      CLR EN ;Start LCD command
      CLR RS ;It's a command
      SETB RW ;It's a read command
      MOV DATO,#0FFh ;Set all pins to FF initially
      SETB EN ;Clock out command to LCD
      MOV A,DATO ;Read the return value
      JB ACC.7,WAIT_LCD ;If bit 7 high, LCD still busy
      CLR EN ;Finish the command
      CLR RW ;Turn off RW for future commands
      mov		P4MDOUT,#0FFh
			RET

;----------------------------------------------------------------------------;

UnSeg:
        mov     r7, #100
LazoUnSeg:
        call    DiezMilis

        djnz    r7, LazoUnSeg
        ret

;----------------------------------------------------------------------------;

DiezMilis:
        mov     r6, #100
LazoDiezMilis:
        call    CienMicros

        djnz    r6, LazoDiezMilis
        ret

;----------------------------------------------------------------------------;

CienMicros:
        mov     r5, #50
LazoCienMicros:
        djnz    r5, LazoCienMicros
        ret

;----------------------------------------------------------------------------;




END
