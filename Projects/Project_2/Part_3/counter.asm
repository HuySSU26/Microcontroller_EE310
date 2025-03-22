;---------------------
; Title: Combined Single Digit Counter with Keypad and Switch Input
;---------------------
; Purpose:
;   The program is a single digit counter which consists of a 7-segment LED and
;   a 4x3 keypad and two momentary contact switches, switch A and switch B,
;   interfaced with a PIC18F47K42 microcontroller unit's GPIOs.
;
;   - '*' key when held down auto-increments the counter
;   - '#' key when held down auto-decrements the counter
;   - '0' key resets the counter to 0
;   - When switch A is depressed, the 7-segment will start to increment starting from 0 to 0x0F (15 in decimal).
;   - If switch A is let go, the incrementing sequence stops. At this point,
;   - if switch B is depressed, the 7-segment decrement from the point where switch A
;   - stops.
;   - When both switches are depressed, the 7-segment will reset to 0
;
; Inputs:
;   Keypad connected to PORTB
;   - Rows (R1-R4): RB3, RB4, RB6, RB7 (input pins with 11KΩ pull-down resistors)
;   - Columns (C1-C3): RB0, RB1, RB2   (output pins)
;   Switch A - PORTA,0 - Normally set High for no contact (when pressed goes Low)
;   Switch B - PORTA,1 - Normally set High for no contact (when pressed goes Low)
;
; Outputs:
;   PORTD [6:0] - 7-segment display (common cathode)
; Date: March 21, 2025
; File Dependencies / Libraries: 
;   The program is required to include the MyConfig.inc in the Header Folder. 
;   It is also required any FunctionCall.inc folder created to simply (shorten) counter.asm 
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
;   V2.0: 03/18/2025 - Add keypad definitions and constants and replace switch operations with keypad
;   V2.1: 03/19/2025 - Use existing time dealy loop for key debouncing and signal statiblization
;   V2.3: 03/20/2025 - Create shorter time delay loop for waiting for signal stabilization
;   V2.4: 03/20/2025 - Rework keypad scanning logic operations according to actual PORTB's hardware configuration
;   V2.5: 03/21/2025 - Expand keypad scanning operation to include all keys
;   V3.0: 03/21/2025 - Combine switch and keypad operation to create one single program

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
KEY_STAR    equ	    0x2A    ; '*' key - increment 
KEY_HASH    equ     0x23    ; '#' key - decrement 
KEY_ZERO    equ     0x00    ; '0' key - reset 
; Switch definitions
#define SW_A    PORTA,0     ; Switch A connected to RA0 
#define SW_B    PORTA,1     ; Switch B connected to RA1 
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
    RCALL   _setupPortA       ; Setup switch port
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
    
    ; Check if both switches are pressed first
    BTFSC   SW_A                   ; Skip next if SW_A is LOW (pressed)
    GOTO    _check_switchB         ; SW_A not pressed, check SW_B
    BTFSC   SW_B                   ; Skip next if SW_B is LOW (pressed)
    GOTO    _check_switchA_only   ; SW_B not pressed, only SW_A pressed
    ; If we reach here, both switches are pressed
    CLRF    COUNT                  ; Reset counter to 0
    RCALL   _display               ; Display 0
    RCALL   _loopDelay             ; Delay
    GOTO    _main                  ; Return to main loop
    
_check_switchA_only:
    ; Only Switch A pressed - Increment
    RCALL   _shortDelay         ; Add debounce delay
    BTFSC   SW_A                ; Recheck switch after delay
    GOTO    _main               ; Switch released, go back
    RCALL   _increment
    GOTO    _main
    
_check_switchB:
    ; Check if only switch B is pressed
    BTFSC   SW_B                ; Skip next instruction if SW_B is LOW (pressed)
    GOTO    _check_keypad       ; No switches pressed, check keypad
    RCALL   _shortDelay         ; Add debounce delay
    BTFSC   SW_B                ; Recheck switch after delay
    GOTO    _main               ; Switch released, go back
    RCALL   _decrement          ; Only SW_B pressed, decrement
    GOTO    _main               ; Return to main loop
    
_check_keypad:
    ; Check for keypad input
    MOVF    KEY, W
    XORLW   KEY_NONE
    BZ      _main                 ; If no key, go back to main loop
    GOTO    _process_key
      
_process_key:
    ; Check if this is a new key press by comparing to LASTKEY
    MOVF    KEY, W
    CPFSEQ  LASTKEY          ; Skip if KEY = LASTKEY (same key still pressed)
    GOTO    _new_key_press   ; Process as new key press
    
    ; Same key still pressed - handle auto-repeat for * and #
    MOVF    KEY, W
    XORLW   KEY_STAR
    BZ      _increment       ; If '*' still pressed, increment
    
    MOVF    KEY, W
    XORLW   KEY_HASH
    BZ      _decrement       ; If '#' still pressed, decrement
    
    ; For other keys, no auto-repeat
    GOTO    _main
    
