/*

C Code for EEPROM Control for Research

April 4, 2024

Author: Fraser Dougall 
Email: fdougall@purdue.edu

*/

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
                sizeKB = 16; 
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

	    //if(bank == 2 && eeprom > 3) { 
	//	    break;
	  //  }

            if (eepromAddr < 0) {
                printf("Failed to initialize EEPROM %d in bank %d\n", eeprom, bank);
            } else {
		        int sizeBytes = getEEPROMSize(bank, eeprom);
		        bool init = true;

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

void logger(time_t startTime) {
    char filename[50];

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    //time_t startTime = time(NULL); 
    time_t currTime = 0;
    int elapsedTime = 0; 

    sprintf(filename, "failures_%d-%02d-%02d_%02d-%02d-%02d.csv",tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,   tm.tm_hour, tm.tm_min, tm.tm_sec);

    FILE *csv_file = fopen(filename, "a");

    if (csv_file == NULL) {
        printf("Failed to open CSV file\n");
        return;
    }

    for (int bank = 0; bank < NUM_BANKS; bank++) {
        selectBank(bank);

        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            int eepromAddr = wiringPiI2CSetup(EEPROM_ADDRESS + eeprom);

            if (eepromAddr == -1) {
                printf("Failed to initialize EEPROM %d in bank %d\n", eeprom, bank);
                fprintf("Bank %d, EEPROM%d, Failures: -1\n", bank, eeprom);
            } else {
                int failures = 0;
		int size = getEEPROMSize(bank, eeprom);

                // 512 done in 64k blocks otherwise takes too long
                if(size/1000 == 512) {
                    size = 64000;
                }

                for (int num = 0; num < size; num++) {
                    // Read the next byte in the eeprom
                    uint8_t data = wiringPiI2CRead(eepromAddr);

                    // Then there must be a bit flip
                    if (data != 0xFF) {
                        failures++;
                    }
                }

                currTime = time(NULL); 
                elapsedTime = (int)difftime(currTime, startTime); 

                fprintf(csv_file, "Elapsed Time %d, Bank %d, EEPROM %d, Failures %d\n", elapsedTime, bank, eeprom, failures);
                printf("Elapsed Time %d, Bank %d, EEPROM %d, Failures: %d\n", elapsedTime, bank, eeprom, failures);

                close(eepromAddr);
            }
        }
    }

    fclose(csv_file);
}

int main() {
    initGPIO();

    printf("In1t EEPROMS?");

    int choice;

    scanf("%d", &choice);

    time_t ctime = time(NULL); 

    printf("%d", choice);

    if(choice == 1) {
        initEEPROMs();  
        printf("Initialization took %d seconds!\n", (int)difftime(time(NULL), ctime)); 
    }

    // Continuously log data
    while (1) {
        logger(ctime);

        //sleep(5); // Read and log every 5 seconds
    }

    return 0;
}
