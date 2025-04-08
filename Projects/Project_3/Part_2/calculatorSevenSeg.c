/* --------------------------------------------------------
 * Title: Binary Display Calculator with 7-Segment Display
 * --------------------------------------------------------
 * Purpose: 
 *    This program implements a calculator using a 4x4 keypad and dual common cathode 7-segment  
 *    display on a PIC18F47K42 microcontroller. It performs four basic arithmetic operations:
 *    addition, subtraction, multiplication, and division, and displays results in decimal.
 * OUTPUTS: 
 *    - PORTD: Controls 7-segment display segments (a-g, dp)
 *    - PORTA [0 : 1] Controls digit selection for 7-segment display
 *    - PORTB [0 : 3] Keypad column control C1-C4
 * INPUTS: 
 *    - PORTB [4 : 7] Keypad row control R1-R4 with external pull down resistors
 * File Dependencies / Libraries:
 *    - Configuration Header File and all needed libraries' functions.
 * Compiler: MPLAB X IDE v6.20; XC8, V3.0
 * Author: Huy Nguyen 
 * Versions:
 *    V2.0: 4/1/25 - Modified to use dual 7-segment display 
 *    V2.1: 4/1/25 - Renamed some varaiable for ease of visual identification
 *    V2.2: 4/3/25 - Add driver circuits to efficiently toggle between RA0 and RA1
 *    V2.3: 4/4/25 - Reworked scanKeypad() to work with 7-segment
 *    V2.4: 4/6/25 - Reworked display functions and negative number display
 *    V2.5: 4/7/25 - Corrected num1 and negative number display
 */
 
#include <xc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "C:/Program Files/Microchip/xc8/v3.00/pic/include/proc/pic18f47k42.h"

// Configuration bits - critical for proper operation
#pragma config WDTE = OFF     // Watchdog Timer disabled
#pragma config DEBUG = OFF    // Background debugger disabled

#define _XTAL_FREQ 4000000    // Crystal frequency
#define FCY _XTAL_FREQ/4

// Input range as specified in requirements
#define MAX_INPUT 0x63        // Maximum input is 99 in decimal
#define MIN_INPUT 0x00        // Minimum input is 0 in decimal

// 7-segment display digit select pins
#define TENS_DIGIT_PIN    0   // RA0 - Controls tens digit (leftmost)
#define UNITS_DIGIT_PIN   1   // RA1 - Controls units digit (rightmost)

// 7-segment display segment definitions
#define SEG_A   (1 << 6)    // RD6
#define SEG_B   (1 << 5)    // RD5
#define SEG_C   (1 << 4)    // RD4
#define SEG_D   (1 << 3)    // RD3
#define SEG_E   (1 << 2)    // RD2
#define SEG_F   (1 << 1)    // RD1
#define SEG_G   (1 << 0)    // RD0
#define SEG_DP  (1 << 7)    // RD7 (decimal point)

// Display mode definitions
#define DISPLAY_RESET     0
#define DISPLAY_NUM1      1
#define DISPLAY_OPERATOR  2
#define DISPLAY_NUM2      3
#define DISPLAY_RESULT    4

// Function prototypes
void initialize(void);                       // Initialize all IO ports and hardware
unsigned char scanKeypad(void);              // Enhanced keypad scanning
int getNum1(void);                           // Get the first number for operation
int getNum2(void);                           // Get the second number for operation
int getOperator(void);                       // Determine the arithmetic operation
int doOperation(int num1, int num2, int operator);  // Performs arithmetic operation
void displayNumber(int number);              // Display number on 7-segment display
void displayDigit(int digit, int position, bool dp); // Display single digit
void refreshDisplay(int number);             // Continuously refresh display to maintain visibility
void blinkDisplay(int count, int delay_ms);  // Blink 7-segment display
void displayResult(void);                    // Display calculation result
void resetCalculator(void);                  // Reset calculator state
void updateDisplay(int value, int mode);     // Update display based on current mode
unsigned char encodeDigit(int digit);        // Encode digit to 7-segment pattern

