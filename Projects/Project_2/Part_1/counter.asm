;---------------------
; Title: Single Digit Counter
;---------------------
; Purpose:
;	  The program is a single digit counter which consist of a 7-segment LED and  
;	  two momentary contact switches, switch A and switch B, interfaced with a  
;	  PIC18F47K42 micro controller unit' GPIOs.  When a  switch A is depressed, 
;	  the 7-segment will start to increment starting form 0 to 0x0F (15 in decimal).
;	  If switch A is let go, the incrementing sequence stops.  At this point, 
;	  if switch B is depressed, the 7-segment decrement from the point where switch A 
;	  stops.  Hence, activating switch B enables the system to work in the reverse 
;	  manner as when switch A is activated.  When both switches are depressed, the
;	  7-segment will reset to 0
;      
; Inputs: 
;		Switch A - PORTB,0
;		Switch B - PORTB,1
; Outputs: 
;		PORTD [6:0]   
; Date: March 14, 2025
; File Dependencies / Libraries: 
;	The is required to include the MyConfig.inc in the Header Folder.
;   It is also required any FunctionCall.inc folder created to simplied counter.asm
; Compiler: MPLABX IDE, v6.20
; Author: Huy Nguyen
; Versions:
;       V1.0: 03/14/2025 - Original
;	V1.1: 03/16/2025 - Redefine PORTB,0 and 1. Enable weak pull-up register to have inputs' state set as normally HIGH (no contact)
;			 - Redefine PORTD [7:0] as all outputs 
; Useful links: 
;    Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/PIC18(L)F26-27-45-46-47-55-56-57K42-Data-Sheet-40001919G.pdf 
;    PIC18F Instruction Sets: https://onlinelibrary.wiley.com/doi/pdf/10.1002/9781119448457.app4 
;    List of Instrucions: http://143.110.227.210/faridfarahmand/sonoma/courses/es310/resources/20140217124422790.pdf 

;---------------------
; Initialization
;---------------------
#include "MyConfig.inc"
#include <xc.inc>

;----------------------------------------------------------------
; Delay loop Inputs to create ~ 0.5 second delay (8MHz clock speed)
;----------------------------------------------------------------
Inner_loop  equ 5		; loop count in decimal
Outer_loop  equ 5
High_loop   equ	5 
;----------------------------------------------------------------
; Program Constants
;----------------------------------------------------------------
REG10   equ     0x10		; inner loop address  
REG11   equ     0x11		; outer loop address
REG30	equ	0x30		; high loop address
COUNT   equ	0x31		; counter variable address
;----------------------------------------------------------------
; Definitions
;----------------------------------------------------------------
#define SW_A    PORTB,0  	; Switch A connected to RB0
#define SW_B    PORTB,1		; Switch B connected to RB1
#define S_SEG 	PORTD 		; 7-segment display connected to PORTD
;----------------------------------------------------------------
;----------------  Main Program  --------------------------------
;----------------------------------------------------------------
    PSECT absdata,abs,ovrld        ; Do not change
    
    ORG          0                 ;Reset vector
    GOTO        _initialization

    ORG          0020H            ; Begin assembly at 0020H
	
; 7-segment display lookup table (common cathode)
S_SEG_TABLE:
    DB 0x3F ; 0
    DB 0x06 ; 1
    DB 0x5B ; 2
    DB 0x4F ; 3
    DB 0x66 ; 4
    DB 0x6D ; 5
    DB 0x7D ; 6
    DB 0x07 ; 7
    DB 0x7F ; 8
    DB 0x6F ; 9
    DB 0x77 ; A
    DB 0x7C ; B
    DB 0x39 ; C
    DB 0x5E ; D
    DB 0x79 ; E
    DB 0x71 ; F
 
_initialization: 
    RCALL _setupPortD
    RCALL _setupPortB
    
