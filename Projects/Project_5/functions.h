/* 
 * File name: functions.h
 * Purpose: Contains all operation functions for the photoresistor light measurement project
 * Author: Huy Nguyen 
 * Created on 04/26/2025
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <xc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "C:/Program Files/Microchip/xc8/v3.00/pic/include/proc/pic18f47k42.h"
#include "LCD_Config.h"

// LCD interface definitions
#define RS LATD0                   /* PORTD 0 pin is used for Register Select */
#define EN LATD1                   /* PORTD 1 pin is used for Enable */
#define ldata LATB                 /* PORTB is used for transmitting data to LCD */
#define LCD_Port TRISB              
#define LCD_Control TRISD
#define Vref 5.0  // Reference voltage for ADC

// Global variables (defined in main.c)
extern int digital;               // ADC result
extern float voltage;             // Converted voltage
extern float lumen;               // Light intensity in lumens
extern char data[10];             // String for LCD display
extern unsigned char interruptTriggered;
extern unsigned char systemState; // 0=normal, 1=halted

// Function prototypes
void MSdelay(unsigned int val);
void LCD_Command(char cmd);
void LCD_Char(char dat);
void LCD_String(const char *msg);
void LCD_String_xy(char row, char pos, const char *msg);
void LCD_Init(void);
void Read_Voltage(void);
void Read_Light_Level(void);
void Handle_System_Halt(void);


void MSdelay(unsigned int val) {  // Generate a millisecond delay
    unsigned int i, j;
    for(i = 0; i < val; i++)
        for(j = 0; j < 165; j++); /*This count Provide delay of 1 ms for 8MHz Frequency */
}


void LCD_Command(char cmd) {  // Send a command to the LCD
    ldata = cmd;           /* Send data to PORT as a command for LCD */   
    RS = 0;                /* Command Register is selected */
    EN = 1;                /* High-to-Low pulse on Enable pin to latch data */ 
    NOP();
    EN = 0;
    MSdelay(3); 
}


void LCD_Char(char dat) { // Send a single character to the LCD
    ldata = dat;           /* Send data to LCD */  
    RS = 1;                /* Data Register is selected */
    EN = 1;                /* High-to-Low pulse on Enable pin to latch data */   
    NOP();
    EN = 0;
    MSdelay(1);
}


void LCD_String(const char *msg) { // Send a string to the LCD
    while((*msg) != 0) {       
        LCD_Char(*msg);
        msg++;    
    }
}


void LCD_String_xy(char row, char pos, const char *msg) { // Position cursor and display a string at that position
    char location = 0;
    if(row <= 1) {
        location = (0x80) | ((pos) & 0x0f); /*Print message on 1st row and desired location*/
        LCD_Command(location);
    } else {
        location = (0xC0) | ((pos) & 0x0f); /*Print message on 2nd row and desired location*/
        LCD_Command(location);    
    }  
    LCD_String(msg);
}


void LCD_Init(void) { // Initialize the LCD display
    MSdelay(15);           /* 15ms,16x2 LCD Power on delay */
    LCD_Port = 0x00;       /* Set PORTB as output PORT for LCD data(D0-D7) pins */
    LCD_Control = 0x00;    /* Set PORTD as output PORT LCD Control(RS,EN) Pins */
    LCD_Command(0x01);     /* clear display screen */
    LCD_Command(0x38);     /* uses 2 line and initialize 5*7 matrix of LCD */
    LCD_Command(0x0c);     /* display on cursor off */
    LCD_Command(0x06);     /* increment cursor (shift cursor to right) */
}


void Read_Voltage(void) { // Read the current voltage from the ADC   
    if (ADCON0bits.GO == 0) { // Check if ADC conversion is complete
        // Get ADC result and calculate voltage
        digital = (ADRESH*256) | (ADRESL); 
        voltage = digital*((float)Vref/(float)(4096));
        
        // Display light reading
        sprintf(data,"%.2f",voltage);
        strcat(data," V");	//Concatenate result and unit to print
        LCD_String_xy(2, 3, data);
                
        ADCON0bits.GO = 1;  // Start next conversion
    }
}


void Read_Light_Level(void) { // Read the current light level from the ADC    
    if (ADCON0bits.GO == 0) { // Check if ADC conversion is complete
        // Get ADC result and calculate lumen
        digital = (ADRESH*256) | (ADRESL); 
        voltage = digital*((float)Vref/(float)(4096));
        lumen = -302 * voltage + 1498.3;
        
        // Display light reading
        sprintf(data, "%.2f", lumen);
        strcat(data, " lux  "); 
        LCD_String_xy(2, 3, data);
               
        ADCON0bits.GO = 1;  // Start next conversion
    }
}


void Handle_System_Halt(void) { // Handle system halt state when interrupt is triggered
    unsigned int haltCounter = 0;
    
    // System has been interrupted - enter halt state
    LCD_Command(0x01);    // Clear display
    LCD_String_xy(1, 0, "SYSTEM HALTED");
    LCD_String_xy(2, 0, "For 10 seconds");
    
    // LED blink for 10 seconds (20 blinks)
    for (haltCounter = 0; haltCounter < 20; haltCounter++) {
        LATCbits.LATC3 = 1;       // LED ON 
        __delay_ms(250);                        
        LATCbits.LATC3 = 0;       // LED OFF
        __delay_ms(250);          
    }
    
    // Reset system 
    interruptTriggered = 0;
    systemState = 0;       // Reset system state
    LATCbits.LATC3 = 0;    // Ensure LED is off
    
    // Clear any pending interrupt flags
    IOCCFbits.IOCCF2 = 0;
    PIR0bits.IOCIF = 0;
    
    // Return to normal display
    LCD_Command(0x01);     // Clear display again to ensure it's cleared
    LCD_String_xy(1, 0, "Input light:");
    LCD_String_xy(2, 3, "Resuming...");
    __delay_ms(1000);      // Show "Resuming..." for 1 second
        
    ADCON0bits.GO = 1;  // Restart ADC conversion
}

#endif /* FUNCTIONS_H */
