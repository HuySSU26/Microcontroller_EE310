;---------------------
; Title: Single Digit Counter
;---------------------
; Purpose:
;   The program is a single digit counter which consist of a 7-segment LED and  
;   two momentary contact switches, switch A and switch B, interfaced with a  
;   PIC18F47K42 micro controller unit' GPIOs. When switch A is depressed,
;   the 7-segment will start to increment starting form 0 to 0x0F (15 in decimal). 
;   If switch A is let go, the incrementing sequence stops.  At this point, 
;   if switch B is depressed, the 7-segment decrement from the point where switch A 
;   stops. Hence, activating switch B enables the system to work in the reverse 
;   manner as when switch A is activated. When both switches are depressed, the
;   7-segment will reset to 0
;      
; Inputs: 
;   Switch A - PORTA,0 - Normally set High for no contact (when pressed goes Low) 
;   Switch B - PORTA,1 - Normally set High for no contact (when pressed goes Low) 
; Outputs: 
;   PORTD [6:0]   
; Date: March 14, 2025
; File Dependencies / Libraries: 
;   The is required to include the MyConfig.inc in the Header Folder. 
;   It is also required any FunctionCall.inc folder created to simply counter.asm 
; Compiler: MPLABX IDE, v6.20
; Author: Huy Nguyen
; Versions:
;   V1.0: 03/14/2025 - Original
;   V1.1: 03/16/2025 - Reworked switches operation:
;	Redefine PORTB,0 and 1. Enable weak pull-up register to have inputs' state set as normally HIGH (no contact)
;	Redefine PORTD [7:0] as all outputs   	
;   V1.2: 03/16/2025 - Reworked _display function and physical connections between 7-Segment and PIC18F4K42
;	Use the case-switch approach with the corrected 7-segment's pins mapping.
;   V1.3: 03/17/2025 - Changed _display function to use lookup table and pointer approach
;	instead of case-switch approach for more efficient code.
;   V1.4: 03/18/2025 - Redefine inputs, SW_A and SW_B, to PORTA [1 : 0] in preparation for revising the code
;	and use PORTB to interface with  a 4x3 keypad.		  	
;
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
Inner_loop  equ 	165		; loop count in decimal 
Outer_loop  equ 	200
High_loop   equ		5 
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
#define SW_A    PORTA,0  	; Switch A connected to RA0 
#define SW_B    PORTA,1		; Switch B connected to RA1
;----------------------------------------------------------------
;----------------  Main Program  --------------------------------
;----------------------------------------------------------------
    PSECT absdata,abs,ovrld        ; Do not change 
    
    ORG          0                 ;Reset vector
    GOTO        _initialization
    ORG          0020H             ; Begin assembly at 0020H
	
    ; Lookup table for 7-segment display patterns (0-F)
    ; Based on pin mapping: RD0 (g), RD1 (f), RD2 (e), RD3 (d), RD4 (c), RD5 (b), RD6 (a)
    ORG          0x100             ; Place table at address 0x100
_segment_table:
    DB          0x7E    ; 0 (abcdef)   - 01111110
    DB          0x30    ; 1 (bc)       - 00110000
    DB          0x6D    ; 2 (abdeg)    - 01101101
    DB          0x79    ; 3 (abcdg)    - 01111001
    DB          0x33    ; 4 (bcfg)     - 00110011
    DB          0x5B    ; 5 (acdfg)    - 01011011
    DB          0x5F    ; 6 (acdefg)   - 01011111
    DB          0x70    ; 7 (abc)      - 01110000
    DB          0x7F    ; 8 (abcdefg)  - 01111111
    DB          0x7B    ; 9 (abcdfg)   - 01111011
    DB          0x77    ; A (abcefg)   - 01110111
    DB          0x1F    ; b (cdefg)    - 00011111
    DB          0x4E    ; C (adef)     - 01001110
    DB          0x3D    ; d (bcdeg)    - 00111101
    DB          0x4F    ; E (adefg)    - 01001111
    DB          0x47    ; F (aefg)     - 01000111
    
_initialization: 
    RCALL	_setupPortD
    RCALL 	_setupPortA
    CLRF 	COUNT                  ; Initialize counter to 0 
    RCALL 	_display               ; Display initial value (0) 
    
_main:
    ; Check if both switches are pressed 
    BTFSC 	SW_A                   ; Skip next instruction if SW_A is LOW (pressed) 
    GOTO 	_check_switchB         ; SW_A not pressed, check SW_B 
    BTFSC 	SW_B                   ; Skip next instruction if SW_B is LOW (also pressed) 
    GOTO 	_increment             ; Only SW_A pressed, increment 
    
    ; Both switches pressed - Reset 
    CLRF 	COUNT                  ; Reset counter to 0 
    RCALL 	_display               ; Display 0 
    RCALL 	_loopDelay             ; Delay 
    GOTO 	_main                  ; Return to main loop 
    
