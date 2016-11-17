;------------------------------------
;-  Generated Initialization File  --
;------------------------------------

$include (C8051F340.inc)

Org		000h
ajmp	inicio


; Peripheral specific initialization functions,
; Called from the Init_Device label

org 100h

inicio:
SPI_Init:
    mov  SPI0CFG,   #040h
    mov  SPI0CN,    #001h
    mov  SPI0CKR,   #010h



    ; P0.0  -  SCK  (SPI0), Push-Pull,  Digital
    ; P0.1  -  MISO (SPI0), Open-Drain, Digital
    ; P0.2  -  MOSI (SPI0), Push-Pull,  Digital
    ; P0.3  -  Unassigned,  Push-Pull,  Digital
    ; P0.4  -  Unassigned,  Open-Drain, Digital
    ; P0.5  -  Unassigned,  Open-Drain, Digital
    ; P0.6  -  Unassigned,  Open-Drain, Digital
    ; P0.7  -  Unassigned,  Open-Drain, Digital

    ; P1.0  -  Unassigned,  Open-Drain, Digital
    ; P1.1  -  Unassigned,  Open-Drain, Digital
    ; P1.2  -  Unassigned,  Open-Drain, Digital
    ; P1.3  -  Unassigned,  Open-Drain, Digital
    ; P0.3  -  Unassigned,  Open-Drain, Digital
    ; P1.5  -  Unassigned,  Open-Drain, Digital
    ; P1.6  -  Unassigned,  Open-Drain, Digital
    ; P1.7  -  Unassigned,  Open-Drain, Digital

    ; P2.0  -  Unassigned,  Open-Drain, Digital
    ; P2.1  -  Unassigned,  Open-Drain, Digital
    ; P2.2  -  Unassigned,  Open-Drain, Digital
    ; P2.3  -  Unassigned,  Open-Drain, Digital
    ; P2.4  -  Unassigned,  Open-Drain, Digital
    ; P2.5  -  Unassigned,  Open-Drain, Digital
    ; P2.6  -  Unassigned,  Open-Drain, Digital
    ; P2.7  -  Unassigned,  Open-Drain, Digital

    ; P3.0  -  Unassigned,  Open-Drain, Digital
    ; P3.1  -  Unassigned,  Open-Drain, Digital
    ; P3.2  -  Unassigned,  Open-Drain, Digital
    ; P3.3  -  Unassigned,  Open-Drain, Digital
    ; P3.4  -  Unassigned,  Open-Drain, Digital
    ; P3.5  -  Unassigned,  Open-Drain, Digital
    ; P3.6  -  Unassigned,  Open-Drain, Digital
    ; P3.7  -  Unassigned,  Open-Drain, Digital

    mov  P0MDOUT,   #00Dh
    mov  XBR0,      #002h
    mov  XBR1,      #040h


Oscillator_Init:
    mov  OSCICN,    #083h

PCA_Init:
    anl  PCA0MD,    #0BFh
    mov  PCA0MD,    #000h





;-----------------------------------------------------------------------------
; PROGRAMA SPI
;-----------------------------------------------------------------------------

ADDR0 EQU 124 ;LSB of MMC memory address to write 
ADDR1 EQU 125 
ADDR2 EQU 126 
ADDR3 EQU 127 ;MSB of MMC memory address to write 
ERRORNUM EQU 120 ;error number 

;LLenado de memoria
mov r0,#00h
mov	r1,#0fh
mov dptr,#0000h
mov	A,#88h
itera:
movx	@dptr,A
;inc		A
inc		dptr
djnz	r0,itera
nop
djnz	r1,itera
mov		dptr,#0fffh


;-----------------------------------------------------------------------------
; CODE SEGMENT
;-----------------------------------------------------------------------------
;------------------------------------------------------------------------------- 
;main routine 


SETB p0.0 ;SPI pins must be high before set SPI mode 
SETB P0.1 ; they are after reset but let's be sure 
SETB P0.2; as per the atmel manual 
SETB P0.3 

;----------------------------------Initialize MultiMediaCard 
MOV ADDR3,#0 
MOV ADDR2,#0 
MOV ADDR1,#0 
MOV ADDR0,#0 
ACALL initmmc 

mov			R4,#000h			;direcc MSB de BYTE a leer y/o escribir
mov			R5,#000h			;
mov			R6,#000h			;
mov			R7,#000h			;direcc LSB de BYTE a leer y/o escribir
ACALL		read_sector

mov			R4,#000h			;direcc MSB de BYTE a leer y/o escribir
mov			R5,#000h			;
mov			R6,#000h			;
mov			R7,#000h			;direcc LSB de BYTE a leer y/o escribir
ACALL		write_sector


esperar:
sjmp		esperar

;------------------------------------------------------------------------------- 
;----------------------------------Initialize MultiMediaCard 
initmmc: 
SETB P0.3 ;Take CS* high 
MOV A,#0FFh
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 

CLR P0.3 ;take CS* low to select card 

MOV A,#0FFH 
ACALL spiout 
MOV A,#0FFH 
ACALL spiout 

