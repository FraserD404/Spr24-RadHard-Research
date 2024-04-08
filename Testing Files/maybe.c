/*

Test CSV Storage File

April 5, 2024
Author: Fraser Dougall 
Email: fdougall@purdue.edu

*/

// Libraries
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

// EEPROM Banking Information
#define EEPROM_ADDRESS 0x50 // base EEPROM I2C address
#define NUM_BANKS 2         // three exist physically but not working + redundant
#define EEPROMS_PER_BANK 8  // eep/bank
#define MAX_EEPROM_SIZE 512000 // maximum size we have

// How long to run the test - seconds
// default is 30 min -> 1800 seconds
#define RUNNING_TIME_SEC 2

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//typedef struct {
  //  uint8_t* addresses;
//} eepromMemArray; 

typedef struct {
    int size;
    int failures; 
    int i2cAddr; 
    uint8_t* mems;
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
                sizeKB = 256;
            } else {
                sizeKB = 64; 
            } 

            break;
        default:
            return -1; 
            break;
    }

    // kilobytes -> bytes
    return 1000 * sizeKB; 
}

// pass into initialization function
void initEEPROMs(allEEPROMs* population) {
    // stores current eeprom
    EEPROM* current = (EEPROM*) malloc(sizeof(current)); 

    for (int bank = 0; bank < NUM_BANKS; bank++) {
       // selectBank(bank);

        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {

            // grab current EEPROM from array
            current = &((population->all)[bank * EEPROMS_PER_BANK  + eeprom]);
           // current->i2cAddr = wiringPiI2CSetup(EEPROM_ADDRESS + eeprom);
            current->i2cAddr = 1; 
	    
            if (current->i2cAddr < 0) {
                printf("Failed to initialize EEPROM %d in bank %d\n", eeprom, bank);
            } else {
		        current->size = getEEPROMSize(bank, eeprom);
		        bool init = true;

                // Linearly initialize all locations in EEPROM ; wish this was faster but impossible for better than O(n)
                /*
                for (int num = 0; num < current->size; num++) {
                    if (wiringPiI2CWrite(current->i2cAddr, 0xFF) == -1) {
                        printf("Failed to write to EEPROM %d in bank %d\n", eeprom, bank);
                        init = false;

                        break;
                    } 		
                }
            */

                if(init) {
                    // malloc our saved addresses array
                    current->mems = calloc(current->size, sizeof(current->mems)); 
                    //(current->mems)->addresses = malloc(sizeof((current->mems)->addresses) * current->size);
		   current->failures = 0;
                    //for(int l = 0; l < current->size; l++) {
                      //  (current->mems)[l] = 0; 
                   // }
                    printf("Initialized EEPROM %d in bank %d\n", eeprom, bank);
                }

                //close(current->i2cAddr);
            }
        }
    }

    // free our temporary storage
//    free(current); 
}

void logger(time_t startTime, int greedy, int boardNum, FILE* csv_file, allEEPROMs* population) {
    time_t currTime = 0;
    int elapsedTime = 0; 
    int eepromSize = 0; 

    EEPROM* current = (EEPROM*) malloc(sizeof(current)); 

    // Go through all EEPROMs and banks 
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            // Get current EEPROM from total population
            current = &((population->all)[bank * EEPROMS_PER_BANK  + eeprom]);

            for (int byte = 0; byte < current->size; byte++) {     
                //uint8_t data = wiringPiI2CRead(eepromAddr);

                uint8_t data = 0xAF; 

                if(data != 0xFF){
                    // check to see if we've looked at this before
                    // yay O(1) access but rip space complexity :( 
                    if( ((current->mems)[byte] != 1)) {
                        current->failures =  current->failures + 1; 
                        (current->mems)[byte] = 1;
                    } // otherwise we do not want to double count failure
                }
            }
            // get current time and calculate how long since we've started
            currTime = time(NULL); 
            elapsedTime = difftime(currTime, startTime); 

            // Log to CSV file 
            fprintf(csv_file, "%d, %d, %d, %d\n", elapsedTime, bank, eeprom, current->failures);
        }
    }

   // free(current); 
}
/*
void logger(time_t startTime, int greedy, int boardNum, FILE* csv_file, int** eepromFailures) {
    time_t currTime = 0;
    int elapsedTime = 0; 
    int eepromSize = 0; 

    // Go through all EEPROMs and banks 
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            eepromSize = getEEPROMSize(bank, eeprom);

            for (int byte = 0; byte < eepromSize; byte++) {     
                // check if already seen

                while(potential != NULL) {
                    if(potential->byte == byte) {
                        seen = True; 
                        break;
                    }
                }
                eepromFailures[bank][eeprom] = eepromFailures[bank][eeprom] + 1; 
            }

            // get current time and calculate how long since we've started
            currTime = time(NULL); 
            elapsedTime = difftime(currTime, startTime); 

            // Log to CSV file 
            fprintf(csv_file, "%d, %d, %d, %d\n", elapsedTime, bank, eeprom, eepromFailures[bank][eeprom]);
            //printf("Elapsed Time %0.2f, Bank %d, EEPROM %d, Failures: %d\n", elapsedTime, bank, eeprom, eepromFailures[b * NUM_BANKS + e]);
        }
    }
}
*/


int main() {
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

    // malloc our entire EEPROM handler
    allEEPROMs* population = (allEEPROMs*) malloc(sizeof(*population));
    
    // malloc storage of all our eeprom structs
    population->all = (EEPROM*) malloc(totalEEPROMs * sizeof(population->all));

    initEEPROMs(population);

    // Continuously log data - no sleep needed since takes time to read EEPROMs
    while ( (int)difftime(time(NULL), ctime) <= RUNNING_TIME_SEC ) {
        logger(ctime, 0, num, csv_file, population);
        
        break; 
    }

    // Close & Free all allocated stuff
    fclose(csv_file);

    // still need to free all EEPROM elements :) 
    printf("Completed and written to file.\n"); 

    return 0;
}