_check_switchB:					   ; check if only switch B is pressed.
    BTFSC 	SW_B                   ; Skip next instruction if SW_B is LOW (pressed) 
    GOTO 	_main              	   ; No switches pressed, do nothing 
    RCALL 	_decrement             ; Only SW_B pressed, decrement 
    GOTO 	_main                  ; Return to main loop 
    
_increment:
    INCF 	COUNT, F               ; Increment counter 
    MOVLW 	0x10                   ; Load decimal 16 into WREG 
    CPFSLT 	COUNT                  ; Skip next instruction if COUNT < 16, keep incrementing 
    CLRF 	COUNT                  ; Set upper boundery. Reset to 0 if COUNT ≥ 16 
    RCALL 	_display               ; Display updated count 
    RCALL 	_loopDelay             ; Delay 
    GOTO 	_main                  ; Return to main loop 
    
_decrement:
    MOVF 	COUNT, W               ; Move COUNT to WREG 
    BZ 		_lower_bound           ; If COUNT is 0, set lower boundery 
    DECF 	COUNT, F               ; Otherwise decrement count 
    RCALL 	_display               ; Display updated count 
    RCALL 	_loopDelay             ; Delay 
    RETURN
    
_lower_bound:
    MOVLW 	0x0F                   ; Load 15 (F) into WREG 
    MOVWF 	COUNT                  ; Set COUNT to F 
    RCALL 	_display               ; Display updated count 
    RCALL 	_loopDelay             ; Delay 
    RETURN
	
	
;----------------------------------------------------------------    
;---------- The Display Subroutine with Lookup Table ------------ 
;----------------------------------------------------------------
_display:
    ; Set up table pointer to point to the segment table
    MOVLW   LOW(_segment_table)     ; Load lower byte of table address
    MOVWF   TBLPTRL                 ; Store in TBLPTRL
    MOVLW   HIGH(_segment_table)    ; Load upper byte of table address
    MOVWF   TBLPTRH                 ; Store in TBLPTRH
    MOVLW   0                       ; Prepare to load upper byte with 0
    MOVWF   TBLPTRU                 ; Load TBLPTRU with 0
    
    ; Add offset to pointer based on COUNT value
    MOVF    COUNT, W                ; Get COUNT value into WREG
    ANDLW   0x0F                    ; Ensure value is between 0-15 (mask upper bits)
    ADDWF   TBLPTRL, F              ; Add offset to TBLPTRL
    BTFSC   STATUS, 0               ; Check if carry occurred
    INCF    TBLPTRH, F              ; If carry, increment TBLPTRH
    
    ; Read the pattern from the table and display it
    TBLRD*                          ; Read table data, no pointer increment
    MOVF    TABLAT, W               ; Get the pattern from TABLAT into WREG
    MOVWF   LATD                    ; Use LATD to display the pattern on PORTD
    RETURN
	
;----------------------------------------------------------------    
;----------  The Delay Subroutine ------------------------------- 
;----------------------------------------------------------------  
_loopDelay: 
    MOVLW       Inner_loop
    MOVWF       REG10
    MOVLW       Outer_loop
    MOVWF       REG11
    MOVLW       High_loop
    MOVWF       REG30
_loop1:
    DECF        REG10,1		    
    BNZ         _loop1
    MOVLW       Inner_loop 	    ; Re-initialize the inner loop for when the outer loop decrements. 
    MOVWF       REG10
    DECF        REG11,1 	    ; outer loop 
    BNZ         _loop1
    MOVLW       Outer_loop 	    ; Re-initialize the outer loop for when the high loop decrements. 
    MOVWF       REG11				
    DECF        REG30,1	    	    ; high loop 
    BNZ         _loop1
    RETURN
	
;----------------------------------------------------------------    
;----------  Initializing the I/O Ports-------------------------- 
;---------------------------------------------------------------- 
_setupPortD:
    BANKSEL	PORTD 	 
    CLRF	PORTD 			; Init PORTD 
    BANKSEL	LATD 			; Data Latch
    CLRF	LATD 	
    BANKSEL	ANSELD 	
    CLRF	ANSELD 			; digital I/O 
    BANKSEL	TRISD 	
    MOVLW	0b00000000 		; Set all PORTD pins as outputs for 7-segment display 
    MOVWF	TRISD 		 
    RETURN
 
_setupPortA:
    BANKSEL	PORTA 
    CLRF	PORTA 			; Init PORTA
    BANKSEL	LATA 			; Data Latch
    CLRF	LATA 
    BANKSEL	ANSELA 
    CLRF	ANSELA 			; digital I/O
    BANKSEL	TRISA ;
    MOVLW	0b00000011 		; Set RA0 and RA1 as inputs
    MOVWF	TRISA 			; RA0 and RA1 are inputs, RA[7:2] are outputs
    BANKSEL WPUA            		; Weak pull-up register for PORTA
    MOVLW   0b00000011      		; Enable weak pull-ups for RA0 and RA1
    MOVWF   WPUA            		; Keep switches set HIGH when not pressed
    RETURN    
    
    END
