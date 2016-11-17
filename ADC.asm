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


dig4		EQU		48h
dig3		EQU		49h
dig2		EQU		4Ah
dig1		EQU		4Bh
dig0		EQU		4Ch
BCD2		EQU		4Dh
BCD1    EQU		4Eh	
BCD0    EQU		4Fh

resultado3		EQU		52h
resultado2		EQU		53h
resultado1		EQU		54h
resultado0		EQU		55h

contador			EQU		40h

;----------------RESET e INTERRUPCIONES---------
org	0000h
ljmp inicio


org	003h
INT0_Int:
RET

org	00Bh
Timer0_Int:
RET

org	013h
INT1_Int:
RET

org	01Bh
Timer1_Int:
RET

org	023h
UART0_Int:
RET

org	02Bh
Timer2_Int:
RET

org	033h
SPI0_Int:
RET

org	03Bh
SMB0_Int:
RET

org	043h
USB0_Int:
RET

org	04Bh
ADC0_Int:
RET

org	053h
ADC_Int:
RET

org	05Bh
PCA_Int:
RET

org	063h
Comp0_Int:
RET

org	06Bh
Comp1_Int:
RET

org	073h
Timer3_Int:
RET

org	07Bh
VBUS_Int:
RET

org	083h
UART1_Int:
RET
;----------------FIN INTERRUPCIONES---------------




;__________________________________________________
;-----------------main code------------------
;__________________________________________________

org 100h

inicio:	

;---------------SETEOS-------------------------  

PCA_Init:
    anl  PCA0MD,    #0BFh
    mov  PCA0MD,    #000h
    mov  PCA0CPM4,  #048h

Timer_Init:
;    mov  TMOD,      #001h


SPI_Init:
;    mov  SPI0CFG,   #040h
;    mov  SPI0CN,    #001h

ADC_Init:
    mov  AMX0P,     #004h
    mov  AMX0N,     #01Fh
    mov  ADC0CN,    #0C0h


Voltage_Reference_Init:
    mov  REF0CN,    #008h


Port_IO_Init:
    ; P0.0  -  SCK  (SPI0), Open-Drain, Digital
    ; P0.1  -  MISO (SPI0), Open-Drain, Digital
    ; P0.2  -  MOSI (SPI0), Open-Drain, Digital
    ; P2.5  -  Skipped,     Open-Drain, Analog

;    mov  P0MDIN,    #03Fh
;    mov  P1MDIN,    #0DFh
    mov  P2MDIN,    #0C0h
;    mov  P4MDIN,    #0FFh
;    mov  P0SKIP,    #0F8h
;    mov  P1SKIP,    #020h
;    mov  P2SKIP,    #0FFh
;    mov  P3SKIP,    #0FFh
    mov  XBR0,      #002h
    mov  XBR1,      #040h

		mov		P3MDOUT,#00Fh
		mov		P4MDOUT,#0FFh

Oscillator_Init:
    mov  OSCICN,    #083h







;-----------FIN SETEOS------------------------- 



;XXXXXXXXXXXXXXXXXXXXXXXXXXXX
;---------------SETEOS-------------------------  
    mov  P0MDOUT,   #008h
    mov  P1MDOUT,   #00Fh
    mov  P3MDOUT,   #0F0h
    mov  XBR0,      #002h
    mov  XBR1,      #040h


Oscillator_Init2:
    mov  OSCICN,    #083h

PCA_Init2:
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

;XXXXXXXXXXXXXXXXXXXXXXXXXXXX

		mov		R0,ADC0L
		mov		R1,ADC0H
		acall		init_lcd

conversion:
		setb		AD0BUSY
conv:
		JB			AD0BUSY,conv
		mov			R0,ADC0L
		mov			R1,ADC0H
		
		mov			Resultado0,R0
		mov			Resultado1,R1
		acall		binbcd
		acall		separa_nib
		
		mov			A,dig0
		acall		lcd7seg
		mov			dig0,A
		
		mov			A,dig1
		acall		lcd7seg
		mov			dig1,A
				
		mov			A,dig2
		acall		lcd7seg
		mov			dig2,A
				
		mov			A,dig3
		acall		lcd7seg
		mov			dig3,A