// Global variables to store input and calculation results
int num1_tens, num1_units;            // First number's digits
int num2_tens, num2_units;            // Second number's digits
int num1;                    // First number
int num2;                    // Second number
int operator;                // Selected arithmetic operation
unsigned char keyVal;        // Value of pressed key
int result;                  // Calculation results
int displayMode;             // Current display mode
bool waitingForHashKey = false;  // Flag for waiting for # key
bool validInput = false;     // Flag for valid input
int currentDisplayValue = 0; // Currently displayed value
bool isDisplayNegative = false; // Whether current display is negative

// 7-segment display patterns for digits 0-9, errors, and blank
// Each pattern represents segments a,b,c,d,e,f,g which maps to PORTD
const unsigned char digitPatterns[] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         // 0
    SEG_B | SEG_C,                                         // 1
    SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                 // 2
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,                 // 3
    SEG_F | SEG_G | SEG_B | SEG_C,                         // 4
    SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                 // 5
    SEG_A | SEG_F | SEG_G | SEG_C | SEG_D | SEG_E,         // 6
    SEG_A | SEG_B | SEG_C,                                 // 7
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, // 8
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,         // 9
    SEG_A | SEG_D | SEG_G,                                 // Error pattern (E)
    0                                                      // Blank
};

// Special display patterns for operators
const unsigned char operatorPatterns[] = {
    SEG_F | SEG_E | SEG_A | SEG_B | SEG_C,                 // A (Addition)
    SEG_F | SEG_E | SEG_G | SEG_C | SEG_D,                 // S (Subtraction alt)
    SEG_A | SEG_F | SEG_E | SEG_D,                         // C (Multiplication)
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G                  // D (Division)
};

// Special display patterns
#define PATTERN_E (SEG_A | SEG_F | SEG_G | SEG_E | SEG_D)  // 'E' for error
#define PATTERN_MINUS (SEG_G)                              // '-' for negative indicator
#define PATTERN_BLANK (0)                                  // Blank display


void initialize(void) {
    // Disable all analog functionality
    ANSELA = 0x00; ANSELB = 0x00;  ANSELC = 0x00; ANSELD = 0x00; ANSELE = 0x00;
       
    TRISA = 0xFC; // Configure PORTA: RA0-RA1 outputs (digit select)
    TRISB = 0xF0; // Configure PORTB: RB0-RB3 outputs (columns), RB4-RB7 inputs (rows)    
    TRISD = 0x00; // Configure PORTD as all outputs (for 7-segment display segments)

    // Initialize all ports to known state
    LATA = 0x00; LATB = 0x00; LATC = 0x00; LATD = 0x00; LATE = 0x00;
                  
    WPUB = 0x00;  // Disable weak pull-ups on PORTB

    // Set initial display mode
    displayMode = DISPLAY_RESET;
    currentDisplayValue = 0;
    isDisplayNegative = false;
    
    blinkDisplay(3, 500); // Startup sequence - blink all segments 3 times
       
    displayNumber(0);  // Display "00" to indicate ready for input
}


unsigned char encodeDigit(int digit) { // Encode digit to 7-segment pattern    
    if (digit >= 0xA && digit <= 0xD) {// Handle special displays     
        return operatorPatterns[digit - 0xA];  // Operator display
    } else if (digit >= 0 && digit <= 9) {        
        return digitPatterns[digit]; // Regular digits
    } else if (digit == -1) {        
        return PATTERN_MINUS; // For negative or minus symbol
    } else {        
        return PATTERN_BLANK; // Default to blank for invalid digits
    }
}


void displayDigit(int digit, int position, bool dp) { // Display a single digit on one of the 7-segment
    
    unsigned char pattern = encodeDigit(digit);  // Get the segment pattern for this digit        
    if (dp) { // Add decimal point if needed
        pattern |= SEG_DP;
    }       
    LATA = 0x00; // Turn off all displays 
    
    LATD = pattern;  // Output the segment pattern before selecting the digit to prevent glitches when multiplexing
    
    // Then select the digit
    if (position == 0) { // Tens digit (RA0 = 1, RA1 = 0)       
        LATA = (1 << TENS_DIGIT_PIN);
    } else { // Units digit (RA0 = 0, RA1 = 1)        
        LATA = (1 << UNITS_DIGIT_PIN);
    }
}


