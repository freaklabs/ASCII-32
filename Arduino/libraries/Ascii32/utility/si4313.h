#pragma once

#include <stdint.h>
#include <SPI.h>
#include "si4313_regs.h"

// For handling Arduino 1.0 compatibility and backwards compatibility
#if ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#define SCAN_STEP_SIZE 1        // step size in MHz to scan

typedef struct
{
    uint16_t freq;
    int16_t db;
} scan_t;

class SI4313
{
    uint8_t _csPin;
    uint8_t _sdnPin;

public:
    void begin(uint8_t csPin, uint8_t sdnPin);
    void writeReg(uint8_t addr, uint8_t data);
    uint8_t readReg(uint8_t addr);
    void changeFreq(uint16_t freq);
    uint8_t getRssi();
    int16_t getDB();
    uint32_t scan(uint16_t start, uint16_t stop, scan_t *data);

private:

};

extern SI4313 si4313;