MOV A,#0 ;send CMD0 to enable SPI mode on MMC 
ACALL cmd 
MOV A,#0FFH ;enable the MMC to finish 
ACALL spiout 

notready: 
MOV A,#41d
ACALL cmd 

CJNE R0,#41d,ready ;return if the MMC is ready 
MOV A,#0FFH ;enable the MMC to finish 
ACALL spiout 
SJMP notready 

ready: 
MOV A,#0FFH ;enable the MMC to finish if we got here 
ACALL spiout 

SETB P0.3	
MOV			A,#0FFh
ACALL		spiout
MOV			A,#0FFh
ACALL		spiout

RET 

;------------------------------------------------------------------------------- 
;----------------------------------Send a byte from ACC to MOSI, result=R0 
spiout: 
MOV SPI0DAT,A 

waitspi:
MOV A,SPI0CN ;read the status register 
ANL A,#10000000b ;upper bit=1 when data is gone 
CJNE A,#10000000b,waitspi ;if only bit 7 set, the data is gone 
ANL	SPI0CN,#01111111b

MOV R0,SPI0DAT 

RET 


;------------------------------------------------------------------------------- 
;----------------------------------Send a CMD to the MMC 
cmd: 
ORL A,#01000000b ;CMD's always have bit 7 set 
ACALL spiout ;send command byte 
MOV A,ADDR3 ;send 4 argument bytes 
ACALL spiout 
MOV A,ADDR2 
ACALL spiout 
MOV A,ADDR1 
ACALL spiout 
MOV A,ADDR0 
ACALL spiout 
MOV A,#95H ;checksum, 95H for CMD0, ignored for rest 
ACALL spiout 

MOV R1,#11 ;read up to 8 times for valid response 
checkr: 
MOV A,#0FFH 
ACALL spiout 
CJNE R0,#255d,response 
DJNZ R1,checkr 

MOV ERRORNUM,#1 ;didn't get a valid response, stop 
AJMP error 

response: ;can't check to see if good: cmd0/1 not 
RET 

error:
sjmp error

;------------------------------------------------------------------------------- 
;------------------------LECTURA DE 1 SECTOR
read_sector:

MOV			A,#0FFh
ACALL		spiout

CLR			P0.3			;Select card

MOV			A,#51h			;envio comando 24 escritura bloque
ACALL		spiout
MOV			A,R4			;direccion MSB
ACALL		spiout
MOV			A,R5			;direccion
ACALL		spiout
MOV			A,R6			;direccion
ACALL		spiout
MOV			A,R7			;direccion LSB
ACALL		spiout
MOV			A,#00h			;CRC
ACALL		spiout
MOV			A,#0FFh			;NCR time
ACALL		spiout

mov			B,#0FFh
espera_token:
MOV			A,#0FFh			;respuesta de comando, deveria ser 00h
DJNZ		B,espera_token2

tiempo_fuera:
sjmp		tiempo_fuera

espera_token2:
ACALL		spiout
CJNE		R0,#0FEh,espera_token

lectura_correcta:
sjmp		lectura_correcta


recibo_512:					;envio 512 bytes
mov 		R2,#00h
mov			R3,#02h
mov 		dptr,#0000h
itera_read:
movx		A,@DPTR
ACALL		spiout
inc			DPTR
djnz		R2,itera_read
djnz		R3,itera_read

MOV			A,#00h			;CRC
ACALL		spiout
MOV			A,#00h			;CRC
ACALL		spiout
MOV			A,#0FFh			;Lectura respuesta comando
ACALL		spiout

MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout


SETB		P0.3				;Deselecciono Memoria

RET

;------------------------------------------------------------------------------- 
;------------------------ESCRITURA DE 1 SECTOR

write_sector:

MOV			A,#0FFh
ACALL		spiout

CLR			P0.3			;Select card

MOV			A,#58h			;envio comando 24 escritura bloque
ACALL		spiout
MOV			A,R4			;direccion MSB
ACALL		spiout
MOV			A,R5			;direccion
ACALL		spiout
MOV			A,R6			;direccion
ACALL		spiout
MOV			A,R7			;direccion LSB
ACALL		spiout
MOV			A,#00h			;CRC
ACALL		spiout
MOV			A,#0FFh			;NCR time
ACALL		spiout
MOV			A,#0FFh			;respuesta de comando, deveria ser 00h
ACALL		spiout
MOV			A,#0FFh			;byte espacio
ACALL		spiout
MOV			A,#0FEh			;Data token
ACALL		spiout


envio_512:					;envio 512 bytes
mov 		R2,#00h
mov			R3,#02h
mov 		dptr,#0000h
itera2:
movx		A,@DPTR
ACALL		spiout
inc			DPTR
djnz		R2,itera2
djnz		R3,itera2

MOV			A,#00h			;CRC
ACALL		spiout
MOV			A,#00h			;CRC
ACALL		spiout
MOV			A,#0FFh			;Lectura respuesta comando
ACALL		spiout

MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout
MOV			A,#0FFh			;nada
ACALL		spiout


SETB		P0.3				;Deselecciono Memoria

RET




;-------------------------------------------------------------------------------
END