void refreshDisplay(int number) { // Refresh display to ensure visibility    
    currentDisplayValue = number;  // Save current display value for future refreshes
    isDisplayNegative = (number < 0);
       
    for (int i = 0; i < 30; i++) { // More refreshes for clearer digit display
        displayNumber(number);
    }
}


void displayNumber(int number) { // Display a number on the 7-segment display using multiplexing
    int tens, units;
    bool isNegative = false;
        
    if (number < 0) { // Check if negative
        isNegative = true;
        number = -number; // Convert to positive for display
                
        if (number > 99) { // Limit to -99
            number = 99;
        }
    } 
	else {        
        if (number > 99) { // Limit to 99
            number = 99;
        }
    }
    
    // Set digits to correct order
    tens = number / 10;
    units = number % 10;
       
    displayDigit(tens, 0, false);  // Display tens digit (no decimal point)
    __delay_ms(5);
        
    displayDigit(units, 1, isNegative);  // Display units digit with decimal point if negative
    __delay_ms(5);
}


void blinkDisplay(int count, int delay_ms) { // Blink all segments of the 7-segment display
    for (int i = 0; i < count; i++) {
        // Turn all segments on for both digits
        LATA = (1 << TENS_DIGIT_PIN);
        LATD = 0xFF; // All segments on
        __delay_ms(10);
        LATA = (1 << UNITS_DIGIT_PIN);
        LATD = 0xFF; // All segments on
        __delay_ms(10);
        
        // Turn all segments off for both digits
        LATA = 0x00;
        LATD = 0x00; // All segments off
        __delay_ms(20);
    }
}


void updateDisplay(int value, int mode) { // Update display based on current mode and value
    
    static int previousMode = DISPLAY_RESET;  // Save the previous mode to properly transition 
       
    displayMode = mode;  // Update display mode
    
    // Special handling for transition from NUM1 to OPERATOR
    if (previousMode == DISPLAY_NUM1 && mode == DISPLAY_OPERATOR) {       
        int rememberedNum1 = currentDisplayValue;  // Remember the num1 value 
        
        // Show the operator briefly
        for (int i = 0; i < 10; i++) {            
            displayDigit(value, 0, false); // Left digit shows operator
            __delay_ms(5);
            
            displayDigit(11, 1, false); // Right digit is blank
            __delay_ms(5);
                       
            displayNumber(rememberedNum1); // Alternate with showing num1
            __delay_ms(5);
        }               
        currentDisplayValue = rememberedNum1;  // Restore current display value to num1
    }
    else {
        switch (mode) {
            case DISPLAY_NUM1:
            case DISPLAY_NUM2:
            case DISPLAY_RESULT:
                refreshDisplay(value);
                break;               
            case DISPLAY_OPERATOR:  // Display operator (A=+, B=-, C=*, D=/)               
                for (int i = 0; i < 10; i++) {                    
                    displayDigit(value, 0, false); // Left digit shows operator
                    __delay_ms(5);                    
                    displayDigit(11, 1, false); // Right digit is blank
                    __delay_ms(5);
                }
                break;                
            case DISPLAY_RESET:
            default:               
                refreshDisplay(0); // Show "00" as default/reset display
                break;
        }
    }
        
    previousMode = mode; // Remember the current mode for next iteration
}


