;------------------------------------
;-  Generated Initialization File  --
;------------------------------------

$include (C8051F340.inc)


;-----------------------LCD 4-bit--------------------------
;LCD Connections to microcontroller
E		EQU	P3.5		; PIN 6 enable
RS 		EQU	P3.7	; /register select

DATOS		EQU	P1		; DATOS lines P0-P3
;----------------------------------------------------------
org	000h

	ajmp on_reset
;----------------------------------------------------------
org 	100h
on_reset:	

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




		MOV DPTR,#init_DATOS
		acall disp_init
		
		MOV DPTR,#message1
		acall disp_string
		
		acall disp_line2
		
		MOV DPTR,#message2
		acall disp_string

ENDLESS:
		ajmp ENDLESS	
;----------------------------------------------------------
; display char in A
; used reg R2
disp_char:
		SETB RS			; desable register select
		
		MOV R2,A
		ANL A,#0F0H			; send high nybble first
		RR A
		RR A
		RR A
		RR A
		ANL DATOS,#0F0H
		ORL DATOS,A
		
		SETB E			; give E pulse
		acall delay_DATOS
		CLR E
		acall delay_DATOS
		
		MOV A,R2
		ANL A,#0FH			; send lower nybble
		ANL DATOS,#0F0H
		ORL DATOS,A
		
		SETB E			; give E pulse
		acall delay_DATOS
		CLR E
		acall delay_DATOS
		
		
RET
;----------------------------------------------------------
; used registers A,R1,R7
disp_string:
		MOV A,#00H
		MOV R1,#00h
next_char:
		INC R1			
		MOVC A,@A+DPTR
			MOV DATOS,A
			acall disp_char
			MOV A,R0
		CJNE R1,#0Fh,next_char			
RET
;----------------------------------------------------------
; used registers A,R1,R7,R2
disp_init:
		CLR RS			; enable /register select
		MOV A,#00H
		MOV R1,#00h
next_chari:
		INC R1			
		MOVC A,@A+DPTR
		MOV R2,A			; take backup
		
		ANL A,#0F0H			; send high nybble first
		RR A
		RR A
		RR A
		RR A
		ANL DATOS,#0F0H
		ORL DATOS,A
		
		SETB E			; give E pulse
		acall delay_init
		CLR E
		acall delay_init
		
		MOV A,R2
		ANL A,#0FH			; send lower nybble
		ANL DATOS,#0F0H
		ORL DATOS,A
		
		SETB E			; give E pulse
		acall delay_init
		CLR E
		acall delay_init

		MOV A,R0
		CJNE R1,#02h,next_chari	
		

RET
;----------------------------------------------------------
disp_line1:
		CLR RS			; register select
		ANL DATOS,#0F0H
		ORL DATOS,#08H		; first line address
		SETB E			; give E pulse
		acall delay_init
		CLR E
		acall delay_init
		
		ANL DATOS,#0F0H		; second nybble
		SETB E			; give E pulse
		acall delay_init
		CLR E
		acall delay_init

RET
;----------------------------------------------------------
disp_line2:
		CLR RS			; register select	
		ANL DATOS,#0F0H		; next line address
		ORL DATOS,#0AH
		SETB E			; give E pulse
		acall delay_init
		CLR E
		acall delay_init
		
		ANL DATOS,#0F0H		; next line address
		ORL DATOS,#08H
		SETB E			; give E pulse
		acall delay_init
		CLR E
		acall delay_init

RET
;----------------------------------------------------------
;2 msec delay
delay_DATOS:
		MOV R7,#0FFH
del:		NOP
		NOP
		DJNZ R7, del	
RET
;----------------------------------------------------------
;20 msec delay
delay_init:
		MOV R7,#0FFH
del1:		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		NOP
		DJNZ R7, del1	

RET
;----------------------------------------------------------
message1:
DB "Testing 123.... "
message2:
DB "It's working    "
init_DATOS:
DB 0FH ; Display On, Cursor On,Cursor Blink On(1)/Off(0)
DB 01H ; Clear Display
DB 38H ; Set Interface Length


end

