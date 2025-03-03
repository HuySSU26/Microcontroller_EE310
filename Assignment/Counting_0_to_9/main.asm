;---------------------
; Title: Counting from 0 to 9
;---------------------
; Program Details:
; 	 The program is used to drive a 7-segment LED to count from 0 to 9
;	 It will loop indefinitely using OshonSoft PIC 18 simulator.
; Inputs:  WREG
; Outputs: PORTC-7 Segment LED display panel
; Date: Feb 23, 2025
; File Dependencies / Libraries: None 
; Compiler: PIC18F4520
; Author: Huy Nguyen
; Versions: V0
;       
; Useful links: 
;	N/A
;

;------------------------------------------------------
#include <xc.inc>

LOOP:
	;ORG	0X20; starting at REG20
	MOVLW	0X00
	MOVWF	TRISC; define port C as output
	MOVLW	0X3F; write number zero
	MOVWF	PORTC,0
	NOP
	MOVLW	0X06; write number 1
	MOVWF	PORTC,0
	NOP
	MOVLW	0X5B; write number 2
	MOVWF	PORTC,0
	NOP
	MOVLW	0X4F; write number 3
	MOVWF	PORTC,0
	NOP
	MOVLW	0X66; write number 4
	MOVWF	PORTC,0
	NOP
	MOVLW	0X6D; write number 5
	MOVWF	PORTC,0
	NOP
	MOVLW	0X7D; write number 6
	MOVWF	PORTC,0
	NOP
	MOVLW	0X07; write number 7
	MOVWF	PORTC,0
	NOP
	MOVLW	0X7F; write number 8
	MOVWF	PORTC,0
	NOP
	MOVLW	0X6F; write number 9
	MOVWF	PORTC,0
	NOP
	GOTO	LOOP

	SLEEP

;------------------------------------------------------
;------------------------------------------------------
