#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <time.h>

#define BANK_SELECT_1 11
#define BANK_SELECT_2 13

#define EEPROM_ADDRESS 0x50 // base EEPROM I2C address

#define NUM_BANKS 3
#define EEPROMS_PER_BANK 8


// Return how many bytes are stored in a given bank,EEPROM combo
// this is a really stupid and inefficient piece of code 
// figure out a better way
int getEEPROMSize(int bank, int num) {
    int sizeKB;

    // how many kilobytes 
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
            sizeKB = 4; 

            break;
        default:
            return -1; 
            break;
    }

    // kilobytes -> bytes
    return 1000 * size; 
}

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
            digitalWrite(BANK_SELECT_1, LOW);
            digitalWrite(BANK_SELECT_2, HIGH);

            break;
        default:
            printf("Invalid bank number\n");
            
            break;
    }
}

void init_EEPROMs() {
    for (int bank = 0; bank < NUM_BANKS; bank++) {
        selectBank(bank);

        for (int eeprom = 0; eeprom < EEPROMS_PER_BANK; eeprom++) {
            int eepromAddr = wiringPiI2CSetup(EEPROM_ADDRESS + eeprom);

            if (eepromStatus == -1) {
                printf("Failed to initialize EEPROM %d in bank %d\n", eeprom, bank);
            } else {
                for (int addr = 0; addr < getEEPROMSize; addr++) {
                    if (wiringPiI2CWriteReg8(eepromAddr, addr, 0xFF) == -1) {
                        printf("Failed to write to EEPROM %d in bank %d\n", eeprom, bank);

                        break;
                    }
                }

                printf("Initialized EEPROM %d in bank %d\n", eeprom, bank);

                close(eepromAddr);
            }
        }
    }
}

void logger() {

    char filename[50];

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    sprintf(filename, "failures_%d-%02d-%02d_%02d-%02d-%02d.csv",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
            tm.tm_hour, tm.tm_min, tm.tm_sec);

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

                fprintf("Bank %d, EEPROM%d failed to respond.\n", bank, eeprom);
            } else {
                int failures = 0;

                for (int addr = 0; addr < getEEPROMSize(bank, eeprom); addr++) {
                    // Read the next byte in the eeprom
                    uint8_t data = wiringPiI2CRead(eepromAddr);

                    // Then there must be a bit flip
                    if (data != 0xFF) {
                        failures++;
                    }
                }

                fprintf(csv_file, "Bank %d, EEPROM %d, Failures: %d\n", bank, eeprom, failures);
                printf("Bank %d, EEPROM %d, Failures: %d\n", bank, eeprom, failures);

                close(eeprom_fd);
            }
        }
    }

    fclose(csv_file);
}

int main() {
    init_GPIO();
    init_EEPROMs();

    while (1) {
        logger();
        sleep(5); // Read and log every 5 seconds
    }

    return 0;
}
