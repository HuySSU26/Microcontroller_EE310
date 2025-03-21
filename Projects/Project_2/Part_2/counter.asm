;---------------------
; Title: Fixed Single Digit Counter with Keypad Interface
;---------------------
; Purpose:
;   The program is a single digit counter which consists of a 7-segment LED and  
;   a 4x3 keypad interfaced with a PIC18F47K42 micro controller unit's GPIOs.
;   - '1' key when held down auto-increments the counter
;   - '2' key when held down auto-decrements the counter
;   - '3' key resets the counter to 0
;      
; Inputs: 
;   Keypad connected to PORTB
;   - Rows (R1-R4): RB3, RB4, RB6, RB7 (input pins with 11KΩ pull-down resistors)
;   - Columns (C1-C3): RB0, RB1, RB2   (output pins)
; Outputs: 
;   PORTD [6:0] - 7-segment display   
; Date: March 20, 2025
; File Dependencies / Libraries: 
;   The is required to include the MyConfig.inc in the Header Folder. 
;   It is also required any FunctionCall.inc folder created to simply counter.asm 
; Compiler: MPLABX IDE, v6.20
; Author: Huy Nguyen 
;; Versions:
;   V1.0: 03/14/2025 - Original
;   V1.1: 03/16/2025 - Reworked switches operation:
;		  Redefine PORTB,0 and 1. Enable weak pull-up register to have inputs' state set as normally HIGH (no contact)
;		  Redefine PORTD [7:0] as all outputs   	
;	V1.2: 03/16/2025 - Reworked _display function and physical connections between 7-Segment and PIC18F4K42
;		  Use the case-switch approach with the corrected 7-segment's pins mapping.
;   V1.3: 03/17/2025 - Changed _display function to use lookup table and pointer approach
;		  instead of case-switch approach for more efficient code.
;   V1.4: 03/18/2025 - Redefine inputs, SW_A and SW_B, to PORTA [1 : 0] in preparation for revising the code
;		  and use PORTB to interface with  a 4x3 keypad.
;	V2.0: 03/18/2025 - Add keypad definitions and constants and replace switch operations with keypad
;	V2.1: 03/19/2025 - Use existing time dealy loop for key debouncing and signal statiblization
;	V2.3: 03/20/2025 - Create shorter time delay loop for waiting for signal stabilization
;	V2.4: 03/20/2025 - Rework keypad scanning logic operations according to actual PORTB's hardware configuration


;---------------------
; Initialization
;---------------------
#include "MyConfig.inc"
#include <xc.inc>

;----------------------------------------------------------------
; Delay loop Inputs to create ~ 0.5 second delay (8MHz clock speed) 
;----------------------------------------------------------------
Inner_loop  equ     165     ; loop count in decimal 
Outer_loop  equ     200
High_loop   equ     5       ; Reduced for faster response

;----------------------------------------------------------------
; Program Constants
;----------------------------------------------------------------
REG10   equ     0x10    ; inner loop address  
REG11   equ     0x11    ; outer loop address
REG30   equ     0x30    ; high loop address
COUNT   equ     0x31    ; counter variable address
KEY     equ     0x32    ; current key pressed
TEMP    equ     0x33    ; temporary storage
LASTKEY equ     0x34    ; last key pressed for debouncing

;----------------------------------------------------------------
; Keypad Definitions and Constants
;----------------------------------------------------------------
; Column pins (outputs)
#define COL1    0       ; PORTB,0
#define COL2    1       ; PORTB,1
#define COL3    2       ; PORTB,2

; Row pins (inputs)
#define ROW1    3       ; PORTB,3
#define ROW2    4       ; PORTB,4
#define ROW3    6       ; PORTB,6
#define ROW4    7       ; PORTB,7

; Key values
KEY_NONE    equ     0xFF    ; No key pressed
KEY_ONE     equ     0x01    ; '1' key - increment
KEY_TWO     equ     0x02    ; '2' key - decrement
KEY_THREE   equ     0x03    ; '3' key - reset

