;---------------------------------------------
;---------------------------------------------
; Title: HVAC Control
;---------------------------------------------
; Purpose: 
;	The program controls a heating, ventilation, and air conditioning (HVAC) system
;   	based on a reference temperature and a measured temperature.
;   	The program compares the temperatures and activates heating or cooling accordingly.
;   	The temperatures are then converted to their decimal digit representations for display.
;
; Dependencies: NONE
; Compiler: 	MPLABX IDE v6.20
; Author: 		Huy Nguyen 
; OUTPUTS: 
;       PORTD1 Heating Control with LED
;	PORTD2 Cooling Control with LED
;	
; INPUTS: 
;        REG 0x20 refTemp  - Reference temperature
;   	 REG 0x21 measTemp - Measured temperature
;	 REG 0x22 contReg  - Control Register
;	
; Versions:
;  	V1.0: 03/08/2025 
;  	V1.1: 03/09/2025 - function to handle negative measTemp
;---------------------------------------------
;
#include "MyConfig.inc"
#include <xc.inc>
;
;---------------------------------------------
; PROGRAM INPUTS
;---------------------------------------------
;
#define  measTempInput	45  ; this is the input value (R5)
#define  refTempInput	25  ; this is the input value (R4)
;
;---------------------------------------------
; Output Register definitions (R6)
;---------------------------------------------
;
#define	SWITCH	LATD,0  
#define LED1	PORTD,1		; heating	
#define LED2	PORTD,2		; cooling
;    
;---------------------------------------------
; Input Register definitions (R8)
;---------------------------------------------
;
refTemp		EQU	0x20	
measTemp	EQU	0x21	
contReg 	EQU	0x22	
;
;---------------------------------------------
; Data Register Definitions
;---------------------------------------------
; for requiments - R9
refTemp_dec1	EQU	0x60
refTemp_dec10	EQU 0x61
refTemp_dec100	EQU 0x62

; for requiments - R9
measTemp_dec1	EQU 0x70
measTemp_dec10	EQU 0x71
measTemp_dec100 EQU 0x72
;
;---------------------------------------------
; Main Program
;---------------------------------------------	
; The next 4 steps of the initialization process are copied
; from LAB5's 'AssemblyDelay.asm'. 
 
    PSECT absdata,abs,ovrld		; Do not change
    ORG          0              ; Reset vector
    GOTO        _start

    ORG          0x20           ; Begin assembly at 0x20 (R7)
	
_start:	
    
    CLRF	TRISD	; initialize PORTD as output
    
    CLRF	STATUS	; initialize STATUS Register
	
    ; Load input values
    MOVLW	refTempInput
    MOVWF	refTemp,1
    MOVLW	measTempInput
    MOVWF	measTemp,1

_isNegative:	; Check if measTemp is negative (bit 7 = 1)
    BTFSC   measTemp,7,1        ; Skip if bit 7 is clear (positive)
    GOTO    HEAT_ON             ; If negative, measTemp < refTemp, run HEAT_ON

_main:   			; Compare measTemp to refTemp
    MOVFF	measTemp, WREG
    CPFSEQ	refTemp,1 	; if measTemp = refTemp skip next instruction 
    GOTO	COMPARE_TEMPS	; measTemp ≠ refTemp, need to determine heating or cooling
    
    ; do nothing when measTemp = refTemp (R3)
    CLRF	contReg,1       ; Set contReg to 0
    BCF		LED1,1		; Turn off Heating
    BCF		LED2,2		; Turn off Cooling
    GOTO	_isNegative

COMPARE_TEMPS:
    MOVFF	measTemp, WREG  ; Reload measTemp into WREG
    CPFSGT	refTemp,1 	; if refTemp > measTemp skip next instruction
    GOTO	COOL_ON		; measTemp > refTemp, so go to COOL_ON (R1)
    GOTO	HEAT_ON		; measTemp < refTemp, so go to HEAT_ON (R2)

HEAT_ON:				; measTemp < refTemp, start heating process
    MOVLW	0x01
    MOVWF	contReg,1
    BCF		LED2,2		; Turn off Cooling
    BSF		LED1,1		; Turn on Heating
    GOTO	_isNegative	 

COOL_ON:				; measTemp > refTemp, start cooling process
    MOVLW	0x02
    MOVWF	contReg,1
    BCF		LED1,1		; Turn off Heating
    BSF		LED2,2		; Turn on Cooling
    GOTO	_isNegative

END
