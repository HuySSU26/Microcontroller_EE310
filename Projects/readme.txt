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
