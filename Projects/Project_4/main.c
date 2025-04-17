/*File: main.c
 * 
 * Author: Huy Nguyen
 * 
 * Created on April 17, 2025
 * 
 * Purpose: This program is a simulation of a security system used on a lock box.   The locking
 *	  		code is preset.  The unlocking code is enter by using two touchless switches. 
 */

#include <xc.h>
#include <stdbool.h>
#include "config.h"
#include "initialize.h"
#include "functions.h"
#include "C:/Program Files/Microchip/xc8/v3.00/pic/include/proc/pic18f47k42.h"

//=============================================================================
// GLOBAL VARIABLES DEFINITION
//=============================================================================
SystemState system_state = STATE_READY;
unsigned char current_digit = 0;  // Currently displayed digit
unsigned char tens_digit = 0;     // Stored tens digit
unsigned char ones_digit = 0;     // Stored ones digit
unsigned char entered_code = 0;   // Final entered code
volatile bool emergency_active = false;

// Global counters
unsigned int debounce_counter = 0;
unsigned char blink_counter = 0;  // Counter for blinking LED D1


void main(void) {
	
    initialize_system();
    
    // Initial startup feedback
    beep(1); // Short beep
    __delay_ms(100);
    beep(1); // Short beep
    
    // Previous input states
    bool prev_button = 1;  // Active-low (1 = not pressed)
    bool prev_pr1 = 0;     // Previous PR1 state
    bool prev_pr2 = 0;     // Previous PR2 state
    
    // Flags to track PR activation
    bool pr1_activated = false;
    bool pr2_activated = false;
    
    // Counter for system resets
    unsigned int reset_counter = 0;
    
    
    while(1) { // main loop
	
        blink_d1();
        
        // Process emergency if active
        if (emergency_active) {
            system_state = STATE_READY;
            current_digit = 0;
            tens_digit = 0;
            ones_digit = 0;
            LATD = PATTERN_0;
            reset_counter = 0;
            pr1_activated = false;
            pr2_activated = false;
            
            // Reset previous states
            prev_pr1 = 0;
            prev_pr2 = 0;
            prev_button = 1;
            
            emergency_active = false;
        }
        
        // Read inputs
        bool button_state = PORTCbits.RC7;  // 0 when pressed (active-low)
        bool pr1_state = PORTCbits.RC4;     // 1 when covered
        bool pr2_state = PORTCbits.RC5;     // 1 when covered
        
        // Periodically reinitialize the pins
        reset_counter++;
        if (reset_counter >= 1000) {  // About every 20 seconds
            // Reconfigure PR pins to ensure they're in correct state
            ANSELC &= ~((1 << 4) | (1 << 5));  // Disable analog functionality for RC4/RC5
            TRISCbits.TRISC4 = 1;  // RC4 input (PR1)
            TRISCbits.TRISC5 = 1;  // RC5 input (PR2)
            
            // Reset flags
            if (!pr1_state) pr1_activated = false;
            if (!pr2_state) pr2_activated = false;
            
            reset_counter = 0;
        }
               
        if (!button_state && prev_button) { // BUTTON PRESS DETECTION (active-low)
            
            beep(2);  // Audio feedback with medium beep
                       
            switch (system_state) { // Handle state transitions
                case STATE_READY:
                    system_state = STATE_TENS_INPUT;
                    tens_digit = 0;
                    current_digit = 0;
                    LATD = PATTERN_0;
                    
                    // Reset PR flags
                    pr1_activated = false;
                    pr2_activated = false;
                    prev_pr1 = 0;
                    prev_pr2 = 0;
                    break;
                    
                case STATE_TENS_INPUT:
                    system_state = STATE_ONES_INPUT;
                    ones_digit = 0;
                    current_digit = 0;
                    LATD = PATTERN_0;
                    
                    // Reset PR flags
                    pr1_activated = false;
                    pr2_activated = false;
                    prev_pr1 = 0;
                    prev_pr2 = 0;
                    break;
                    
                case STATE_ONES_INPUT:
                    // Form entered code
                    entered_code = ((unsigned char)(tens_digit) << 4) | (unsigned char)(ones_digit);
                                      
                    if (entered_code == LOCKING_CODE) {  // Check if code matches
                        // Success - Turn D1 solid, D2 off
                        LED_D1_ON();
                        LED_D2_OFF();
                        
                        beep(3);  // Success = long beep
                        
                        // Motor on for 5 seconds
                        MOTOR_ON();
                        __delay_ms(5000);
                        MOTOR_OFF();
                        
                        // Reset to default
                        LED_D1_OFF();  // Blinks in main loop
                        LED_D2_ON();
                    } else {
                        
                        LED_D2_ON(); // Incorrect code entered. Box stays locked
                        
                        // Failure beep
                        BUZZER_ON();
                        __delay_ms(2000);
                        BUZZER_OFF();
                    }
                    
                    // Reset state
                    system_state = STATE_READY;
                    current_digit = 0;
                    LATD = PATTERN_0;
                    
                    // Reset flags after code check
                    pr1_activated = false;
                    pr2_activated = false;
                    prev_pr1 = 0;
                    prev_pr2 = 0;
                    break;
                    
                default:
                    system_state = STATE_READY;
                    current_digit = 0;
                    LATD = PATTERN_0;
                    break;
            }
                       
            __delay_ms(300); // Debounce delay
        }
               
        if (system_state == STATE_TENS_INPUT) { // PR1 HANDLING - TENS DIGIT            
            if (pr1_state && !prev_pr1 && !pr1_activated) { // Handle rising edge (PR1 just covered)               
                if (tens_digit < 4) { // Increment tens digit
                    tens_digit++;
                } else {
                    tens_digit = 0;
                }
                               
                current_digit = tens_digit; // Update display
                LATD = 0;  // Clear display briefly
                __delay_ms(5);
                switch (current_digit) {
                    case 0: LATD = PATTERN_0; break;
                    case 1: LATD = PATTERN_1; break;
                    case 2: LATD = PATTERN_2; break;
                    case 3: LATD = PATTERN_3; break;
                    case 4: LATD = PATTERN_4; break;
                    default: LATD = PATTERN_0; break;
                }
                               
                LATAbits.LATA1 = 1; // Enabled 7-Segment ones digit
                
                beep(1);  // Short beep
                               
                pr1_activated = true; // Register as activated until PR1 is uncovered
                                
                __delay_ms(200); // Small delay for signal stabilization 
            }
                       
            if (!pr1_state) { // Reset activation flag when PR1 is uncovered
                pr1_activated = false;
            }
        }
               
        if (system_state == STATE_ONES_INPUT) { // PR2 HANDLING - ONES DIGIT
            
            if (pr2_state && !prev_pr2 && !pr2_activated) { // Handle rising edge (PR2 just covered)               
                if (ones_digit < 4) { // Increment ones digit
                    ones_digit++;
                } else {
                    ones_digit = 0;
                }
                                
                current_digit = ones_digit; // Update display
                LATD = 0;  // Clear display briefly
                __delay_ms(5);
                switch (current_digit) {
                    case 0: LATD = PATTERN_0; break;
                    case 1: LATD = PATTERN_1; break;
                    case 2: LATD = PATTERN_2; break;
                    case 3: LATD = PATTERN_3; break;
                    case 4: LATD = PATTERN_4; break;
                    default: LATD = PATTERN_0; break;
                }
                                
                LATAbits.LATA1 = 1;
                
                // Visual feedback
                LED_D2_OFF();
                __delay_ms(50);
                LED_D2_ON();
                
                beep(1);  // Audible feedback - Short beep
                              
                pr2_activated = true; // Rgistered as activated until PR2 is uncovered
                               
                __delay_ms(200); // Small delay for signal stabiliztion
            }
                       
            if (!pr2_state) { // Reset activation flag when PR2 is uncovered
                pr2_activated = false;
            }
        }
        
        // Update previous states at the end of each loop
        prev_button = button_state;
        prev_pr1 = pr1_state;
        prev_pr2 = pr2_state;        
        
        __delay_ms(20); // Debounce
    }
}
