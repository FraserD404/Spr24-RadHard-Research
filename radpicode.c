/*

C Code for EEPROM Control for Research

April 4, 2024
Author: Fraser Dougall 
Email: fdougall@purdue.edu

*/

// Libraries
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <time.h>
#include <stdbool.h>

// Selector Pins
#define BANK_SELECT_1 0
#define BANK_SELECT_2 1

// EEPROM Banking Information
#define EEPROM_ADDRESS 0x50 // base EEPROM I2C address
#define NUM_BANKS 3
#define EEPROMS_PER_BANK 8

// 512k Speedup constant (bytes)
#define FAST_READ_SIZE 32000

// How long to run the test - seconds
// default is 30 min -> 1800 seconds
#define RUNNING_TIME_SEC 1800

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Return how many bytes are stored in a given bank,EEPROM combo
// this is a really stupid and inefficient piece of code 
// figure out a better way
int getEEPROMSize(int bank, int num) {
    int sizeKB;

    switch(bank){
        case 0: 
            if(num >= 0 && num <= 3) { 
                sizeKB = 4;
            } else {
                sizeKB = 512; 
            }

            break;
        case 1: 
            if(num >= 0 && num <= 3) { 
                sizeKB = 32;
            } else { 
                sizeKB = 128; 
            }

            break;
        case 2: 
            if(num >= 0 && num <= 3) { 
                sizeKB = 4;
            } else {
                sizeKB = -1; 
            } 

            break;
        default:
            return -1; 
            break;
    }

    // kilobytes -> bytes
    return 1000 * sizeKB; 
}

/*
Initialize GPIO Pins
*/
void initGPIO() {
    wiringPiSetup();

    pinMode(BANK_SELECT_1, OUTPUT);
    pinMode(BANK_SELECT_2, OUTPUT);
}

/*
Choose with bank of EEPROM we are looking at by changing which switch state we are at
*/
void selectBank(int bank) {
    switch (bank) {
        case 0:
            digitalWrite(BANK_SELECT_1, LOW);
            digitalWrite(BANK_SELECT_2, LOW);

            break;
        case 1:
            digitalWrite(BANK_SELECT_1, HIGH);
            digitalWrite(BANK_SELECT_2, LOW);
	    
            break;
        case 2:
            digitalWrite(BANK_SELECT_1, HIGH);
            digitalWrite(BANK_SELECT_2, HIGH);
	    
            break;
        default:
            printf("Invalid bank number\n");
            
            break;
    }
}

/* 
Initialize all EEPROMs to have 0xFF in all memory locations
*/
void initEEPROMs() {
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        selectBank(bank);

        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            int eepromAddr = wiringPiI2CSetup(EEPROM_ADDRESS + eeprom);

            if (eepromAddr < 0) {
                printf("Failed to initialize EEPROM %d in bank %d\n", eeprom, bank);
            } else {
		        int sizeBytes = getEEPROMSize(bank, eeprom);
		        bool init = true;

                // Linearly initialize all locations in EEPROM ; wish this was faster but impossible for better than O(n)
                for (int num = 0; num < sizeBytes; num++) {
                    if (wiringPiI2CWrite(eepromAddr, 0xFF) == -1) {
                        printf("Failed to write to EEPROM %d in bank %d\n", eeprom, bank);
                        init = false;

                        break;
                    } 		
                }

                if(init) {
                    printf("Initialized EEPROM %d in bank %d\n", eeprom, bank);
                }

                close(eepromAddr);
            }
        }
    }
}

void logger(time_t startTime, int greedy, int boardNum, FILE* csv_file, int* eepromFailures) {
    time_t currTime = 0;
    int elapsedTime = 0; 
    int eepromSize = 0; 

    // Go through b-th bank and set e-th EEPROM
    for(int b = 0; b < NUM_BANKS; b++) {
        for(int e = 0; e < EEPROMS_PER_BANK; e++) {
            eepromFailures[b * NUM_BANKS + e] = 0; 
        }
    }

    // Initialize which addresses have failed

    // Go through all EEPROMs and banks 
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        selectBank(bank);

        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            int eepromAddr = wiringPiI2CSetup(EEPROM_ADDRESS + eeprom);

            if (eepromAddr == -1) {
                fprintf("Bank %d, EEPROM%d, Failures: -1\n", bank, eeprom);
            } else {
	        	eepromSize = getEEPROMSize(bank, eeprom);

                // Speed up reads of 512k's if we are speedy
                if(eepromSize == 512000 && greedy != 1) {
                    eepromSize = FAST_READ_SIZE;
                }

                for (int num = 0; num < eepromSize; num++) {
                    // Read the next byte in the eeprom
                    uint8_t data = wiringPiI2CRead(eepromAddr);

                    // Then there must be a bit flip

                    // this is not double count safe and will report more errors than in reality!
                    // need to implement a hashmap or some other LUT 
                    if (data != 0xFF) {
                        eepromFailures[b * NUM_BANKS + e] = eepromFailures[b * NUM_BANKS + e] + 1; 
                    }
                }

                // get current time and calculate how long since we've started
                currTime = time(NULL); 
                elapsedTime = (int)difftime(currTime, startTime); 

                // Log to CSV file 
                fprintf(csv_file, "Elapsed Time %d, Bank %d, EEPROM %d, Failures %d\n", elapsedTime, bank, eeprom, failures);
                printf("Elapsed Time %d, Bank %d, EEPROM %d, Failures: %d\n", elapsedTime, bank, eeprom, failures);

                // End our transactions with this EEPROM
                close(eepromAddr);
            }
        }
    }

}

int main() {
    initGPIO();

    // Optionally initialize EEPROMs or not
    printf("In1t EEPROMS?");
    int choice;
    scanf("%d", &choice);

    /*
    TEST MODES:
    1. Greedy - Dump entire EEPROM each pass ; (much) slower but gets to later data in sizeable EEPROMs faster
    2. Speedy - Dump set blocks of data from EEPROMs ; faster but takes longer to get to further data in big EEPROMs
    */

    printf("Greedy (1) or Speedy? (else)"); 
    int greedy; 
    scanf("%d", &greedy);

    printf("Board number: ");
    int num; 
    scanf("%d", &num); 

    time_t ctime = time(NULL); 

    printf("%d", choice);

    if(choice == 1) {
        initEEPROMs();  
        printf("Initialization took %d seconds!\n", (int)difftime(time(NULL), ctime)); 
    }

    char filename[50];

    // maybe modify this to have board # then current hhmmss
    sprintf(filename, "board %d data.csv", num);
    FILE *csv_file = fopen(filename, "a");

    if (csv_file == NULL) {
        printf("Failed to open CSV file\n");
        return;
    }

    // Initialize failure array
    int* eepromFailures = malloc( (EEPROMS_PER_BANK * NUM_BANKS) * sizeof(eepromFailures) );

    // Continuously log data - no sleep needed since takes time to read EEPROMs
    while ( (int)difftime(time(NULL), ctime) <= RUNNING_TIME_SEC ) {
        logger(ctime, greedy, num, csv_file, eepromFailures);
    }

    // Close & Free all allocated stuff
    fclose(csv_file);
    free(eepromFailures); 

    printf("Completed and written to file.\n"); 

    return 0;
}
