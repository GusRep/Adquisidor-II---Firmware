$include (C8051F340.inc)


;---------------CONSTANTES-------------------------  

DB4     EQU   P1.4			;bit 4 del bus de datos
DB5     EQU   P1.5			;bit 5 del bus de datos
DB6     EQU   P1.6			;bit 6 del bus de datos
DB7     EQU   P1.7			;bit 7 del bus de datos

RS      EQU   P3.7			;selección de registro de datos (1) o de instrucción (0)
RW      EQU   P3.6			;selección de escritura (0) o lectura (1)
EN      EQU   P3.5			;habilitación
BKL			EQU   P3.7   			;control de luz de fondo

DATO    EQU   P1			;port de datos
;---------------FIN CONSTANTES---------------------  

		org		0
		ajmp	inicio


		org		100h
inicio:

;---------------SETEOS-------------------------  
    mov  P0MDOUT,   #001h
    mov  P0SKIP,    #003h
    mov  XBR1,      #040h
prueba3:
CPL	p0.0
CPL	p0.1
CPL	p0.2
sjmp prueba3



;    mov  P0MDOUT,   #008h
;    mov  P1MDOUT,   #00Fh
;    mov  P3MDOUT,   #0F0h
    mov  XBR0,      #002h
    mov  XBR1,      #040h

Oscillator_Init:
    mov  OSCICN,    #083h

PCA_Init:
    anl  PCA0MD,    #0BFh
    mov  PCA0MD,    #000h


;-----------PROGRAMA--------------------------- 

			LCALL		INIT_LCD
			LCALL		hello

espera:
			AJMP		espera



;-------------------------------------

      READ_2_NIBBLES:
       ORL   DATO,#0F0h     ;Be sure to release DATOlines (set outputlatches 
                           ;to '1') so we can read the LCD
       SETB  EN
       MOV  A,DATO  			;Read first part of the return value (high nibble)
       CLR   EN
       ANL  A,#0F0h         ;Only high nibble is usable
       PUSH  ACC
       SETB  EN
       MOV   A,DATO        ;Read second part of the return value (low nibble)
       CLR   EN 
       ANL   A,#0F0h       ;Only high nibble is usable
       SWAP  A             ;Last received is actually low nibble, so put it in place
       MOV   R7,A 
       POP   ACC 
       ORL   A,R7          ;And combine it with low nibble
       RET
 
 ;-------------------------------------

      WRITE_2_NIBBLES:
       PUSH  ACC           ;Save A for low nibble
       ORL   DATO,#0F0h    ;Bits 4..7 <- 1
       ORL   A,#0Fh        ;Don't affect bits 0-3
       ANL   DATO,A        ;High nibble to display
       SETB  EN 
       CLR   EN 
       POP   ACC           ;Prepare to send
       SWAP  A             ;...second nibble
       ORL   DATO,#0F0h    ; Bits 4...7 <- 1
       ORL   A,#0Fh       ; Don't affect bits 0...3
       ANL   DATO,A        ;Low nibble to display
       SETB  EN 
       CLR   EN 
       RET
       

;-------------------------------------

      WAIT_LCD:
        CLR RS ;It's a command
        SETB RW ;It's a read command
        LCALL READ_2_NIBBLES ;Take two nibbles from LCD in A
        JB ACC.7,WAIT_LCD ;If bit 7 high, LCD still busy
        CLR RW ;Turn off RW for future commands
        RET

;-------------------------------------
;INICIALIZACION

      INIT_LCD:
       CLR   RS
       CLR   RW
       CLR   EN
       SETB  EN
       MOV   DATO,#28h
       CLR   EN
       LCALL WAIT_LCD
       MOV   A,#28h
       LCALL WRITE_2_NIBBLES
       LCALL WAIT_LCD
       MOV   A,#0Eh
       LCALL WRITE_2_NIBBLES
       LCALL WAIT_LCD
       MOV   A,#06h
       LCALL WRITE_2_NIBBLES
       LCALL WAIT_LCD
       RET
       
;-------------------------------------
;CLEARING THE DISPLAY

      CLEAR_LCD:
       CLR   RS
       MOV   A,#01h
       LCALL WRITE_2_NIBBLES ;Write A as two separate nibbles to LCD
       LCALL WAIT_LCD
       RET

;-------------------------------------       
;WRITING TEXT TO THE LCD

      WRITE_TEXT:
       SETB  RS
       LCALL WRITE_2_NIBBLES
       LCALL WAIT_LCD
       RET

       
;-------------------------------------

;THE "HELLO WORLD" PROGRAM


hello:
       LCALL INIT_LCD
       LCALL CLEAR_LCD
       MOV A,#'H'
       LCALL WRITE_TEXT
       MOV A,#'E'
       LCALL WRITE_TEXT
       MOV A,#'L'
       LCALL WRITE_TEXT
       MOV A,#'L'
       LCALL WRITE_TEXT
       MOV A,#'O'
       LCALL WRITE_TEXT
       MOV A,#' '
       LCALL WRITE_TEXT
       MOV A,#'W'
       LCALL WRITE_TEXT
       MOV A,#'O'
       LCALL WRITE_TEXT
       MOV A,#'R'
       LCALL WRITE_TEXT
       MOV A,#'L'
       LCALL WRITE_TEXT
       MOV A,#'D'
       LCALL WRITE_TEXT

       RET

;-------------------------------------




;===============================================
END