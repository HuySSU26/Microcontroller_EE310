/*
 * File: initialize.h
 * Author: Huy Nguyen
 * 
 * Created on April 17, 2025
 * 
 * Purpose: Initialization values for the security system
 */

#ifndef INIT_H
#define INIT_H

#include <xc.h>
#include <stdbool.h>


#define LOCKING_CODE 0x21  // Secret code (tens digit = 2, ones digit = 1)

//=============================================================================
// HARDWARE DEFINITIONS
//=============================================================================

// PORT A pins
#define DIGIT_ONES_PIN      1   // RA1 output (ones digit)
#define MOTOR_RELAY_PIN     2   // RA2 output (motor)
#define BUZZER_PIN          5   // RA5 output (buzzer)

// PORT C pins
#define LED_D1_PIN          2   // RC2 output (LED D1)
#define LED_D2_PIN          3   // RC3 output (LED D2)
#define PHOTORESISTOR1_PIN  4   // RC4 input (PR1)
#define PHOTORESISTOR2_PIN  5   // RC5 input (PR2)
#define CONFIRM_PIN         7   // RC7 input (confirm button)

// PORT B pins
#define EMERGENCY_PIN       0   // RB0 input (emergency button)

// Control macros
#define LED_D1_ON()         LATCbits.LATC2 = 1
#define LED_D1_OFF()        LATCbits.LATC2 = 0
#define LED_D2_ON()         LATCbits.LATC3 = 1
#define LED_D2_OFF()        LATCbits.LATC3 = 0
#define BUZZER_ON()         LATAbits.LATA5 = 1
#define BUZZER_OFF()        LATAbits.LATA5 = 0
#define MOTOR_ON()          LATAbits.LATA2 = 1
#define MOTOR_OFF()         LATAbits.LATA2 = 0

// 7-segment display segments
#define SEG_G               0   // RD0 - Segment G
#define SEG_F               1   // RD1 - Segment F
#define SEG_E               2   // RD2 - Segment E
#define SEG_D               3   // RD3 - Segment D
#define SEG_C               4   // RD4 - Segment C
#define SEG_B               5   // RD5 - Segment B
#define SEG_A               6   // RD6 - Segment A
#define SEG_DP              7   // RD7 - Decimal Point

// Segment patterns for digits 0-9
#define PATTERN_0  ((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_F))
#define PATTERN_1  ((1 << SEG_B) | (1 << SEG_C))
#define PATTERN_2  ((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_D) | (1 << SEG_E) | (1 << SEG_G))
#define PATTERN_3  ((1 << SEG_A) | (1 << SEG_B) | (1 << SEG_C) | (1 << SEG_D) | (1 << SEG_G))
#define PATTERN_4  ((1 << SEG_B) | (1 << SEG_C) | (1 << SEG_F) | (1 << SEG_G))

//=============================================================================
// SYSTEM STATE DEFINITIONS
//=============================================================================
typedef enum {
    STATE_READY,       // System is ready for code entry
    STATE_TENS_INPUT,  // Getting tens digit input from PR1
    STATE_ONES_INPUT,  // Getting ones digit input from PR2
    STATE_UNLOCKED,    // System is unlocked (code matched)
    STATE_EMERGENCY    // Emergency interrupt triggered
} SystemState;

//=============================================================================
// GLOBAL VARIABLES
//=============================================================================
extern SystemState system_state;
extern unsigned char current_digit;   // Currently displayed digit
extern unsigned char tens_digit;      // Stored tens digit
extern unsigned char ones_digit;      // Stored ones digit
extern unsigned char entered_code;    // Final entered code
extern volatile bool emergency_active;

// Counter for global timing
extern unsigned int debounce_counter;
extern unsigned char blink_counter;  // Counter for blinking LED D1

#endif /* INIT_H */
