/*
 * File: functions.h
 * Author: Huy Nguyen
 * 
 * Created on April 17, 2025
 * 
 * Purpose: Function declarations and implementations for the security system
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <xc.h>
#include "initialize.h"
#include "config.h"

//=============================================================================
// FUNCTION DECLARATIONS
//=============================================================================

void initialize_system(void);  // Initialize the system
void display_digit(unsigned char digit);  // Display a digit on the 7-segment display
void beep(unsigned char beep_type); // Beep function with fixed durations(1 = 50ms, 2 = 300ms, 3 = 500ms)
void play_emergency_melody(void);  // Play emergency melody on buzzer
void play_incorrect_code(void);  // Play incorrect code buzzer
void handle_unlock(void);  // Handle system unlocking
void blink_d1(void);  // Blink LED D1
void process_button_press(void);  // Process button press
void process_pr1(bool pr1_covered);  // Process PR1 (tens digit)
void process_pr2(bool pr2_covered); // Process PR2 (ones digit)
bool pr_just_covered(bool current, bool *previous);  // Check if a PR has just been covered (rising edge detection)
void __interrupt(irq(IRQ_INT0), base(0x4008)) ISR(void);  // Interrupt service routine

//=============================================================================
// FUNCTION IMPLEMENTATIONS
//=============================================================================

// Initialize the system
void initialize_system(void) {
    // Disable all analog functionality
    ANSELA = 0; ANSELB = 0;  ANSELC = 0; ANSELD = 0;
        
    // Set up inputs/outputs
    TRISAbits.TRISA1 = 0;  // RA1 output (ones digit)
    TRISAbits.TRISA2 = 0;  // RA2 output (motor)
    TRISAbits.TRISA5 = 0;  // RA5 output (buzzer)
    
    TRISCbits.TRISC2 = 0;  // RC2 output (LED D1)
    TRISCbits.TRISC3 = 0;  // RC3 output (LED D2)
    TRISCbits.TRISC4 = 1;  // RC4 input (PR1)
    TRISCbits.TRISC5 = 1;  // RC5 input (PR2)
    TRISCbits.TRISC7 = 1;  // RC7 input (confirm button)
    
    TRISBbits.TRISB0 = 1;  // RB0 input (interrupt button)
    
    TRISD = 0;            // All PORTD pins as outputs for 7-segment
    
    // Enable weak pull-ups for inputs
    WPUA = 0;              // No pull-ups on PORTA
    WPUB = (1 << 0);       // Pull-up on RB0 (iterrupt button)
    WPUC = (1 << 7);       // Pull-up on RC7 (confirm button)
    
    // Initialize outputs
    LED_D1_OFF();         // D1 will blink
    LED_D2_ON();          // D2 on solid
    MOTOR_OFF();          // Motor off
    BUZZER_OFF();         // Buzzer off
    
    // Initialize interrupts for the emergency button on RB0/INT0
    // Disable interrupts while configuring
    INTCON0bits.GIEH = 0;     // Disable high priority interrupts
    INTCON0bits.GIEL = 0;     // Disable low priority interrupts

    INTCON0bits.IPEN = 1;     // Enable interrupt priority
    
    // Configure INT0 for active-low button (falls when pressed)
    INTCON0bits.INT0EDG = 0;  // Interrupt on falling edge (button press)
    IPR1bits.INT0IP = 1;      // High priority for INT0
    PIE1bits.INT0IE = 1;      // Enable INT0 interrupt
    PIR1bits.INT0IF = 0;      // Clear interrupt flag
    
    // Change IVTBASE
    IVTBASEU = 0x00;
    IVTBASEH = 0x40;
    IVTBASEL = 0x08;
    
    // Enable interrupts
    INTCON0bits.GIEH = 1;     // Enable high priority interrupts
    INTCON0bits.GIEL = 1;     // Enable low priority interrupts
    
    // Initialize display
    current_digit = 0;
    display_digit(0);         // Explicitly display digit 0
    
    // Initial state
    system_state = STATE_READY;
    tens_digit = 0;
    ones_digit = 0;
    
    // Reset counters
    debounce_counter = 0;
    blink_counter = 0;
}

// Display a digit on the 7-segment display
void display_digit(unsigned char digit) {
    // Pick the pattern based on the digit
    unsigned char pattern;
    switch(digit) {
        case 0: pattern = PATTERN_0; break;
        case 1: pattern = PATTERN_1; break;
        case 2: pattern = PATTERN_2; break;
        case 3: pattern = PATTERN_3; break;
        case 4: pattern = PATTERN_4; break;
        default: pattern = PATTERN_0; break; // Default to 0 for invalid digits
    }
       
    LATD = pattern;  // Set the segment pattern
       
    LATAbits.LATA1 = 1; // Enable 7-Segment ones digit
}

// Beep function with fixed durations
void beep(unsigned char beep_type) {
    BUZZER_ON();
    switch(beep_type) {
        case 1:  // Short beep
            __delay_ms(50);
            break;
        case 2:  // Medium beep
            __delay_ms(300);
            break;
        case 3:  // Long beep
            __delay_ms(500);
            break;
        default:  // Default short beep
            __delay_ms(50);
            break;
    }
    BUZZER_OFF();
        
    __delay_ms(50); // Add silence between beeps
}

// Play emergency melody on buzzer
void play_emergency_melody(void) {
    // Play distinctive emergency melody
    for (int i = 0; i < 3; i++) {
        // High tone
        LATAbits.LATA5 = 1;  // Buzzer on
        __delay_ms(200);
        LATAbits.LATA5 = 0;  // Buzzer off
        __delay_ms(100);
        
        // Low tone
        LATAbits.LATA5 = 1;  // Buzzer on
        __delay_ms(400);
        LATAbits.LATA5 = 0;  // Buzzer off
        __delay_ms(200);
    }
}

// Play incorrect code buzzer
void play_incorrect_code(void) {
    BUZZER_ON();
    __delay_ms(2000);  // 2 seconds of buzzer
    BUZZER_OFF();
}

// Handle system unlocking
void handle_unlock(void) {
    // Turn D1 solid, D2 off
    LED_D1_ON();
    LED_D2_OFF();
       
    beep(3);  // Success beep (long)
        
    MOTOR_ON();  // Turn on motor for 5 seconds
    for (unsigned int i = 0; i < 5; i++) {
        __delay_ms(1000);  // 5 seconds total
    }
    MOTOR_OFF();
    
    // Return to ready state
    LED_D1_OFF();  // D1 will blink in main loop
    LED_D2_ON();   // D2 back on
}

// Blink LED D1
void blink_d1(void) {
    blink_counter++;
    if (blink_counter >= 25) {
        LED_D1_ON();
    }
    if (blink_counter >= 50) {
        LED_D1_OFF();
        blink_counter = 0;
    }
}

// Check if a PR has just been covered (rising edge detection)
bool pr_just_covered(bool current, bool *previous) {
    bool result = current && !(*previous);
    *previous = current;
    return result;
}

// Process button press
void process_button_press(void) {
    
    beep(2);  // Audio feedback
    
    // Process based on current state
    switch (system_state) {
        case STATE_READY:
            system_state = STATE_TENS_INPUT;
            tens_digit = 0;
            current_digit = 0;
            LATD = PATTERN_0;
            break;
            
        case STATE_TENS_INPUT:
            system_state = STATE_ONES_INPUT;
            ones_digit = 0;
            current_digit = 0;
            LATD = PATTERN_0;
            break;
            
        case STATE_ONES_INPUT:
            // Form entered code (tens digit in upper nibble, ones digit in lower nibble)
            entered_code = ((unsigned char)(tens_digit) << 4) | (unsigned char)(ones_digit);
                        
            if (entered_code == LOCKING_CODE) { // Check if code matches              
                handle_unlock(); // Correct code entered - unlock
            } 
			else {              
                LED_D2_ON(); // Incorrect code entered - keep D2 on               
                play_incorrect_code(); // Failure beep
            }
            
            // Reset state
            system_state = STATE_READY;
            current_digit = 0;
            LATD = PATTERN_0;
            break;
            
        default:
            // Reset to ready state
            system_state = STATE_READY;
            current_digit = 0;
            LATD = PATTERN_0;
            break;
    }
       
    __delay_ms(300); // Debounce delay
}

// Process PR1 (tens digit)
void process_pr1(bool pr1_covered) {
    if (system_state == STATE_TENS_INPUT) {
        if (pr1_covered) {
            // Increment tens digit (with rollover)
            if (tens_digit < 4) {
                tens_digit++;
            } else {
                tens_digit = 0;
            }
            
            // Update display
            current_digit = tens_digit;
            display_digit(current_digit);
            
            // Feedback
            beep(1);  // Short beep
            
            // Small delay for stability
            __delay_ms(200);
        }
    }
}

// Process PR2 (ones digit)
void process_pr2(bool pr2_covered) {
    if (system_state == STATE_ONES_INPUT) {
        if (pr2_covered) {
            // Increment ones digit (with rollover)
            if (ones_digit < 4) {
                ones_digit++;
            } else {
                ones_digit = 0;
            }
            
            // Update display
            current_digit = ones_digit;
            display_digit(current_digit);
            
            // Visual feedback - flash D2
            LED_D2_OFF();
            __delay_ms(50);
            LED_D2_ON();
			
            beep(1);  // Audible feedback - Short beep
                       
            __delay_ms(200); // Small delay for signal stabilization
        }
    }
}

// Interrupt service routine
void __interrupt(irq(IRQ_INT0), base(0x4008)) ISR(void) {
    if (PIR1bits.INT0IF) {
        
        play_emergency_melody();  // Play emergency melody
               
        emergency_active = true;  // Signal emergency
        
        // Visual feedback
        LED_D1_ON();
        __delay_ms(500);
        LED_D1_OFF();
               
        PIR1bits.INT0IF = 0;  // Clear interrupt flag
    }
}

#endif /* FUNCTIONS_H */
