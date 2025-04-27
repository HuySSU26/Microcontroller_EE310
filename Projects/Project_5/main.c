/* 
 * File name: main.c
 * Purpose: 
 * 		This program is a modification of the codes given by Dr. Farahmand.
 *      It is used to demonstrate a light measurement system using a photoresistor.
 *      The input analog signal to the PIC18F47K42 is measured and converted to
 *      light intensity in lux. The program uses polling for continuous updates.
 * Input:  PORTA [0] - analog input to PIC from photoresistor
 *         PORTC [2] - control interrupt service routine
 * Output: PORTB [0 : 7] - output data to LCD display's pins D0-D7
 *         PORTC [3] - control interrupt indicator LED
 *         PORTD [0: 1] - control LCD display's Register Select and Enable pins 
 * Author: Huy Nguyen 
 * Version: 1.0 04/21/2025 - Use 10KÎ© potentiometer to control input DC voltage to RA0. 
 *
 *			1.1 04/22/2025 - Modify code to use photoresistor to control input voltage to RA0.
 *				Photo sensor characterization:
 * 
 *			 		+ ------------	+---+ Light on +---+ Light off
 *			 		Voltage (Vdc)	+---+  1.65	   +---+	4.63 
 *			        Lumen   (lux)	+---+  1000	   +---+	100
 *
 *		        	For y = mx + b => m = (1000-100)/(1.65-4.63) = -302; and b = 1000 + (302*1.65) = 1498.3
 *			    	=> lumen = -302 * voltage +1498.3;
 *
 *			1.2 04/24/2025 - Add ADC interrupt support.
 *			1.3 04/26/2025 - Create header files to streamline program.  
 *
 */

#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "C:/Program Files/Microchip/xc8/v3.00/pic/include/proc/pic18f47k42.h"
#include "LCD_Config.h"
#include "functions.h" 
#include "initialize.h" 

// Global variables
int digital;                       // ADC result
float voltage;                     // Converted voltage
float lumen;                       // Light intensity in lumens
char data[10];                     // String for LCD display
unsigned char interruptTriggered = 0;  // Flag to indicate interrupt has occurred
unsigned char systemState = 0;         // 0=normal, 1=halted

// Main function
void main(void) {   
    System_Init();  // Initialize all peripherals and start the system
       
    while(1) { // Main loop        
        if (systemState == 0) { // Check if in normal operating mode            
            Read_Light_Level(); // Read and display light level
                        
            __delay_ms(300);  // Small delay for signal stabilization
        }
                
        if (interruptTriggered && systemState == 1) { // Check if interrupt button was pressed
            Handle_System_Halt();
        }
    }
}

// Interrupt Service Routine
void __interrupt(irq(default), base(0x6008)) my_ISR(void) {    
    if (PIR0bits.IOCIF) { // Check if IOC interrupt has occurred       
        if (IOCCFbits.IOCCF2) { // Check if specifically RC2 triggered it
            
            if (systemState == 0) { // Process the interrupt if not in HALT state               
                systemState = 1;  // Set system to HALT state
                interruptTriggered = 1;
                LATCbits.LATC3 = 1;  // Turn LED ON
            }
            
            // Read the port to clear mismatch condition
            unsigned char portValue = PORTC;
            
            // Clear the interrupt flags
            IOCCFbits.IOCCF2 = 0;     // Clear the pin-specific flag first
            PIR0bits.IOCIF = 0;       // Clear the master IOC flag
        }
    }
        
    if (PIR1bits.ADIF) { // Clear ADC interrupt flag if set       
        PIR1bits.ADIF = 0;
    }
}
