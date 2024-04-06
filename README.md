# Spr24 RadHard Research

## Overview:
EEPROM Control Code for Research

This code initializes EEPROMs on a PCB in select banks and then continuously dumps the data to see if there has been a bit flip.
This code is designed to run on a Raspberry Pi 4 with wiringPi library on a 1 MHz I2C bus.

## Known Issues:
- This is not memory safe as there is no code to free the alloc'd data structures. Only ~24 MB of garbage, however. I plan on fixing this soon.
- This code is prone to segmentation faults if any of the pins are disconnected. I don't know the fix for this.
- A full array dump takes approximately 2 minutes if the 512k dump is done in one pass. This is a limitation of the I2C bus. 
- This code could probably be more efficient time & space complexity-wise. I'll probably optimize this at some point.
- There is no tracking individual bit flips. This would be a reasonably simple implementation but time limits me. I do plan on implementing this.

## Files
Control File: radpicode.c
Test Files: filewriting.c , maybe.c

To compile on a Raspberry Pi: **gcc -o rad radpicode.c -l wiringPi**

Memory Usage: ~25 MB allocated total for 16 EEPROMs

CSV Size: ~1.3 KB over 5 minutes
