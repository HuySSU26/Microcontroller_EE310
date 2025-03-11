;---------------------------------------------
;---------------------------------------------
; Title: HVAC Control
;---------------------------------------------
; Purpose: 
;	The program controls a heating, ventilation, and air conditioning (HVAC) system
;   	based on a reference temperature and a measured temperature.
;   	The program compares the temperatures and activates heating or cooling accordingly.
;   	The temperatures are also converted to their decimal digits representations as  
;	required and also for displaying purpose, if required in the future.
;
; Dependencies: NONE
; Compiler: 	MPLABX IDE v6.20
; Author: 	Huy Nguyen 
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
;	V1.2: 03/10/2025 - added conversion to decimal digits as required in R9 and R10
;	V1.3: 03/10/2025 - simplify the process of getting absolute value for negative inputs
;---------------------------------------------
;
#include "MyConfig.inc"
#include <xc.inc>
;
;---------------------------------------------
; PROGRAM INPUTS
;---------------------------------------------
;
#define  measTempInput	0XFD ; this is the input value (R5)
#define  refTempInput	15  ; this is the input value (R4)
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
refTemp_dec10	EQU 	0x61
refTemp_dec100	EQU 	0x62

; for requiments - R10
measTemp_dec1	EQU 	0x70
measTemp_dec10	EQU 	0x71
measTemp_dec100 EQU 	0x72
 
;---------------------------------------------
; Temporary registers used for HEX to decimal conversion operations
;---------------------------------------------
temp_value	EQU     0x30	; divided number/remainder
div_count	EQU     0x31	; division counter
;
;---------------------------------------------
; Main Program
;---------------------------------------------	
; The next 4 steps of the initialization process are copied
; from LAB5's 'AssemblyDelay.asm'. 
 
    PSECT absdata,abs,ovrld	; Do not change
    ORG          0              ; Reset vector
    GOTO        _start

    ORG          0x20           ; Begin assembly at 0x20 (R7)
	
_start:	
    CLRF	TRISD	; initialize PORTD as output
	
    ; Load input values
    MOVLW	refTempInput
    MOVWF	refTemp,1
    MOVLW	measTempInput
    MOVWF	measTemp,1

_main_1:	; Convert refTemp to decimal digits (R9)
    MOVFF   refTemp, temp_value
    CALL    _convert_to_decimal
    MOVFF   temp_value, refTemp_dec1
    MOVFF   div_count, refTemp_dec10
    CLRF    refTemp_dec100,1

_main_2:	; Convert measTemp to decimal digits (R10)
    MOVFF   measTemp, temp_value
    BTFSC   temp_value, 7, 1    ; Check if negative
    NEGF    temp_value,1	; If negative, convert to absolute value
    CALL    _convert_to_decimal
    MOVFF   temp_value, measTemp_dec1
    MOVFF   div_count, measTemp_dec10
    CLRF    measTemp_dec100,1

_isNegative:			; Check if measTemp is negative (bit 7 = 1)
    BTFSC   measTemp,7,1        ; Skip next instruction if bit 7 is clear (measTemp > 0)
    GOTO    HEAT_ON             ; measTemp < 0, run HEAT_ON

_main_3:   			; Compare measTemp to refTemp
    MOVFF	measTemp, WREG
    CPFSEQ	refTemp,1 	; if measTemp = refTemp skip next instruction 
    GOTO	_compare	; measTemp ≠ refTemp, need to determine heating or cooling
    
    ; do nothing when measTemp = refTemp (R3)
    CLRF	contReg,1   ; Set contReg to 0
    BCF		LED1		; Turn off Heating
    BCF		LED2		; Turn off Cooling
    BRA		_main_2		; Return to _main_2 to update displays

_compare:
    MOVFF	measTemp, WREG  ; Reload measTemp into WREG
    CPFSGT	refTemp,1 	; if refTemp > measTemp skip next instruction
    GOTO	COOL_ON		; measTemp > refTemp, so go to COOL_ON (R1)
    GOTO	HEAT_ON		; measTemp < refTemp, so go to HEAT_ON (R2)

HEAT_ON:			; measTemp < refTemp, start heating process
    MOVLW	0x01
    MOVWF	contReg,1
    BCF		LED2		; Turn off Cooling
    BSF		LED1		; Turn on Heating
    BRA		_main_2		

COOL_ON:			; measTemp > refTemp, start cooling process
    MOVLW	0x02
    MOVWF	contReg,1
    BCF		LED1		; Turn off Heating
    BSF		LED2		; Turn on Cooling
    BRA		_main_2		

;---------------------------------------------
; Subroutine used to convert HEX value to decimal
; by dividing the number's BCD by 10
;---------------------------------------------
_convert_to_decimal:
    CLRF    div_count, 1	
    
    MOVLW   10
    CPFSLT  temp_value, 1   ; Skip next instruction if temp_value < 10
    GOTO    _div_by_10
    RETURN		    ; Return to caller, _main_1 or 2
    
_div_by_10:		    ; looping subtraction to perform division
    MOVLW   10
    SUBWF   temp_value,1    
    INCF    div_count,1	    
    
    ; Check if we can subtract 10 again
    CPFSLT  temp_value, 1   ; Skip next instruction if temp_value < 10
    GOTO    _div_by_10	    ; loop back to subtraction operation
    RETURN		    ; Return to caller, _convert_to decimal

END
