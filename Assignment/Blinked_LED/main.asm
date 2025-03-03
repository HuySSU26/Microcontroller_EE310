;---------------------
; Title: Lab 4 - Part B - Blink LED
;---------------------
; Program Details:
; 	 Using BRANCH instructions write a program to blink the LED 5 times and then
; 	 the program should stop. The LED blink codes are repurposed from Lecture Assignment 2
; 	 and can run independently when the CALL instructions are replaced with NOP.  A counter
;	 function is added to keep track of the number of blink.  A delay function is also added
;	 as in Part A.
;
; Inputs:  WREG
; Outputs: PORTC
; Date: Feb 23, 2025
; File Dependencies / Libraries: None
; Compiler: PIC18F4520
; Author: Huy Nguyen
; Versions: V0
;------------------------------------------------------
#include <xc.inc>
BLKCNT  EQU     0x11	    ; assign address loc 11h to blink counter
DLYCNT1 EQU     0x12        ; assign address loc 12h to delay counter 1
;DLYCNT2 EQU     0x13        ; assign address loc 13h to delay counter 2

        ORG     0x10        ; start at address loc 10h
;START
	MOVLW   0x00        ; write value 0h to WREG (clear WREG)
        MOVWF   TRISC       ; initialize Port C as output Port
        MOVLW   0x05        ; set number of count to 5 blinks
        MOVWF   BLKCNT      ; store number of count in blink counter register

LOOP:                       ; repurpose LED blink subroutine
        MOVLW   0x01        ; write value 1h to WREG
        MOVWF   PORTC       ; turn on bit-0 of 8XLED
        ;CALL    DELAY       ; time delay
        NOP

        MOVLW   0x00        ; write value 0h to WREG
        MOVWF   PORTC       ; turn off bit-0 of 8XLED
        ;CALL    DELAY       ; time delay
        NOP

        DECF    BLKCNT,F    ; decrement blink counter once, keep result there
	;CALL    DELAY      ; time delay
        BNZ     LOOP        ; if blink counter is not zero, loop to keep LED blinking

;STOP    GOTO    STOP        ; stop program

DELAY   ;ORG     0x300       ; start delay routine at address loc 300h
        MOVLW   0xF        ; write value 02h into WREG for outer loop
        MOVWF   DLYCNT1     ; initialize outer delay counter
AGAIN   NOP
	NOP
;OUTER   MOVLW   0x02        ; write value 02h into WREG for inner loop
        ;MOVWF   DLYCNT2     ; initialize inner delay counter
;INNER   NOP                 ; no operation wastes clock cycles
        ;DECF    DLYCNT2,F   ; decrement inner counter
        ;BNZ     INNER       ; if inner counter not zero, continue inner loop
        DECF    DLYCNT1,F   ; decrement outer counter
        BNZ     AGAIN       ; if outer counter not zero, continue outer loop
        RETURN              ; return from delay subroutine

        ;SLEEP

        ;END
