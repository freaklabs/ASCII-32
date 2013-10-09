/*******************************************************************
    Copyright (C) 2012 FreakLabs
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
#include "ascii32.h"

ASCII32 ascii32;

/**************************************************************************/
/*!

*/
/**************************************************************************/
void ASCII32::begin(uint8_t radioCsPin, uint8_t radioSdnPin, HardwareSerial *serial, char *gpsBuf)
{
    // init radio
    si4313.begin(radioCsPin, radioSdnPin);

    // init gps
    gps_init(serial, gpsBuf);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void ASCII32::radioWriteReg(uint8_t addr, uint8_t data)
{
    si4313.writeReg(addr, data);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t ASCII32::radioReadReg(uint8_t addr)
{
    return si4313.readReg(addr);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void ASCII32::radioChangeFreq(uint16_t freq)
{
    si4313.changeFreq(freq);
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
int16_t ASCII32::radioGetDB()
{
    return si4313.getDB();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint8_t ASCII32::radioGetRssi()
{
    return si4313.getRssi();
}

/**************************************************************************/
/*!

*/
/**************************************************************************/
uint32_t ASCII32::radioScan(uint16_t start, uint16_t stop, scan_t *data)
{
    return si4313.scan(start, stop, data);
}