_main:
    BTFSC 	SW_A 		; Check switch A. Skip next instruction if pressed (set Low)
    BTFSS 	SW_B 		; Check switch B. Skip next instruction if not pressed (set High)
    CALL 	_reset 		; if both are pressed, reset
    BTFSC 	SW_A		; Check switch A. If depressed, start incrementing
    CALL 	_increment
    BTFSC	SW_B 		; Check switch B. 
    CALL 	_decrement
    CALL 	_display 	; Display the current count
    CALL 	loopDelay       ; Implement the delay
    BRA		_main
	
_increment:
    INCF	COUNT,1 	; Increment counter
    MOVLW	0x10 		; load decimal value 16 into WREG
    CPFSLT	COUNT 		; if count < 16, skip next instruction.
    CLRF	COUNT 		; reset to 0 if COUNT â¥ 16 .
    CALL	_display
    RETURN
_decrement:
    DECF	COUNT, F	; Decrement counter
    MOVLW	0xFF		; load decimal value 255 into WREG
    CPFSEQ	COUNT		; if count â¡ 255, skip next instruction.
    RETURN			; return to _main if no underflow
    MOVLW	0x0F		; Load 15 into WREG
    MOVWF	COUNT		; Set COUNT to 15
    CALL	_display
    RETURN
_reset:
    CLRF COUNT ; Reset counter to 0
    CALL _display
    RETURN
_display:
    MOVFF COUNT, WREG 	; Move counter value to WREG
    ADDWF PCL,1 	; lookup table jump to the corresponding 7-segment value
    RETLW 0x3F ; 0
    RETLW 0x06 ; 1
    RETLW 0x5B ; 2
    RETLW 0x4F ; 3
    RETLW 0x66 ; 4
    RETLW 0x6D ; 5
    RETLW 0x7D ; 6
    RETLW 0x07 ; 7
    RETLW 0x7F ; 8
    RETLW 0x6F ; 9
    RETLW 0x77 ; A
    RETLW 0x7C ; B
    RETLW 0x39 ; C
    RETLW 0x5E ; D
    RETLW 0x79 ; E
    RETLW 0x71 ; F
	
;----------------------------------------------------------------    
;----------  The Delay Subroutine ------------------------------- 
;----------------------------------------------------------------  

loopDelay: 
    MOVLW       Inner_loop
    MOVWF       REG10
    MOVLW       Outer_loop
    MOVWF       REG11
    MOVLW       High_loop
    MOVWF       REG30
_loop1:
    DECF        REG10,1
    BNZ         _loop1
    MOVLW       Inner_loop 	; Re-initialize the inner loop for when the outer loop decrements.
    MOVWF       REG10
    DECF        REG11,1 	; outer loop
    BNZ        _loop1
    MOVLW       Outer_loop 	; Re-initialize the outer loop for when the high loop decrements.
    MOVWF       REG11		; high loop
    DECF        REG30,1	    
    BNZ        _loop1
    RETURN

 
_setupPortD:
    BANKSEL		PORTD 	;
    CLRF		PORTD 		; Init PORTD
    BANKSEL		LATD 		; Data Latch
    CLRF		LATD 	;
    BANKSEL		ANSELD 	;
    CLRF		ANSELD 		;digital I/O
    BANKSEL		TRISD 	;
    MOVLW		0b10000000 	;Set RD7 as input
    MOVWF		TRISD 		;and set PORTD [6:0] as outputs
    RETURN
 
_setupPortB:
    BANKSEL		PORTB ;
    CLRF		PORTB 		;Init PORTB
    BANKSEL		LATB 		;Data Latch
    CLRF		LATB ;
    BANKSEL		ANSELB ;
    CLRF		ANSELB 		;digital I/O
    BANKSEL		TRISB ;
    MOVLW		0b00000011 	;Set RB0 and RB1 as inputs (corrected)
    MOVWF		TRISB
    ; Enable pull-ups for the switch pins
    BANKSEL     	WPUB        ; Select weak pull-up register for PORTB
    MOVLW       	0b00000011  ; Enable pull-ups on RB0 and RB1
    MOVWF       	WPUB
    RETURN
    
    END