;----------------------------------------------------------------
;----------------  Main Program  --------------------------------
;----------------------------------------------------------------
    PSECT absdata,abs,ovrld        ; Do not change 

    ORG     0                   ; Reset vector
    GOTO    _initialization
    ORG     0020H               ; Begin assembly at 0020H
    
    ; Lookup table for 7-segment display patterns (0-9)
    ; Based on pin mapping: RD0 (g), RD1 (f), RD2 (e), RD3 (d), RD4 (c), RD5 (b), RD6 (a)
    ORG     0x100               ; Place table at address 0x100
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
    RCALL   _setupPortD       ; Setup 7-segment display port
    RCALL   _setupPortB       ; Setup keypad port
    
    ; Initialize variables
    MOVLW   KEY_NONE
    MOVWF   KEY              ; Initialize KEY to "no key pressed"
    MOVWF   LASTKEY          ; Initialize LASTKEY to "no key pressed"
    
    ; Clear counter to 0 and display it
    CLRF    COUNT            ; Initialize counter to 0
    RCALL   _loopDelay   ; Short delay for system stabilization
    RCALL   _display         ; Display initial value (0)
    
    ; Wait for system to stabilize
    RCALL   _loopDelay
    
_main:
    ; Reset column pins for next scan
    RCALL   _resetColumns
    
    ; Scan keypad and store result in KEY
    RCALL   _scanKeypad
    
    ; Check if a key is pressed
    MOVF    KEY, W
    XORLW   KEY_NONE
    BZ      _no_key_pressed   ; If no key, handle it separately
    
    ; Process the key press
    GOTO    _process_key
    
_no_key_pressed:
    ; No key pressed, reset LASTKEY
    MOVLW   KEY_NONE
    MOVWF   LASTKEY
    GOTO    _main
    
_process_key:
    ; Check if this is a new key press by comparing to LASTKEY
    MOVF    KEY, W
    CPFSEQ  LASTKEY          ; Skip if KEY = LASTKEY (same key still pressed)
    GOTO    _new_key_press   ; Process as new key press
    
    ; Same key still pressed - handle auto-repeat for 1 and 2
    MOVF    KEY, W
    XORLW   KEY_ONE
    BZ      _increment       ; If '1' still pressed, increment
    
    MOVF    KEY, W
    XORLW   KEY_TWO
    BZ      _decrement       ; If '2' still pressed, decrement
    
    ; For other keys, no auto-repeat
    GOTO    _main
    
_new_key_press:
    ; Save the current key as LASTKEY
    MOVF    KEY, W
    MOVWF   LASTKEY
    
    ; Process based on which key was pressed
    XORLW   KEY_ONE
    BZ      _increment       ; If '1' pressed, increment
    
    MOVF    KEY, W
    XORLW   KEY_TWO
    BZ      _decrement       ; If '2' pressed, decrement
    
    MOVF    KEY, W
    XORLW   KEY_THREE
    BZ      _reset           ; If '3' pressed, reset
    
    ; If any other key, ignore and continue scanning
    GOTO    _main
    
_reset:
    CLRF    COUNT           ; Reset counter to 0
    RCALL   _display
    RCALL   _loopDelay  	; Use loop delay for debouncing.
    GOTO    _main
    
_increment:
    INCF    COUNT, F        ; Increment counter
    MOVLW   0x10            ; Load decimal 16
    CPFSLT  COUNT           ; Skip next instruction if COUNT < 16
    CLRF    COUNT           ; Set upper boundary: Reset to 0 if COUNT ≥ 16
    RCALL   _display        ; Display updated count
    RCALL   _loopDelay       ; Delay for auto-repeat rate
    GOTO    _main

_decrement:
    MOVF    COUNT, W        ; Move COUNT to WREG
    BZ      _set_to_F       ; If COUNT is 0, set to F (for 0-F counter)
    DECF    COUNT, F        ; Otherwise decrement count
    RCALL   _display        ; Display updated count
    RCALL   _loopDelay       ; Delay for auto-repeat rate
    GOTO    _main

