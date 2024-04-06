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
#include <stdlib.h>

// Selector Pins
#define BANK_SELECT_1 0
#define BANK_SELECT_2 1

// EEPROM Banking Information
#define EEPROM_ADDRESS 0x50 // base EEPROM I2C address
#define NUM_BANKS 2
#define EEPROMS_PER_BANK 8
#define MAX_EEPROM_SIZE 512000 // maximum size we have

// How long to run the test - seconds
// default is 30 min -> 1800 seconds
#define RUNNING_TIME_SEC 1800

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// technically this doesn't need to be a struck....
//typedef struct {
   // uint8_t* addresses;
//} eepromMemArray; 

typedef struct {
    int size;             // size in bytes of eeprom
    int failures;         // how many times has this EEPROM failed
    int i2cAddr;          // where on the i2c bus is it
    uint8_t* mems; // addresses that we know have failed
} EEPROM; 

typedef struct {
    EEPROM* all;
} allEEPROMs; 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// global variables :(
int totalEEPROMs = NUM_BANKS * EEPROMS_PER_BANK; 

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
void initEEPROMs(allEEPROMs* population) {
    // stores current eeprom
    EEPROM* current = (EEPROM*) malloc(sizeof(EEPROM)); 

    for (int bank = 0; bank < NUM_BANKS; bank++) {
        selectBank(bank);

        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            // grab current EEPROM from array
            current = &((population->all)[bank * EEPROMS_PER_BANK  + eeprom]);
            current->i2cAddr = wiringPiI2CSetup(EEPROM_ADDRESS + eeprom);

            if (current->i2cAddr < 0) {
                printf("Failed to initialize EEPROM %d in bank %d\n", eeprom, bank);
            } else {
		        current->size = getEEPROMSize(bank, eeprom);
		        bool init = true;

                // Linearly initialize all locations in EEPROM ; wish this was faster but impossible for better than O(n)
                for (int num = 0; num < current->size; num++) {
                    if (wiringPiI2CWrite(current->i2cAddr, 0xFF) == -1) {
                        printf("Failed to write to EEPROM %d in bank %d\n", eeprom, bank);
                        init = false;

                        break;
                    } 		
                }

                if(init) {
                    // malloc our saved addresses array
                   current->mems = calloc(current->size, sizeof(current->mems));

                    printf("Initialized EEPROM %d in bank %d\n", eeprom, bank);
                }

                close(current->i2cAddr);
            }
        }
    }

    // free our temporary storage
   // free(current); 
}

void logger(time_t startTime, int greedy, int boardNum, FILE* csv_file, allEEPROMs* population) {
    time_t currTime = 0;
    int elapsedTime = 0; 
    int eepromSize = 0; 

    EEPROM* current = (EEPROM*) malloc(sizeof(EEPROM)); 

    // Go through all EEPROMs and banks 
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        selectBank(bank); 

        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            // Get current EEPROM from total population
            current = &((population->all)[bank * EEPROMS_PER_BANK  + eeprom]);
            current->i2cAddr = wiringPiI2CSetup(EEPROM_ADDRESS + eeprom); 

	    //printf("yeet %d", current->size);
            // make sure our EEPROM actually exists lol 
            if(current->i2cAddr >= 0) {

                /*
                do 512 in 128k blocks so more data points :) 
                */
                for (int byte = 0; byte < current->size; byte++) {     
                    int data = wiringPiI2CRead(current->i2cAddr);

                    if(data != 0xFF && data > 0){
                        // check to see if we've looked at this before
                        // yay O(1) access but rip space complexity :( 
                        if( ((current->mems))[byte] != 1) {
                            current->failures =  current->failures + 1; 
                            (current->mems)[byte] = 1;
                         } /// otherwise we do not want to double count failure
                    } 
                }
                // get current time and calculate how long since we've started
                currTime = time(NULL); 
                elapsedTime = difftime(currTime, startTime); 

                // Log to CSV file 
                fprintf(csv_file, "%d, %d, %d, %d\n", elapsedTime, bank, eeprom, current->failures);
                
                close(current->i2cAddr); 
            } else {
                current->failures = -1337; // since it doesn't exist 
            }
        }
    }


   // free(current); 
}


int main() {
    initGPIO();


    // illusion of choice ^-^ 
    // Optionally initialize EEPROMs or not
    //printf("In1t EEPROMS?");
    //int choice;
   // scanf("%d", &choice);

    printf("Board number: ");
    int num; 
    scanf("%d", &num); 

    time_t ctime = time(NULL); 
    
    char filename[50];

    sprintf(filename, "board %d data.csv", num);
    FILE *csv_file = fopen(filename, "a");

    if (csv_file == NULL) {
        printf("Failed to open CSV file\n");
        return -1;
    }

    fprintf(csv_file, "Elapsed Time, Bank, EEPROM, Failures\n");

    fclose(csv_file);
    fopen(filename, "a");

    // malloc our entire EEPROM handler
     allEEPROMs* population = (allEEPROMs*) malloc(sizeof(*population));
    
    // malloc storage of all our eeprom structs
    population->all = (EEPROM*) calloc(totalEEPROMs, sizeof(EEPROM));

    // Initialize everything
    initEEPROMs(population);

    // reset
    ctime = time(NULL); 

    printf("it's logging time\n");

    // Continuously log data - no sleep needed since takes time to read EEPROMs
    while ( (int)(difftime(time(NULL), ctime)) <= RUNNING_TIME_SEC ) {
        logger(ctime, 0, num, csv_file, population);
	fclose(csv_file); // update file
	fopen(filename, "a");
    }

    // Close & Free all allocated stuff
    fclose(csv_file);

    // still need to free all EEPROM elements :) 

    // WHAT THE HECK IS A GARBAGE COLLECTOR RAHHHH
    // I AM THE GARBAGE COLLECTOR
    // THEY CALL ME THE TRASH MAN
    // I COLLECT TRASH

    printf("Completed and written to file.\n"); 

    return 0;
}
