/*******************************************************************
    Copyright (C) 2013 FreakLabs
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
    3. Neither the name of the the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
    HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
    LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
    OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.

    Originally written by Christopher Wang aka Akiba.
    Please post support questions to the FreakLabs forum.

*******************************************************************/
/*!
    \file 
    \ingroup


*/
/**************************************************************************/
#include "si4313.h"

SI4313 si4313;

/**************************************************************************/
/*!

*/
/**************************************************************************/
void SI4313::begin(uint8_t csPin, uint8_t sdnPin)
{
    _csPin = csPin;
    _sdnPin = sdnPin;

    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);

    pinMode(_sdnPin, OUTPUT);
    digitalWrite(_sdnPin, LOW);

    SPI.begin();

    // clear radio interrupt regs
    readReg(SI4313_INTPSTAT1);
    readReg(SI4313_INTPSTAT2);
    
    // issue sw reset
    writeReg(SI4313_CONTROL1, 1<<BIT_SWRES);
    
    // wait for power on reset status flag to be set
    while (!(readReg(SI4313_INTPSTAT2) & (1<<BIT_IPOR)));
    
    // clear intp regs
    readReg(SI4313_INTPSTAT2);
    
    // wait for chip ready indicator
    while (!(readReg(SI4313_INTPSTAT2) & (1<<BIT_ICHIPRDY)));
    
    // clear intp regs
    readReg(SI4313_INTPSTAT2);
    
    // configure receiver based on excel spreadsheet
    // FSK, 20ppm tx/rx xtal tolerance, Fc = 905 mhz
    // bitrate 9.6 kbps, AFC disabled, freq deviation = 70 khz 
    writeReg(SI4313_FREQSEL,      0x75);
    writeReg(SI4313_FREQCARR1,    0x4B);
    writeReg(SI4313_FREQCARR0,    0x00);
    writeReg(SI4313_IFBW,         0xAE);
    writeReg(SI4313_CLKRATIO,     0x39);
    writeReg(SI4313_CLKOFFS2,     0x20);
    writeReg(SI4313_CLKOFFS1,     0x68);
    writeReg(SI4313_CLKOFFS0,     0xDC);
    writeReg(SI4313_CLKLPGAIN1,   0x00);
    writeReg(SI4313_CLKLPGAIN0,   0x10);
    writeReg(SI4313_AFCOVRD,      0x00);
    writeReg(SI4313_AFCCTRL,      0x0A);
    writeReg(SI4313_AFCLIMITER,   0x50);
    writeReg(SI4313_CLKOVERD,     0x03);
    writeReg(SI4313_AGCOVERD1,    0x60);
    writeReg(SI4313_PRECTRL,      0x20); // disable rssi adjustment
    
    // enable receiver
    writeReg(SI4313_MODCTRL2, 0x02); // enable FSK
    writeReg(SI4313_CONTROL1, 0x05); // enable receiver & xtal
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void SI4313::changeFreq(uint16_t freq)
{
    uint16_t fb, fc, tmp;

    if (freq < 240)
    {
        Serial.println("Frequencies below 240 MHz not supported.");
    }
    else if (freq < 480)
    {
        tmp = freq - 240;

        // write to freq band select reg
        fb = tmp / 10;
        fb |= 0x40;
        writeReg(0x75, fb);

        // calculate the nominal carrier freq & write to regs
        fc = (tmp % 10) * 0x1900;
        writeReg(0x76, fc >> 8);
        writeReg(0x75, fc);
    }
    else if (freq < 960)
    {
        tmp = freq - 480;

        // write to freq band select reg
        fb = (tmp/20);      // this is the actual fb we write
        fb |= 0x60;         // enable sbsel & hbsel
        writeReg(0x75, fb);

        // calculate the nominal carrier freq & write to regs
        fc = (tmp % 20) * 0xC80;    
        writeReg(0x76, fc >> 8);
        writeReg (0x77, fc);
    }
    else
    {
        Serial.println("Frequencies above 960 MHz are not support.");
    }

    // add short delay to allow pll to stabilize at new frequency
    delay(1);
}

/**************************************************************************/
/*!
    Scan the frequency spectrum from <start> frequency to <stop> frequency.
    <start> and <stop> are in MHz.
*/
/**************************************************************************/
uint32_t SI4313::scan(uint16_t start, uint16_t stop, scan_t *data)
{
    uint16_t i, j;
    scan_t *idx = data;

    if ((start < 240) | (stop > 960))
    {
        Serial.println("Frequency not supported.");
    }

    for (i=start; i<stop; i += SCAN_STEP_SIZE)
    {
        int16_t val;
        
        // change frequency and get signal strength in dB
        changeFreq(i);

//        printf("%02X, %02X, %02X, %03d\n", readReg(0x75), readReg(0x76), readReg(0x77), readReg(0x26));

        val = getDB();

        // write to data struct
        idx->freq = i;
        idx->db = val;
        idx++;
    }

    // return the number of data points from the scan
    return idx - data;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t SI4313::getRssi()
{
    return readReg(SI4313_RSSI);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
int16_t SI4313::getDB()
{
    uint16_t rssi = readReg(SI4313_RSSI);
    int dB;

    // these values are very roughly eyeballed and linearized from the
    // graph in the SI4313-B1 datasheet v1.0, Figure 12.
    dB = map(rssi, 8, 208, -118, -20);
    return dB;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t SI4313::readReg(uint8_t addr)
{
    uint8_t val;
    addr &= ~(1<<7);  // set bit 7 low for read
    
    cli();
    digitalWrite(_csPin, LOW);
    val = SPI.transfer(addr); // send address
    val = SPI.transfer(0);  // receive data
    digitalWrite(_csPin, HIGH);
    sei();
    
    return val;
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void SI4313::writeReg(uint8_t addr, uint8_t data)
{
    addr |= (1<<7);  // set bit 7 high for write
    
    cli();
    digitalWrite(_csPin, LOW);
    SPI.transfer(addr); // send address
    SPI.transfer(data);  // receive data
    digitalWrite(_csPin, HIGH);
    sei();
}