_set_to_F:
    MOVLW 	0x0F            ; Load 15 (F) into WREG 
    MOVWF 	COUNT           ; Set COUNT to F 
    RCALL 	_display        ; Display updated count 
    RCALL 	_loopDelay       ; Delay for auto-repeat rate
    GOTO 	_main


_resetColumns:
    BANKSEL LATB
    ; Set all columns HIGH (inactive)
    BSF     LATB, COL1
    BSF     LATB, COL2
    BSF     LATB, COL3
    RETURN

;----------------------------------------------------------------    
;---------------- Keypad Scanning Subroutine -------------------- 
;----------------------------------------------------------------
_scanKeypad:
    ; Initialize KEY to "no key pressed"
    MOVLW   KEY_NONE
    MOVWF   KEY

    ;-------------------------------------------
    ; Scan Column 1 (Key 1)
    ;-------------------------------------------
    BANKSEL LATB
    ; Set Column 1 active (HIGH), others inactive (LOW)
    BSF     LATB, COL1      ; Set C1 HIGH (active)
    BCF     LATB, COL2      ; Set C2 LOW (inactive)
    BCF     LATB, COL3      ; Set C3 LOW (inactive)
    RCALL   _shortDelay     ; Small delay for signal stabilization
    
    BANKSEL PORTB
    ; Check Row 1 (Key 1) - With pull-down resistors, pressed key = HIGH
    BTFSS   PORTB, ROW1     ; Skip if row pin is HIGH (button pressed)
    GOTO    _check_col2     ; Not pressed, check next column
    
    ; Key 1 is pressed
    MOVLW   KEY_ONE
    MOVWF   KEY
    RETURN                  ; Return immediately when key found

_check_col2:
    ;-------------------------------------------
    ; Scan Column 2 (Key 2)
    ;-------------------------------------------
    BANKSEL LATB
    ; Set Column 2 active (LOW), others inactive (HIGH)
    BCF     LATB, COL1      ; Set C1 LOW (inactive)
    BSF     LATB, COL2      ; Set C2 HIGH (active)
    BCF     LATB, COL3      ; Set C3 LOW (inactive)
    RCALL   _shortDelay     ; Small delay for signal stabilization
    
    BANKSEL PORTB
    ; Check Row 1 (Key 2) - With pull-down resistors, pressed key = HIGH
    BTFSS   PORTB, ROW1     ; Skip if row pin is HIGH (button pressed)
    GOTO    _check_col3     ; Not pressed, check next column
    
    ; Key 2 is pressed
    MOVLW   KEY_TWO
    MOVWF   KEY
    RETURN                  ; Return immediately when key found

_check_col3:
    ;-------------------------------------------
    ; Scan Column 3 (Key 3)
    ;-------------------------------------------
    BANKSEL LATB
    ; Set Column 3 active (HIGH), others inactive (LOW)
    BCF     LATB, COL1      ; Set C1 LOW (inactive)
    BCF     LATB, COL2      ; Set C2 LOW (inactive)
    BSF     LATB, COL3      ; Set C3 HIGH (active)
    RCALL   _shortDelay     ; Small delay for signal stabilization
    
    BANKSEL PORTB
    ; Check Row 1 (Key 3) - With pull-down resistors, pressed key = HIGH
    BTFSS   PORTB, ROW1     ; Skip if row pin is HIGH (button pressed)
    GOTO    _no_key         ; No key pressed
    
    ; Key 3 is pressed
    MOVLW   KEY_THREE
    MOVWF   KEY
    RETURN                  ; Return immediately when key found

_no_key:
    ; No key was pressed - reset all columns to inactive
    RCALL   _resetColumns
    RETURN

