This sub directory is reserved for projects
PROJECT # 1
03/08/2025  Started the project: 
  Created main.asm V1.0 for Project_1/Part1.  The code is currently saved as hvac.asm in Project1_HVAC in MPLABX IDE.
  The code is working correctly for all three case of positive temperature.  Need to revise to handle negative temp.
03/09/25   Revise main.asm:
  Added function to handle negative measTemp value. Need to work on meeting requirement R9 and R10.
  Also need to work on continuously updating measTemp in REG21 as a real life HVAC system would.
  Also added 'MyConfig.inc' to Projects main branch
03/10/25   Revise main.asm:
  Add functions to convert HEX values into decimals and load into data registers for display.
  Need to work on '_get_abs_val' function to correctly handle all negative value.  For example:  the function currently get
  the absolute value of -10d (0xF6) to 10.  But it will convert -5d (0xFD) to only 3.

PROJECT # 2
03/14/2025 Started the project
  Created counter.asm V1.0 for Project_2/Part_1. The code is currently compile and run without errors.  
  However, Stimulus and Logic Analyzer result does not indicate that the code produces the expected outputs.
  Need to investigate whether Stimulus is correctly set up or there are bugs in the code.
03/16/2025 Revise counter.asm
  Redefine PORTB,0 and 1. Enable weak pull-up register to have inputs' state set as normally HIGH (no contact.)
  Also, redefine PORTD [7:0] as all outputs.  In actual circuit, RD7 is connected to the positive power rail.
  Reworked _display function and physical connections between 7-Segment and PIC18F4K42
  Use the case-switch approach with the corrected 7-segment's pins mapping.
03/17/2025 Revise counter.asm 
  Changed _display function to use lookup table and pointer approach instead of case-switch approach for more efficient code.
03/18/2025 Revise counter.asm
  Redefine inputs, SW_A and SW_B, to PORTA [1 : 0] in preparation for revising the code
  and use PORTB to interface with  a 4x3 keypad.
03/20/2025 Revise counter.asm (V2.0)
  Rework code to use a 4x3 keypad instead of the two momentary switches.
03/21/2025 Revise counter.asm
  Expand keypad scanning operation to include all keys. USe key '*' for incrementing the counter, key '#' for decrementing the counter,
  and key '0' for reseting the counter. Keyes 1-9 have their digit displayed on the 7-segment, and the counting operations starts or reset from that point.
03/21/2025 Revise counter.asm (V3.0)
  Combine the keypad and switches operations into a single program.  Currently have some issue with the switches as followed: On every other cycle, increment skips from E to 0 and decrement skip from 1 to F when using the switches. Possibly caused by insufficient debounce.
03/24/2025  Revise counter.asm
  V3.2: 03/24/2025 - Rework keypad and switches detection logic by adding state detection logic, and counting operation/flow for better debouncing handling

PROJECT # 3
04/04/2025 - Add fully functional code for a simple calculator
           - Add MyConfig.h file to Project main
04/07/2025 - Add reworked code for simple calculator. Result displayed on dual seven segment

PROJECT # 4
04/17/2025 - Add fully functional code ( main.c and 3 header files) for a security system project
           - The files are results of at least 20 revisions.  There are some redundancies between main.c and functions.h as results of trouble shooting.
           - Most bug and problems were hardware related.
  