_new_key_press:
    ; Save the current key as LASTKEY
    MOVF    KEY, W
    MOVWF   LASTKEY
    
    ; Check for special keys first
    MOVF    KEY, W
    XORLW   KEY_STAR        ; Check if '*' key (0x2A)
    BZ      _increment
    
    MOVF    KEY, W
    XORLW   KEY_HASH        ; Check if '#' key (0x23)
    BZ      _decrement
    
    MOVF    KEY, W
    XORLW   KEY_ZERO        ; Check if '0' key (0x00)
    BZ      _reset
    
    ; Direct check for digits 1-9
    MOVF    KEY, W
    SUBLW   0x09            ; Compare W with 9
    BN      _main           ; If KEY > 9, not a valid digit
    
    MOVF    KEY, W
    BZ      _main           ; If KEY = 0, already handled as KEY_ZERO
    
    ; KEY [1 : 9]
    MOVWF   COUNT           ; Set COUNT to the key value (1-9)
    RCALL   _display        ; Display the digit
    RCALL   _loopDelay	    ; Use _loopdelay for debouncing
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
    CLRF    COUNT           ; Set upper boundary: Reset to 0 if COUNT ? 16
    RCALL   _display        ; Display updated count
    RCALL   _loopDelay      ; Delay for auto-repeat rate
    RCALL   _shortDelay         ; Add debounce delay
    GOTO    _main
    
_decrement:
    MOVF    COUNT, W        ; Move COUNT to WREG
    BZ      _lower_bound    ; If COUNT is 0, set lower boundery
    DECF    COUNT, F        ; Otherwise decrement count
    RCALL   _display        ; Display updated count
    RCALL   _loopDelay      ; Delay for auto-repeat rate
    RCALL   _shortDelay         ; Add debounce delay
    GOTO    _main
    
_lower_bound:
    MOVLW 	0x0F            ; Load 15 (F) into WREG
    MOVWF 	COUNT           ; Set COUNT to F and make 0 the lower boundery
    RCALL 	_display        ; Display updated count
    RCALL 	_loopDelay      ; Delay for auto-repeat rate
    GOTO 	_main
    

_resetColumns:
    BANKSEL LATB
    ; Set all columns HIGH (active)
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
    ; Scan Column 1
    ;-------------------------------------------
    BANKSEL LATB
    ; Set Column 1 active (HIGH), others inactive (LOW)
    BSF     LATB, COL1      ; Set C1 HIGH (active)
    BCF     LATB, COL2      ; Set C2 LOW (inactive)
    BCF     LATB, COL3      ; Set C3 LOW (inactive)
    RCALL   _shortDelay     ; Small delay for signal stabilization
	
	BANKSEL PORTB
_col1_row1:
    ; Check Row 1
    BTFSS   PORTB, ROW1
    GOTO    _col1_row2
    MOVLW   0x01            ; Key '1' pressed
    MOVWF   KEY
    RETURN
    
_col1_row2:
    ; Check Row 2
    BTFSS   PORTB, ROW2
    GOTO    _col1_row3
    MOVLW   0x04            ; Key '4' pressed
    MOVWF   KEY
    RETURN
    
_col1_row3:
    ; Check Row 3
    BTFSS   PORTB, ROW3
    GOTO    _col1_row4
    MOVLW   0x07            ; Key '7' pressed
    MOVWF   KEY
    RETURN
    
_col1_row4:
    ; Check Row 4
    BTFSS   PORTB, ROW4
    GOTO    _scan_col2
    MOVLW   0x2A            ; Key '*' pressed
    MOVWF   KEY
    RETURN
	
    
_scan_col2:
    ;-------------------------------------------
    ; Scan Column 2
    ;-------------------------------------------
    BANKSEL LATB
    ; Set Column 2 active (HIGH), others inactive (LOW)
    BCF     LATB, COL1      ; Set C1 LOW (inactive)
    BSF     LATB, COL2      ; Set C2 HIGH (active)
    BCF     LATB, COL3      ; Set C3 LOW (inactive)
    RCALL   _shortDelay     ; Small delay for signal stabilization
    
    BANKSEL PORTB
_col2_row1:
    ; Check Row 1
    BTFSS   PORTB, ROW1
    GOTO    _col2_row2
    MOVLW   0x02            ; Key '2' pressed
    MOVWF   KEY
    RETURN
    
_col2_row2:
    ; Check Row 2
    BTFSS   PORTB, ROW2
    GOTO    _col2_row3
    MOVLW   0x05            ; Key '5' pressed
    MOVWF   KEY
    RETURN
    
_col2_row3:
    ; Check Row 3
    BTFSS   PORTB, ROW3
    GOTO    _col2_row4
    MOVLW   0x08            ; Key '8' pressed
    MOVWF   KEY
    RETURN
    
_col2_row4:
    ; Check Row 4
    BTFSS   PORTB, ROW4
    GOTO    _scan_col3
    MOVLW   0x00            ; Key '0' pressed
    MOVWF   KEY
    RETURN
    
