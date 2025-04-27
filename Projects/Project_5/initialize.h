/* 
 * File name: initialize.h
 * Purpose: Contains all initialization functions for the photoresistor light measurement project
 * Author: Huy Nguyen 
 * Created on 04/26/2025
 */

#ifndef INITIALIZE_H
#define INITIALIZE_H

#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "C:/Program Files/Microchip/xc8/v3.00/pic/include/proc/pic18f47k42.h"
#include "LCD_Config.h"
#include "functions.h" // Include functions.h to get LCD function declarations

// Global variables
extern int digital;               // ADC result
extern float voltage;             // Converted voltage
extern float lumen;               // Light intensity in lumens
extern char data[10];             // String for LCD display
extern unsigned char systemState; // 0=normal, 1=halted
extern unsigned char interruptTriggered;

// Function prototypes
void ADC_Init(void);
void Interrupt_Init(void);
void System_Init(void);


void Interrupt_Init(void) { // Initialize and configure the interrupt system
    // 1. Disable all interrupts during configuration
    INTCON0bits.GIE = 0;
    
    // 2. Configure RC3 as output for LED with initial state off
    TRISCbits.TRISC3 = 0;     // Set RC3 as output
    LATCbits.LATC3 = 0;       // Initialize LED as off
    ODCONCbits.ODCC3 = 0;     // Disable open drain on LED pin
    ANSELCbits.ANSELC3 = 0;   // Make sure RC3 is digital
    
    // 3. Configure RC2 as input for button with proper characteristics
    TRISCbits.TRISC2 = 1;      // Set RC2 as input
    ANSELCbits.ANSELC2 = 0;    // CRUCIAL: Set RC2 as digital (not analog)
    WPUCbits.WPUC2 = 1;        // Enable weak pull-up
    INLVLCbits.INLVLC2 = 1;    // Set input level to ST (Schmitt Trigger)
    SLRCONCbits.SLRC2 = 0;     // Disable slew rate limiting
    ODCONCbits.ODCC2 = 0;      // Disable open drain
    
    // 4. Set up the Interrupt Vector Table base address
    IVTBASEU = 0x00;           // Upper byte of IVT base address
    IVTBASEH = 0x60;           // High byte of IVT base address
    IVTBASEL = 0x08;           // Low byte of IVT base address
    
    // 5. Configure IOC (Interrupt-On-Change) for RC2
    IOCCFbits.IOCCF2 = 0;      // Clear any pending flag
    PIE0bits.IOCIE = 0;        // Disable IOC interrupts while configuring
    IOCCPbits.IOCCP2 = 1;      // Enable positive edge detection
    IOCCNbits.IOCCN2 = 1;      // Enable negative edge detection
    
    // 6. Set up interrupt priority
    IPR0bits.IOCIP = 1;        // High priority for IOC interrupts
    
    // 7. Enable IOC for PORTC and clear main IOC flag
    PIR0bits.IOCIF = 0;        // Clear main IOC interrupt flag
    PIE0bits.IOCIE = 1;        // Enable IOC interrupts
    
    // 8. Enable priority system and global interrupts
    INTCON0bits.IPEN = 1;      // Enable interrupt priority
    INTCON0bits.GIEH = 1;      // Enable high priority interrupts
    INTCON0bits.GIEL = 1;      // Enable low priority interrupts
    INTCON0bits.GIE = 1;       // Enable global interrupts
}


void ADC_Init(void) { // Initialize and configure the ADC
    // Reset ADC module first
    ADCON0 = 0x00;         // Turn off ADC to reset it
    
    // Configure ADC
    ADCON0bits.FM = 1;     // Right justify result
    ADCON0bits.CS = 1;     // ADC clock source ADCRC
    
    // Configure pin RA0 for analog input
    TRISAbits.TRISA0 = 1;  // Set RA0 as input
    ANSELAbits.ANSELA0 = 1; // Set RA0 as analog
    
    // Select channel and configure ADC parameters
    ADPCH = 0x00;          // Select RA0/ANA0 as input channel
    ADCLK = 0x01;          // Set ADC clock to FOSC/4 for better stability
    
    // Clear result registers
    ADRESH = 0x00;
    ADRESL = 0x00;
    
    // Configure acquisition settings
    ADACQL = 0x08;         // Set acquisition time to 8 TAD for better stability
    ADACQH = 0x00;
    
    // Turn on ADC module
    ADCON0bits.ON = 1;     // Turn ADC on
    
    // Short delay to allow ADC to stabilize
    __delay_ms(1);
}


void System_Init(void) { // Initialize all peripherals and I/O ports
    // Disable all analog functionalities
    ANSELA = 0;  ANSELB = 0; ANSELC = 0; ANSELD = 0;
        
    LCD_Init();  // Initialize LCD
        
    ADC_Init();  // Initialize ADC
       
    Interrupt_Init();  // Initialize Interrupts
    
    // Display initial message
    LCD_Command(0x01);    // Clear display
    LCD_String_xy(1, 0, "Input light:");
    LCD_String_xy(2, 3, "Reading...");
	__delay_ms(2000);
       
    PIE1bits.ADIE = 0;  // Disable ADC interrupts for polling mode
        
    ADCON0bits.GO = 1; // Start first ADC conversion
}

#endif /* INITIALIZE_H */