;----------------------------------------------------------------    
;---------- The Display Subroutine with Lookup Table ------------ 
;----------------------------------------------------------------
_display:
    ; Set up table pointer to point to the segment table
    MOVLW   LOW(_segment_table)  ; Load lower byte of table address
    MOVWF   TBLPTRL              ; Store in TBLPTRL
    MOVLW   HIGH(_segment_table) ; Load upper byte of table address
    MOVWF   TBLPTRH              ; Store in TBLPTRH
    CLRF    TBLPTRU              ; Clear upper byte (ensure it's 0)
    
    ; Add offset to pointer based on COUNT value
    MOVF    COUNT, W             ; Get COUNT value into WREG
    ANDLW   0x0F                 ; Ensure value is between 0-15 (mask upper bits)
    
    ; Add offset to table pointer
    ADDWF   TBLPTRL, F           ; Add offset to TBLPTRL
    BTFSC   STATUS, 0            ; Check if carry occurred
    INCF    TBLPTRH, F           ; If carry, increment TBLPTRH
    
    ; Read the pattern from the table and display it
    TBLRD*                       ; Read table data, no pointer increment
    MOVF    TABLAT, W            ; Get the pattern from TABLAT into WREG
    BANKSEL LATD
    MOVWF   LATD                 ; Use LATD to display the pattern on PORTD
    RETURN
	
;----------------------------------------------------------------    
;----------  The Delay Subroutines ------------------------------ 
;----------------------------------------------------------------  
_loopDelay: 
    MOVLW       Inner_loop
    MOVWF       REG10
    MOVLW       Outer_loop
    MOVWF       REG11
    MOVLW       High_loop
    MOVWF       REG30
_loop1:
    DECF        REG10, F
    BNZ         _loop1
    MOVLW       Inner_loop 		; Re-initialize the inner loop for when the outer loop decrements. 
    MOVWF       REG10
    DECF        REG11, F 		; outer loop 
    BNZ         _loop1
    MOVLW       Outer_loop 		; Re-initialize the outer loop for when the high loop decrements. 
    MOVWF       REG11				
    DECF        REG30, F	    	; high loop 
    BNZ         _loop1
    RETURN

; Shorter delay for keypad scanning
_shortDelay:
    MOVLW       100             ; Short delay for signal stabilization
    MOVWF       TEMP
_shortLoop:
    DECF        TEMP, F
    BNZ         _shortLoop
    RETURN
	
;----------------------------------------------------------------    
;----------  Initializing the I/O Ports-------------------------- 
;---------------------------------------------------------------- 
_setupPortD:
    BANKSEL	PORTD 	 
    CLRF	PORTD 			; Init PORTD 
    BANKSEL	LATD 			; Data Latch
    CLRF	LATD 	        ; Clear LATD to ensure all segments are off initially
    BANKSEL	ANSELD 	
    CLRF	ANSELD 			; digital I/O 
    BANKSEL	TRISD 	
    MOVLW	0b00000000 		; Set all PORTD pins as outputs for 7-segment display 
    MOVWF	TRISD 		 
    RETURN
 
_setupPortB:
    BANKSEL	PORTB 
    CLRF	PORTB 			; Init PORTB
    BANKSEL	LATB 			; Data Latch
    CLRF	LATB 
    
    ; Set initial state of column pins to HIGH (inactive)
    BSF     LATB, COL1
    BSF     LATB, COL2
    BSF     LATB, COL3
    
    BANKSEL	ANSELB 
    CLRF	ANSELB 			; digital I/O for all pins
    
    BANKSEL	TRISB
    ; Columns as outputs (RB0-RB2), rows as inputs (RB3-RB4, RB6-RB7)
    MOVLW	0b11111000 		; RB0-RB2 outputs, RB3-RB7 inputs
    MOVWF	TRISB
    
    ; Using external pull-down resistors on PORTB,3,4,6,7
    BANKSEL WPUB
    CLRF    WPUB            ; Disable all internal pull-ups
    
    RETURN    
    
    END