_scan_col3:
    ;-------------------------------------------
    ; Scan Column 3
    ;-------------------------------------------
    BANKSEL LATB
    ; Set Column 3 active (HIGH), others inactive (LOW)
    BCF     LATB, COL1      ; Set C1 LOW (inactive)
    BCF     LATB, COL2      ; Set C2 LOW (inactive)
    BSF     LATB, COL3      ; Set C3 HIGH (active)
    RCALL   _shortDelay     ; Small delay for signal stabilization
    
    BANKSEL PORTB
_col3_row1:
    ; Check Row 1
    BTFSS   PORTB, ROW1
    GOTO    _col3_row2
    MOVLW   0x03            ; Key '3' pressed
    MOVWF   KEY
    RETURN
    
_col3_row2:
    ; Check Row 2
    BTFSS   PORTB, ROW2
    GOTO    _col3_row3
    MOVLW   0x06            ; Key '6' pressed
    MOVWF   KEY
    RETURN
    
_col3_row3:
    ; Check Row 3
    BTFSS   PORTB, ROW3
    GOTO    _col3_row4
    MOVLW   0x09            ; Key '9' pressed
    MOVWF   KEY
    RETURN
    
_col3_row4:
    ; Check Row 4
    BTFSS   PORTB, ROW4
    GOTO    _no_key
    MOVLW   0x23            ; Key '#' pressed
    MOVWF   KEY
    RETURN
    
_no_key:
    ; No key was pressed - reset all columns to active
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
    TBLRD* ; Read table data, no pointer increment
    MOVF    TABLAT, W            ; Get the pattern from TABLAT into WREG
    BANKSEL LATD
    MOVWF   LATD                 ; Use LATD to display the pattern on PORTD
    RETURN
	
;----------------------------------------------------------------
;----------  The Delay Subroutines ------------------------------
;----------------------------------------------------------------
_loopDelay:
    MOVLW     Inner_loop
    MOVWF     REG10
    MOVLW     Outer_loop
    MOVWF     REG11
    MOVLW     High_loop
    MOVWF     REG30
_loop1:
    DECF      REG10, F
    BNZ       _loop1
    MOVLW     Inner_loop 	; Re-initialize the inner loop for when the outer loop decrements.
    MOVWF     REG10
    DECF      REG11, F 		; outer loop
    BNZ       _loop1
    MOVLW     Outer_loop 	; Re-initialize the outer loop for when the high loop decrements.
    MOVWF     REG11				
    DECF      REG30, F		; high loop
    BNZ       _loop1
    RETURN

; Shorter delay for debouncing (~50µs)
_shortDelay:
    MOVLW       50             
    MOVWF       TEMP
_shortLoop:
    DECF        TEMP, F
    BNZ         _shortLoop
    RETURN
	
;----------------------------------------------------------------
;----------  Initializing the I/O Ports--------------------------
;----------------------------------------------------------------
_setupPortD:
    BANKSEL		PORTD
    CLRF		PORTD 			; Init PORTD
    BANKSEL		LATD 			; Data Latch
    CLRF		LATD 	        ; Clear LATD to ensure all segments are off initially
    BANKSEL		ANSELD
    CLRF		ANSELD 			; digital I/O
    BANKSEL		TRISD
    MOVLW		0b00000000 		; Set all PORTD pins as outputs for 7-segment display
    MOVWF		TRISD
    RETURN

_setupPortB:
    BANKSEL		PORTB
    CLRF		PORTB 			; Init PORTB
    BANKSEL		LATB 			; Data Latch
    CLRF		LATB
    
    ; Set initial state of column pins to HIGH (inactive)
    BSF     	LATB, COL1
    BSF     	LATB, COL2
    BSF     	LATB, COL3
    
    BANKSEL		ANSELB
    CLRF		ANSELB 			; digital I/O for all pins
    
    BANKSEL	TRISB
    ; Columns as outputs (RB0-RB2), rows as inputs (RB3-RB4, RB6-RB7)
    MOVLW		0b11111000 		; RB0-RB2 outputs, RB3-RB7 inputs
    MOVWF		TRISB
    
    ; Using external pull-down resistors on PORTB,3,4,6,7
    BANKSEL 	WPUB
    CLRF    	WPUB            ; Disable all internal pull-ups
    RETURN
    
_setupPortA:
    BANKSEL		PORTA
    CLRF		PORTA 			; Init PORTA
    BANKSEL		LATA 			; Data Latch
    CLRF		LATA
    BANKSEL		ANSELA
    CLRF		ANSELA 			; digital I/O
    BANKSEL		TRISB ;
    MOVLW		0b00000011 		; Set RA0 and RA1 as inputs
    MOVWF		TRISA 			; RA0 and RA1 are inputs, RA[7:2] are outputs
    BANKSEL 	WPUA            ; Weak pull-up register for PORTA
    MOVLW   	0b00000011      ; Enable weak pull-ups for RA0 and RA1
    MOVWF   	WPUA            ; Keep switches set HIGH when not pressed
    RETURN
    
    END