;		acall		clear_lcd
CLR RS
MOV DATO,#080h
SETB EN
CLR EN
LCALL WAIT_LCD
		
		mov			A,dig3
		acall		write_text
		mov			A,dig2
		acall		write_text
		mov			A,dig1
		acall		write_text
		mov			A,dig0
		acall		write_text


		ajmp		conversion

;---------------------------------------------------------
;---------PROGRAMA LCD------------------------------------


INIT_LCD:
			
			
			mov		P3MDOUT,#00Fh
			mov		P4MDOUT,#0FFh
			mov		DATO,#00h
			clr		RW
			clr		EN


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




;******** comienzo rutina BCD 16bit **********************			
;toma numero de resultado1_resultado0
;Y lo convierte a BCD en los BCD2_BCD1_BCD0 
;Rota 16 veces acarreando Resultado_0,1_BCD_0,1,2
;si algun BCD es mayor que 4, corrije

binbcd:		
				mov	contador,#16d       ;
				mov	bcd2,#00h
				mov	bcd1,#00h
				mov	bcd0,#00h

desplaz:		mov	A,resultado0
				rlc	A
				mov	resultado0,A
				mov	A,resultado1
				rlc	A
				mov	resultado1,A
				mov	A,bcd0
				rlc	A
				mov	bcd0,A
				mov	A,bcd1
				rlc	A
				mov	bcd1,A
				mov	A,bcd2
				rlc	A
				mov	bcd2,A
				djnz	contador,ajuste
				ret								;fin de conversion BCD

ajuste:		mov	r0,#bcd0
				call	ajbcd
				mov	r0,#bcd1
				call	ajbcd
				mov	r0,#bcd2
				call	ajbcd
				ajmp	desplaz

ajbcd:		mov	A,@r0
				add	A,#3h
				mov	B,A
				mov	C,B.3
				jnc	sig
				mov	@r0,B
sig:			mov	A,@r0
				add	A,#30h
				mov	B,A
				mov	C,B.7
				jnc	sig2
				mov	@r0,B
sig2:			ret

;************ fin Rutina BCD *********************

;************separa nibbles de los BCD************
;toma los BCD2_BCD1_BCD0
;los pone en dig1,2,3,4,5
separa_nib:
				mov	A,BCD0
				anl	A,#0Fh
				mov	dig0,A
				mov	A,BCD0
				anl	A,#0F0h
				swap	A
				mov	dig1,A
				mov	A,BCD1
				anl	A,#0Fh
				mov	dig2,A
				mov	A,BCD1
				anl	A,#0F0h
				swap	A
				mov	dig3,A
				mov	A,BCD2
				anl	A,#0Fh
				mov	dig4,A
				ret



;*************************************************
; toma numero del acumulador
; lo multiplico por 4 y salto a 
; acumulador mas direccion de Tabla


LCD7seg:		
				rl		A
				rl		A         
				mov	DPTR,#tabla
				jmp	@A+DPTR
ret_accion:	ret


;*************************************************


;*************************************************
;reemplaza numero por secuencia correspondiente en 7segmentos

tabla:		
				mov	A,#48d							;cero en LCD			
				ajmp	ret_accion					
				mov	A,#49d							;uno en LCD			
				ajmp	ret_accion					
				mov	A,#50d							;dos en LCD			
				ajmp	ret_accion					
				mov	A,#51d							;tres en LCD			
				ajmp	ret_accion					
				mov	A,#52d							;cuatro en LCD			
				ajmp	ret_accion					
				mov	A,#53d							;cinco en LCD			
				ajmp	ret_accion					
				mov	A,#54d							;seis en LCD			
				ajmp	ret_accion					
				mov	A,#55d							;siete en LCD			
				ajmp	ret_accion					
				mov	A,#56d							;ocho en LCD			
				ajmp	ret_accion					
				mov	A,#57d							;nueve en LCD			
				ajmp	ret_accion					
										      
;************************************************************


END
