#pragma once

// For handling Arduino 1.0 compatibility and backwards compatibility
#if ARDUINO >= 100
    #include "Arduino.h"
#else
    #include "WProgram.h"
#endif

#include "utility/si4313.h"
#include "utility/gps.h"

class ASCII32
{
public:
    void begin(uint8_t radioCsPin, uint8_t radioSdnPin, HardwareSerial *serial, char *gpsBuf);
    int gpsAvail();
    void gpsUpdate();
    void radioWriteReg(uint8_t addr, uint8_t data);
    uint8_t radioReadReg(uint8_t addr);
    void radioChangeFreq(uint16_t freq);
    uint8_t radioGetRssi();
    int16_t radioGetDB();
    uint32_t radioScan(uint16_t start, uint16_t stop, scan_t *data);

private:

};

extern ASCII32 ascii32;


