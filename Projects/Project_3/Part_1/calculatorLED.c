/* ---------------------
 * Title: Basic Calculator
 * ---------------------
 * Purpose: 
 *    This program implements a calculator using a 4x4 keypad and 8-bit LED array on 
 *    a PIC18F47K42 microcontroller. It performs four basic arithmetic operations:
 *    addition, subtraction, multiplication, and division.  Its functions are limited to handing
 *	  integers ranging from -99 to 99.  
 *	  The calculations result is displayed in binary format on the 8-bit LED array to represent the calculation result.
 *	  Negative result is indicated by blinking the MSB of the array , 8th LED from left.
 * OUTPUTS: 
 *    - PORTD: Controls 8-bit LED array
 *    - PORTB [0 : 3] Keypad column control C1-C4
 * INPUTS: 
 *    - PORTB [4 : 7] Keypad row control R1-R4 with external pull down resistors
 * File Dependencies / Libraries:
 *    - Configuration Header File and all needed library's functions.
 * Compiler: MPLAB X IDE v6.20; XC8, V3.0
 * Author: Huy Nguyen
 * Versions:
 *    V1.0: 3/28/25 - Original 
 *    V1.1: 3/29/25 - Revised  initialize() function, add blinkLED(). 
 *    V1.2: 3/30/25 - Reworked scanKeypad() and improved I/O initialization
 *    V1.3: 3/31/25 - Revised LED indicators for initialization and input sequence
 *    V1.4: 3/31/25 - Added support for zero inputs in operations
 *					- Disable all analogue functionality in preparation for integrating 7-segment display
 *	  V1.5: 4/1/25	- Modified to display results in binary format
 * Useful links:  
 *      Datasheet: https://ww1.microchip.com/downloads/en/DeviceDoc/PIC18(L)F26-27-45-46-47-55-56-57K42-Data-Sheet-40001919G.pdf 
 *      PIC18F Instruction Sets: https://onlinelibrary.wiley.com/doi/pdf/10.1002/9781119448457.app4 
 *      List of Instrcutions: http://143.110.227.210/faridfarahmand/sonoma/courses/es310/resources/20140217124422790.pdf 
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

#define MAX_INPUT 0x63        // Maximum input is 99 in decimal
#define MIN_INPUT 0x00        // Minimum input is 0 in decimal (changed from 1)

// Function prototypes
void initialize();                 // Initialize all IO ports and hardware
unsigned char scanKeypad();        // Keypad scanning 
int getNum1();                     // Get the first number for the operation
int getNum2();                     // Get the second number for the operation
int getOperator();                 // Determine the arithmetic operation
int doOperation(int num1, int num2, int operator);  // Performs arithmetic operation
void displayBinary(int number);        // Convert and display number in binary on LEDs
void displayBinaryWithBlink(int number); // Display binary with D8 blinking for negative numbers
void displayResult();              // Display calculation result
void resetCalculator();            // Reset calculator state
void blinkLED(unsigned char pattern, int count, int delay_ms);  // Blink LEDs with pattern

// Global variables to store input and calculation results
int num11, num12;                 // First number's digits
int num21, num22;                 // Second number's digits
int num1;                         // First number
int num2;                         // Second number
int operator;                     // Selected arithmetic operation
unsigned char keyVal;             // Value of pressed key
int result;                       // Calculation results 
bool waitingForHashKey = false;   // Flag to indicate waiting for # key
bool validInput = false;          // Flag to track if a valid input has been entered


void initialize() {
    // Disable all analog functionality 
    ANSELA = 0x00; ANSELB = 0x00;  ANSELC = 0x00; ANSELD = 0x00;  ANSELE = 0x00;   
   
    TRISB = 0xF0; // Configure PORTB: RB0-RB3 outputs (columns), RB4-RB7 inputs (rows)   
    TRISD = 0x00; // Configure PORTD as all outputs (for LEDs)

    // Initialize all ports to known state
    LATA = 0x00; LATB = 0x00;  LATC = 0x00; LATD = 0x00;  LATE = 0x00;
    
    WPUB = 0x00;   // Disable all weak pull-ups on PORTB. Using pull-down resistor  on PORTB [4 : 7]

    // Startup sequence to indicate system is working
    blinkLED(0xFF, 3, 250);  // Blink all LEDs 3 times
    blinkLED(0x01, 3, 250);  // Blink D1 3 times to indicate ready for input
    LATD = 0x00;             // Turn off all LEDs after initialization
}


void blinkLED(unsigned char pattern, int count, int delay_ms) {
    for (int i = 0; i < count; i++) {
        LATD = pattern;
        __delay_ms(500);
        LATD = 0x00;
        __delay_ms(500);
    }
}


unsigned char scanKeypad() {
    unsigned char col, row;
    unsigned char key = 0xFF;  // Default to no key pressed (0xFF)
        
    LATB &= 0xF0;  // Set all columns low 
    
    for (col = 0; col < 4; col++) { // Scan each column        
        LATB |= (1 << col);  // Set current column high
                
        __delay_ms(20);  // Wait for signal to stabilize
               
        for (row = 0; row < 4; row++) { // Read each row for this column
            if (PORTB & (1 << (row + 4))) {                             
                if (row == 0) { // Row 0 (physical row 1)
                    switch (col) {
                        case 0: key = 1; break;    // 1
                        case 1: key = 2; break;    // 2
                        case 2: key = 3; break;    // 3
                        case 3: key = 0xA; break;  // A
                    }
                }               
                else if (row == 1) { // Row 1 (physical row 2)
                    switch (col) {
                        case 0: key = 4; break;    // 4
                        case 1: key = 5; break;    // 5
                        case 2: key = 6; break;    // 6
                        case 3: key = 0xB; break;  // B
                    }
                }                
                else if (row == 2) { // Row 2 (physical row 3)
                    switch (col) {
                        case 0: key = 7; break;    // 7
                        case 1: key = 8; break;    // 8
                        case 2: key = 9; break;    // 9
                        case 3: key = 0xC; break;  // C
                    }
                }                
                else if (row == 3) { // Row 3 (physical row 4)
                    switch (col) {
                        case 0: key = 0xE; break;  // * (0xE)
                        case 1: key = 0; break;    // 0
                        case 2: key = 0xF; break;  // # (0xF)
                        case 3: key = 0xD; break;  // D
                    }
                }                               
                __delay_ms(20);  // Debouncing
                
                // Wait for key release with timeout
                unsigned char timeout = 100;
                while ((PORTB & (1 << (row + 4))) && (timeout > 0)) {
                    __delay_ms(5);
                    timeout--;
                }                               
                __delay_ms(20);  // Final debounce delay                                
                LATB &= ~(1 << col);  // Set column back to low                                
                return key;  // Return the key value
            }
        }                
        LATB &= ~(1 << col);  // Set column back to low before trying next column
    }    
    return 0xFF;  // No key pressed
}


int getNum1() {
    num11 = 0;
    num12 = 0;
    validInput = false;
    
    LATD = 0x00;  // All LEDs off, waiting for first number
    while (1) {
        keyVal = scanKeypad();       
        if (keyVal == 0xFF) { // If no key pressed, continue scanning
            continue;
        }        
        if (keyVal == 0xE) { // Check for reset (* key)
            resetCalculator();
            blinkLED(0x01, 5, 200);
            return 0xFF; // Return special value to indicate reset
        }       
        if (keyVal <= 9) { // Get first digit (0-9)
            num11 = keyVal;
            validInput = true; // valid input detected
            LATD = 0x01;  // D1 on to indicate first number mode
            __delay_ms(500);
            
            while (1) { // Wait for second digit or operator
                keyVal = scanKeypad();                
                if (keyVal == 0xFF) { // If no key pressed, continue
                    continue;
                }                
                if (keyVal <= 9) { // Check if second digit (0-9)
                    num12 = keyVal;
                    num1 = (num11 * 10) + num12;
                   
                    if (num1 > MAX_INPUT) { // Limit to valid range
                        num1 = MAX_INPUT;
                    }
                    LATD = 0x01;   // turn D1 on to indicate first number mode
                    return num1;
                }          
                else if (keyVal >= 0xA && keyVal <= 0xD) { // Check if operator (A-D)
                    num1 = num11;
                    operator = keyVal;
                    return num1;
                }                
                else if (keyVal == 0xF) { // Check for hash key '#', determine if num1 is single digit input
                    num1 = num11;
                    return num1;
                }                
                else if (keyVal == 0xE) { // Check for reset (* key)
                    resetCalculator();
                    blinkLED(0x01, 5, 200);
                    return 0xFF; // Return special value to indicate reset
                }
                __delay_ms(50);
            }
        }
        __delay_ms(50);
    }
}


int getOperator() {   
    if (operator >= 0xA && operator <= 0xD) { // If operator was already set by getNum1
        int temp = operator;
        operator = 0; // Clear it
        return temp;
    }   
    LATD = 0x04; // D3 on waiting for operator key press   
    while (1) {
        keyVal = scanKeypad();               
        if (keyVal == 0xFF) { // If no key pressed, continue
            continue;
        }       
        if (keyVal >= 0xA && keyVal <= 0xD) { // Check if valid operator (A-D)
            LATD = 0x04; // Keep D3 on to indicate operator received
            __delay_ms(500);
            return keyVal;
        }        
        else if (keyVal == 0xE) { // Check for reset (* key)
            resetCalculator();
            blinkLED(0x01, 5, 200);
            return 0; // Return 0 to indicate reset
        }        
        __delay_ms(50);
    }
}


int getNum2() {
    num21 = 0;
    num22 = 0;
    validInput = false;
    
    LATD = 0x02; // D2 on waiting for second number
    while (1) {
        keyVal = scanKeypad();       
        if (keyVal == 0xFF) { // If no key pressed, continue
            continue;
        }       
        if (keyVal == 0xE) { // Check for reset (* key)
            resetCalculator();
            blinkLED(0x01, 5, 200);
            return 0xFF; // Return special value to indicate reset
        }       
        if (keyVal <= 9) { // Get first digit (0-9)
            num21 = keyVal;
            validInput = true; // valid input detected
            LATD = 0x02; // Keep D2 on to indicate second number mode
            __delay_ms(500);            
            while (1) { // Wait for second digit or hash key
                keyVal = scanKeypad();               
                if (keyVal == 0xFF) { // If no key pressed, continue
                    continue;
                }                
                if (keyVal <= 9) { // Check if second digit (0-9)
                    num22 = keyVal;
                    num2 = (num21 * 10) + num22;
                   
                    if (num2 > MAX_INPUT) {  // Limit to valid range
                        num2 = MAX_INPUT;
                    }
                    LATD = 0x02; // Keep D2 on to indicate second number mode
                    waitingForHashKey = true;
                    return num2;
                }
                // Check if hash key '#', determine if num1 is single digit input
                else if (keyVal == 0xF) {
                    num2 = num21;
                    waitingForHashKey = true;
                    return num2;
                }                
                else if (keyVal == 0xE) { // Check for reset (* key)
                    resetCalculator();
                    blinkLED(0x01, 5, 200);
                    return 0xFF; // Return special value to indicate reset
                }
                __delay_ms(50);
            }
        }
        __delay_ms(50);
    }
}


int doOperation(int num1, int num2, int operator) {
    int result = 0;
        
    switch (operator) { // Perform calculation based on operator
        case 0xA: // 'A' - Addition
            result = num1 + num2;
            break;
        case 0xB: // 'B' - Subtraction
            result = num1 - num2;
            break;
        case 0xC: // 'C' - Multiplication
            result = num1 * num2;
            // Check for overflow
            if (result > MAX_INPUT) {
                result = MAX_INPUT;
            }
            break;
        case 0xD: // 'D' - Division
            if (num2 != 0) {
                result = num1 / num2;
            } else {
                // Division by zero - indicate error
                blinkLED(0xFF, 5, 200); // Blink all LEDs
                return -1; // Error code for division by zero
            }
            break;
        default:
            return 0;
    }    
    return result;
}


void displayBinary(int number) {    
    if (number > 0xFF) { // Limit to maximum displayable value (0xFF)
        number = 0xFF;
    } else if (number < 0) { // Negative numbers are displayed using two's complement        
        number = -number;
        if (number > 0x7F) {
            number = 0x7F; // Limit magnitude to 7 bits (to leave room for sign bit)
        }
    }  
    LATD = (unsigned char)number;   // Display the binary representation directly
}


void displayBinaryWithBlink(int number) {
    bool is_negative = false;
        
    if (number < 0) { // Handle negative numbers
        is_negative = true;
        number = -number; // Take absolute value for display
        if (number > 0x7F) {
            number = 0x7F;
        }
    } else {
        // Limit positive numbers to 8 bits (0-255)
        if (number > 0xFF) {
            number = 0xFF;
        }
    }    
    if (is_negative) {       
        unsigned int blink_counter = 0;     // Blink timing variables
        const unsigned int BLINK_RATE = 5;  // Note: higher rate = slower blink
        bool blink_state = false;
        
        while(1) {
            // Toggle D8 based on blink counter to indicate negative
            blink_counter++;
            if (blink_counter >= BLINK_RATE) {
                blink_state = !blink_state;
                blink_counter = 0;
            }
            
            // Display the binary value with blinking sign bit for negative numbers
            if (blink_state) {               
                LATD = (unsigned char)number | 0x80;  // D8 on as negative indicator
            } else {                
                LATD = (unsigned char)number & 0x7F;  // D8 off 
            }
                        
            keyVal = scanKeypad();
            if (keyVal == 0xE) { // Check for reset (* key)
                resetCalculator();
                return;
            }                      
            __delay_ms(25);  // Small delay for the blink effect
        }
    } else { // Positive number - display normally in binary        
        displayBinary(number);
    }
}


void displayResult() {    
    if (!waitingForHashKey) { // Waiting for # key
        blinkLED(0x08, 3, 200); // Blink D4 to indicate waiting for '#'
    }

    // Wait for '#' key to complete calculation
    while (1) {
        keyVal = scanKeypad();        
        if (keyVal == 0xFF) { // If no key pressed, continue
            continue;
        }
        if (keyVal == 0xF) { // '#' key pressed            
            result = doOperation(num1, num2, operator);  // Calculate result
            
            LATD = 0x00; // Clear LEDs before displaying result                        
            if (result == -1 && num2 == 0) { // Handling division by zero
                // Error has already been displayed in doOperation
                while(1) {
                    keyVal = scanKeypad();
                    if (keyVal == 0xE) {
                        resetCalculator();
                        return;
                    }
                }
            }           
            else {             
                displayBinaryWithBlink(result); // Display negative numbers                               
                if (result >= 0) {
                    while(1) {
                        keyVal = scanKeypad();
                        if (keyVal == 0xE) {
                            resetCalculator();
                            return;
                        }
                    }
                }               
            }
            return;
        }
        else if (keyVal == 0xE) { // '*' key for reset
            resetCalculator();
            blinkLED(0x01, 5, 200);
            return;
        }
        __delay_ms(50);
    }
}


void resetCalculator() {
    // Reset all variables
    num1 = 0;
    num2 = 0;
    operator = 0;
    waitingForHashKey = false;
    validInput = false;
         
    LATD = 0x00;  // Clear display
}


void main() {    
    initialize();  // Initialize hardware
        
    while (1) { // Main program loop        
        resetCalculator();  // Reset calculator state
               
        num1 = getNum1();   // Get first number
        LATD = 0x01; // D1 on
        if (num1 == 0xFF) continue; // Reset occurred
                
        operator = getOperator();  // Get operator
        LATD = 0x04; // D3 on
        if (operator == 0) continue; // Reset occurred
               
        num2 = getNum2();  // Get second number
        LATD = 0x02; // D2 on
        if (num2 == 0xFF) continue; // Reset occurred 
                
        displayResult();  // Display result
                
        __delay_ms(500);  // Short delay before next calculation
    }
}