unsigned char scanKeypad() { // keypad scanning function
    unsigned char col, row;
    unsigned char key = 0xFF;  // Default to no key pressed (0xFF)
    
    // Always display the current value to maintain visibility
    if (displayMode == DISPLAY_NUM1 || displayMode == DISPLAY_NUM2 || 
        displayMode == DISPLAY_RESULT || displayMode == DISPLAY_RESET) {        
        displayNumber(currentDisplayValue); // Use currentDisplayValue for consistent display
    } else if (displayMode == DISPLAY_OPERATOR) { // For operator mode, alternate between showing operator and num1        
        static int displayToggle = 0;       
        if (displayToggle++ % 3 != 0) {           
            displayDigit(operator, 0, false); // Display the operator
            __delay_ms(2);
            displayDigit(11, 1, false); // Blank
            __delay_ms(2);
        } else {            
            displayNumber(currentDisplayValue); // Periodically show num1  to prevent it from disappearing
            __delay_ms(2);
        }
    }
       
    LATB &= 0xF0;   // Set all columns low 
        
    for (col = 0; col < 4; col++) { // Scan each column       
        LATB |= (1 << col); // Set current column high              
        __delay_ms(1); // Wait for signal to stabilize
               
        for (row = 0; row < 4; row++) { // Read each row for this column
            if (PORTB & (1 << (row + 4))) {
                // Key pressed: calculate the correct key value based on the physical layout                               
                if (row == 0) {
                    switch (col) {
                        case 0: key = 1; break;    // 1
                        case 1: key = 2; break;    // 2
                        case 2: key = 3; break;    // 3
                        case 3: key = 0xA; break;  // A
                    }
                }
                else if (row == 1) {
                    switch (col) {
                        case 0: key = 4; break;    // 4
                        case 1: key = 5; break;    // 5
                        case 2: key = 6; break;    // 6
                        case 3: key = 0xB; break;  // B
                    }
                }
                else if (row == 2) {
                    switch (col) {
                        case 0: key = 7; break;    // 7
                        case 1: key = 8; break;    // 8
                        case 2: key = 9; break;    // 9
                        case 3: key = 0xC; break;  // C
                    }
                }
                else if (row == 3) {
                    switch (col) {
                        case 0: key = 0xE; break;  // * (0xE)
                        case 1: key = 0; break;    // 0
                        case 2: key = 0xF; break;  // # (0xF)
                        case 3: key = 0xD; break;  // D
                    }
                }
                               
                __delay_ms(10);  // Debounce
                
                // Wait for key release with timeout
                unsigned char timeout = 100;
                while ((PORTB & (1 << (row + 4))) && (timeout > 0)) {
                    // Display refresh while waiting for key release
                    if (displayMode == DISPLAY_NUM1 || displayMode == DISPLAY_NUM2 || 
                        displayMode == DISPLAY_RESULT || displayMode == DISPLAY_RESET) {
                        displayNumber(currentDisplayValue);
                    } else if (displayMode == DISPLAY_OPERATOR) {
						// Alternate between showing operator and num1
                        static int innerDisplayToggle = 0;                          
                        if (innerDisplayToggle++ % 3 != 0) {                            
                            displayDigit(operator, 0, false); // Display the operator
                            __delay_ms(2);
                            displayDigit(11, 1, false); // Blank
                            __delay_ms(2);
                        } else {                             
                            displayNumber(currentDisplayValue); // Periodically show num1
                            __delay_ms(2);
                        }
                    }                    
                    __delay_ms(1);
                    timeout--;
                }
                               
                __delay_ms(20); // Final debounce delay
                               
                LATB &= ~(1 << col); // Set column back to low
                                
                return key; // Return the key value
            }
        }
                
        LATB &= ~(1 << col); // Set column back to low before trying next column
    }    
    return 0xFF;  // No key pressed
}


int getNum1() { // Get first number from user input
    num1_tens = 0;
    num1_units = 0;
    validInput = false;
    int currentDisplay = 0;
    
    updateDisplay(currentDisplay, DISPLAY_NUM1); // Update display for first number input
    
    while (1) {
        keyVal = scanKeypad();        
        if (keyVal == 0xFF) { // Continue scanning if no key pressed
            continue;
        }
        
        if (keyVal == 0xE) { // Check for reset (* key)
            resetCalculator();
            return 0xFF; // Return special value to indicate reset
        }
        
        if (keyVal <= 9) { // Get first digit (0-9)
            num1_tens = keyVal;
            currentDisplay = num1_tens;
            validInput = true; // Valid input
            updateDisplay(currentDisplay, DISPLAY_NUM1);
            
            // Wait for second digit or operator
            while (1) {
                keyVal = scanKeypad();                              
                if (keyVal == 0xFF) { // Continue if no key pressed
                    continue;
                }                
                if (keyVal <= 9) { // Check if second digit (0-9)
                    num1_units = keyVal;
                    num1 = (num1_tens * 10) + num1_units;
                    
                    if (num1 > MAX_INPUT) { // Limit to valid range
                        num1 = MAX_INPUT;
                    }
                   // Update the display with the two-digit number
                    currentDisplay = num1; 
                    updateDisplay(num1, DISPLAY_NUM1);
                    return num1;
                }                
                else if (keyVal >= 0xA && keyVal <= 0xD) { // Check if operator (A-D)                   
                    num1 = num1_tens; // Single digit input followed by operator
                    currentDisplay = num1; // Update currentDisplay
                    operator = keyVal;
                    return num1;
                }
                // Check for hash key (#) - allows single-digit input
                else if (keyVal == 0xF) {
                    num1 = num1_tens;
                    currentDisplay = num1; // FIXED: Update currentDisplay
                    return num1;
                }
                // Check for reset (* key)
                else if (keyVal == 0xE) {
                    resetCalculator();
                    return 0xFF; // Return special value for reset
                }
            }
        }
    }
}


