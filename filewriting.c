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
#define NUM_BANKS 3
#define EEPROMS_PER_BANK 8

// How long to run the test - seconds
// default is 30 min -> 1800 seconds
#define RUNNING_TIME_SEC 2

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

void logger(time_t startTime, int greedy, int boardNum, FILE* csv_file, int** eepromFailures) {
    time_t currTime = 0;
    int elapsedTime = 0; 
    int eepromSize = 0; 

    // Initialize which addresses have failed

    // Go through all EEPROMs and banks 
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            eepromSize = getEEPROMSize(bank, eeprom);

            for (int num = 0; num < eepromSize; num++) {     
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

    // Initialize failure array
    int** eepromFailures = (int**) malloc( NUM_BANKS * sizeof(eepromFailures) );

    for(int b = 0; b < NUM_BANKS; b++) {
        eepromFailures[b] = (int*) malloc(EEPROMS_PER_BANK * sizeof(int));
    }

    int a, q; 

    // Go through b-th bank and set e-th EEPROM
    for(a = 0; a < NUM_BANKS; a++) {
        for(q = 0; q < EEPROMS_PER_BANK; q++) {
            eepromFailures[a][q] = 0; 
        }
    }

    // Continuously log data - no sleep needed since takes time to read EEPROMs
    while ( (int)difftime(time(NULL), ctime) <= RUNNING_TIME_SEC ) {
        logger(ctime, 0, num, csv_file, eepromFailures);
        
        break; 
    }

    // Close & Free all allocated stuff
    fclose(csv_file);

    for(int b = 0; b < NUM_BANKS; b++) {
        free(eepromFailures[b]);
    }
    free(eepromFailures); 

    printf("Completed and written to file.\n"); 

    return 0;
}