int getOperator() { // Get operator input (A, B, C, or D)
    
    int storedNum1 = num1; // Store num1 
        
    if (operator >= 0xA && operator <= 0xD) { // If operator was already set by getNum1
        int temp = operator;
                
        for (int i = 0; i < 10; i++) { // Display the operator without losing the num1 value
            
            displayDigit(temp, 0, false); // Show operator on left digit
            __delay_ms(5);           
            displayDigit(11, 1, false); // Show blank on right digit
            __delay_ms(5);                        
            displayNumber(storedNum1); // Alternate with showing num1 to maintain its visibility
            __delay_ms(5);
        }               
        currentDisplayValue = storedNum1; // Keep displaying num1
        
        return temp;
    }
    
    // Wait for operator key while maintaining the num1 display
    while (1) {       
        displayNumber(storedNum1); // Keep display num1 
        __delay_ms(5);        
        keyVal = scanKeypad();
                
        if (keyVal == 0xFF) { // Continue scanning if no key pressed           
            displayNumber(storedNum1); // Keep displaying num1
            continue;
        }               
        if (keyVal >= 0xA && keyVal <= 0xD) { // Check if valid operator (A-D)
            // Display the operator briefly
            for (int i = 0; i < 10; i++) {                
                displayDigit(keyVal, 0, false); // Show operator on left digit
                __delay_ms(5);               
                displayDigit(11, 1, false); // Show blank on right digit
                __delay_ms(5);                              
                displayNumber(storedNum1); // Alternate with showing num1 to maintain its visibility
                __delay_ms(5);
            }
                        
            currentDisplayValue = storedNum1; // Keep displaying num1
            
            return keyVal;
        }        
        else if (keyVal == 0xE) { // Check for reset (* key)
            resetCalculator();
            return 0; // Return 0 to indicate reset
        }
    }
}


int getNum2() { // Get second number from user input
    num2_tens = 0;
    num2_units = 0;
    validInput = false;
    int currentDisplay = 0;
    
    updateDisplay(currentDisplay, DISPLAY_NUM2); // Update display for second number input
    
    while (1) {
        keyVal = scanKeypad();        
        if (keyVal == 0xFF) { // Continue scanning if no key pressed
            continue;
        }        
        if (keyVal == 0xE) { // Check for reset (* key)
            resetCalculator();
            return 0; // Return 0 to indicate reset
        }       
        if (keyVal <= 9) { // Get first digit (0-9)
            num2_tens = keyVal;
            currentDisplay = num2_tens;
            validInput = true; // Mark as valid input
            updateDisplay(currentDisplay, DISPLAY_NUM2);
            
            // Wait for second digit or hash key
            while (1) {
                keyVal = scanKeypad();                               
                if (keyVal == 0xFF) { // Continue if no key pressed
                    continue;
                }                
                if (keyVal <= 9) { // Check if second digit (0-9)
                    num2_units = keyVal;
                    num2 = (num2_tens * 10) + num2_units;
                    
                    if (num2 > MAX_INPUT) { // Limit to valid range
                        num2 = MAX_INPUT;
                    }
                    updateDisplay(num2, DISPLAY_NUM2);
                    waitingForHashKey = true;
                    return num2;
                }
                
                else if (keyVal == 0xF) { // Check if hash key (#) - num2 is 1-digit number
                    num2 = num2_tens;
                    waitingForHashKey = true;
                    return num2;
                }               
                else if (keyVal == 0xE) { // Check for reset (* key)
                    resetCalculator();
                    return 0; // Return 0 to indicate reset
                }
            }
        }
    }
}


int doOperation(int num1, int num2, int operator) { // Perform the calculation based on the selected operation
    int result = 0;
    
    // Perform calculation based on operator
    switch (operator) {
        case 0xA: // 'A' - Addition
            result = num1 + num2;
            break;
        case 0xB: // 'B' - Subtraction
            result = num1 - num2;
            break;
        case 0xC: // 'C' - Multiplication
            result = num1 * num2;
            // Check for overflow
            if (result > 99) {
                result = 99;
            } else if (result < -99) {
                result = -99;
            }
            break;
        case 0xD: // 'D' - Division
            if (num2 != 0) {
                result = num1 / num2;
            } else {
                // Division by zero - display error
                for (int i = 0; i < 5; i++) {                    
                    LATA = (1 << TENS_DIGIT_PIN);
                    LATD = PATTERN_E;  // Display "E" for error
                    __delay_ms(100);
                    
                    LATA = (1 << UNITS_DIGIT_PIN);
                    LATD = digitPatterns[0];  // "0" pattern
                    __delay_ms(100);
                    
                    // Turn off display briefly
                    LATA = 0x00;
                    LATD = 0x00;
                    __delay_ms(100);
                }
                return 0; // Return 0 after division by zero error
            }
            break;
        default:
            return 0;
    }
    
    // Limit result to displayable range (-99 to 99)
    if (result > 99) {
        result = 99;
    } else if (result < -99) {
        result = -99;
    }
    
    return result;
}


void displayResult() { // Display the calculation result
    // Indicate waiting for # key if needed
    if (!waitingForHashKey) {
        // Blink display to to show waiting for '#' key
        for (int i = 0; i < 3; i++) {
            LATA = 0x00;  // Turn off digit select
            LATD = 0x00;  // Turn off all segments
            __delay_ms(200);
                        
            refreshDisplay(num2); // Show current inputs, num2
            __delay_ms(200);
        }
    }

    // Wait for # key to complete calculation
    while (1) {
        keyVal = scanKeypad();
                
        if (keyVal == 0xFF) { // Continue if no key pressed
            continue;
        }
        if (keyVal == 0xF) { // '#' key pressed           
            result = doOperation(num1, num2, operator); // Calculate result                      
            updateDisplay(result, DISPLAY_RESULT); // Display the result
            
            // Enter result display loop
            while(1) {
                keyVal = scanKeypad();
                if (keyVal == 0xE) {
                    resetCalculator();
                    return;
                }
            }
        }
        else if (keyVal == 0xE) { // '*' key for reset
            resetCalculator();
            return;
        }
    }
}


void resetCalculator() { // Reset the calculator state
    // Reset all variables
    num1 = 0;
    num2 = 0;
    operator = 0;
    waitingForHashKey = false;
    validInput = false;
    displayMode = DISPLAY_RESET;
    currentDisplayValue = 0;
    isDisplayNegative = false;
        
    updateDisplay(0, DISPLAY_RESET); // Clear display and show "00"
}


void main(void) {
    
    initialize(); // Initialize hardware
    
    // Main program loop
    while (1) {        
        resetCalculator(); // Reset calculator state
                
        num1 = getNum1(); // Get first number
        if (num1 == 0xFF) continue; // Reset occurred
                
        currentDisplayValue = num1; // Keep displaying num1
               
        operator = getOperator(); // Get operator
        if (operator == 0) continue; // Reset occurred
                
        int savedNum1 = currentDisplayValue; // Check if num1 is still maintained
                
        num2 = getNum2(); // Get second number
        if (num2 == 0xFF) continue; // Reset occurred
               
        num1 = savedNum1; // Restore num1 to correct value for calculation
                
        displayResult(); // Display result
                
        __delay_ms(500); // Short delay before next calculation
    }
}
