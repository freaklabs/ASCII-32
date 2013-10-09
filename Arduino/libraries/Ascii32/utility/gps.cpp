/*
   Simple library to read Geolocation info from a hardware Serial GPS module

   Copyright (c) 2011, Robin Scheibler aka FakuFaku, Christopher Wang aka Akiba
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
   DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
   ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "GPS.h"
#include <limits.h>

/* 'private' methods declarations */
void parse_line_rmc(char **token);  // parse RMC sentence (from NMEA protocol)
void parse_line_gga(char **token);  // parse GGA sentence (from NMEA protocol)
void parse_datetime();              // parse date and time into correct data struct

/* state variables */
byte _updating;
HardwareSerial* _serial;      // The serial port used by GPS
gps_t _gps_data;              // GPS data structure
char *_line;                  // buffer to receive new line from serial
byte _index;                  // current character index
unsigned long _rx_time;       // Timestamp of received time

// Constructor
// we need to provide the line buffer for RAM efficiency
void gps_init(HardwareSerial *serial, char *line)
{
  _serial = serial; // Hardware Serial connection, supposed to be initialized
  _index = 0;            // character counter initialization
  _updating = 1;    
  _line = line;     // the character array for serial com buffering

  // set initial gps data to all zero
  memset((void *)&_gps_data, 0, sizeof(gps_t));
}

// Availability indicator
int gps_available()
{
  // location available when not updating
  return !_updating;
}

// Return the millisecond microprocessor time of when data was received
unsigned long gps_age()
{
  unsigned long now = millis();
  
  if (now >= _rx_time)
    return now - _rx_time;
  else
    return (ULONG_MAX - _rx_time + now);
}

// Update routine
void gps_update()
{

  char *tok[SYM_SZ] = {0};
  int j = 0;

  while (_serial->available())
  {
    if (_index < LINE_SZ)
    {
      _line[_index] = _serial->read();
      if (_line[_index] == '\n')
      {
        // set timestamp
        _rx_time = millis();

        // terminate string with null character
        _line[_index+1] = '\0';
        
        // reset character counter
        _index=0;

        // dump the raw GPS data
        //Serial.print(_line);

        // verify the line is valid NMEA sentence
        int L = strlen(_line);
        int valid_sentence = gps_verify_NMEA_sentence(_line, L-2); // -2 is for \r\n

        // if the sentence is a valide sentence, try to parse it
        if (valid_sentence)
        {
          // tokenize line
          char *string;

          string = _line;

          while ((tok[j++] = strsep(&string, ",")) != NULL)
            ;

          // reset the index variable for next run
          j = 0;

          // parse line and date/time and update gps struct
          if (strcmp(tok[0], "$GPRMC") == 0)
          {
            parse_line_rmc(tok);
            parse_datetime();
          }
          else if (strcmp(tok[0], "$GPGGA") == 0)
          {
             parse_line_gga(tok);
          }
        }
        
        // clear the flag so that we know its okay to read the data
        _updating = 0;
      }
      else
      {
        // still taking in data. increment index and make sure the updating flag is set
        _updating = 1;
        _index++;
      }
    }
    else
    {
      _index = 0;
    }
  }
  
}

// Compute checksum of input array
char gps_checksum(char *s, int N)
{
  int i = 0;
  char chk = s[0];

  for (i=1 ; i < N ; i++)
    chk ^= s[i];

  return chk;
}

// compare checksum of string str of length L
// with hex checksum contained in length 2 string chk
int gps_checksum_match(char *str, int L, char *chk)
{
  char ch1, ch2;

  // compute local checksum
  char chk_str = gps_checksum(str, L);

  // transform hex string to char
  // first digit
  if (chk[0] > '9')
    ch1 = chk[0] - 'A' + 10;
  else
    ch1 = chk[0] - '0';
  // second digit
  if (chk[1] > '9')
    ch2 = chk[1] - 'A' + 10;
  else
    ch2 = chk[1]-'0';
  char chk_num = ch1*16 + ch2;

  // return result of matching
  return (chk_str == chk_num);
}

// verify formating and checksum of NMEA-style sentence of length L
int gps_verify_NMEA_sentence(char *sentence, int L)
{
  // basice property, starts with dollar, ends with star
  if (sentence[0] != '$' || sentence[L-3] != '*')
    return false;

  // verify checksum is hex string
  if ( ! ( (sentence[L-2] >= '0' && sentence[L-2] <= '9') || (sentence[L-2] <= 'F' && sentence[L-2] >= 'A') ) )
    return false;
  if ( ! ( (sentence[L-1] >= '0' && sentence[L-1] <= '9') || (sentence[L-1] <= 'F' && sentence[L-1] >= 'A') ) )
    return false;

  // if we reach that part, just return checksum matching result
  return gps_checksum_match(sentence+1, L-4, sentence+L-2);
}

// Return reference to GPS data structure
gps_t *gps_getData() 
{ 
  return &_gps_data; 
}

// Parse RMC sentence
void parse_line_rmc(char **token)
{
  // put all that to zero
  memset(&_gps_data.utc,        0,     UTC_SZ-1);
  memset(&_gps_data.status,     0,     DEFAULT_SZ-1);
  memset(&_gps_data.lat,        0,     LAT_SZ-1);
  memset(&_gps_data.lat_hem,    0,     DEFAULT_SZ-1);
  memset(&_gps_data.lon,        0,     LON_SZ-1);
  memset(&_gps_data.lon_hem,    0,     DEFAULT_SZ-1);
  memset(&_gps_data.speed,      0,     SPD_SZ-1);
  memset(&_gps_data.course,     0,     CRS_SZ-1);
  memset(&_gps_data.date,       0,     DATE_SZ-1);
  memset(&_gps_data.checksum,   0,     CKSUM_SZ-1);

  // now if token is not NULL, copy into the GPS data structure
  if (token[1][0] != 0)
    memcpy(&_gps_data.utc,        token[1],     UTC_SZ-1);
  if (token[2][0] != 0)
    memcpy(&_gps_data.status,     token[2],     DEFAULT_SZ-1);
  if (token[3][0] != 0)
    memcpy(&_gps_data.lat,        token[3],     LAT_SZ-1);
  if (token[4][0] != 0)
    memcpy(&_gps_data.lat_hem,    token[4],     DEFAULT_SZ-1);
  if (token[5][0] != 0)
    memcpy(&_gps_data.lon,        token[5],     LON_SZ-1);
  if (token[6][0] != 0)
    memcpy(&_gps_data.lon_hem,    token[6],     DEFAULT_SZ-1);
  if (token[7][0] != 0)
    memcpy(&_gps_data.speed,      token[7],     SPD_SZ-1);
  if (token[8][0] != 0)
    memcpy(&_gps_data.course,     token[8],     CRS_SZ-1);
  if (token[9][0] != 0)
    memcpy(&_gps_data.date,       token[9],     DATE_SZ-1);
  if (token[10][0] != 0)
    memcpy(&_gps_data.checksum,   token[10],    CKSUM_SZ-1);
}

// Parse GGA sentence
void parse_line_gga(char **token)
{
    // set structure to zero
    memset(&_gps_data.quality,    0, DEFAULT_SZ-1);
    memset(&_gps_data.num_sat,    0, NUM_SAT_SZ-1);
    memset(&_gps_data.precision,  0, PRECISION_SZ-1);
    memset(&_gps_data.altitude,   0, ALTITUDE_SZ-1);
    
    // now copy data if present
    if (token[6][0] != 0)
      memcpy(&_gps_data.quality,    token[6], DEFAULT_SZ-1);
    if (token[7][0] != 0)
      memcpy(&_gps_data.num_sat,    token[7], NUM_SAT_SZ-1);
    if (token[8][0] != 0)
      memcpy(&_gps_data.precision,  token[8], PRECISION_SZ-1);
    if (token[9][0] != 0)
      memcpy(&_gps_data.altitude,   token[9], ALTITUDE_SZ-1);
}

// Parse date and time from GPS and input in structure
void parse_datetime()
{
    memset(&_gps_data.datetime, 0, sizeof(date_time_t));

    // parse UTC time
    memcpy(_gps_data.datetime.hour, &_gps_data.utc[0], 2);
    memcpy(_gps_data.datetime.minute, &_gps_data.utc[2], 2);
    memcpy(_gps_data.datetime.second, &_gps_data.utc[4], 2);

    // parse UTC calendar
    memcpy(_gps_data.datetime.day, &_gps_data.date[0], 2);
    memcpy(_gps_data.datetime.month, &_gps_data.date[2], 2);
    memcpy(_gps_data.datetime.year, &_gps_data.date[4], 2);
}

// Gets next gps line, or up to N characters
// This routine is blocking
// returns 1 for success, 0 for failure (timeout or N reached)
int gps_get_next_line(char *str, int N, int timeout)
{
  int i = 0;
  unsigned long now = millis();
  while (1)
  {
    while (_serial->available())
    {
      str[i] = _serial->read();
      if (i == N || str[i] == '\n')
        goto out;
      i++;
    }
    if (millis() - now > timeout)
      break;
  }
out:
  // terminate string
  str[i+1] = 0;
  // return error if did not terminate correctly
  if (i == N || millis() - now >= timeout)
    return 0;
  else
    return 1;
}

void gps_diagnostics()
{

#define TIMEOUT 1000
#define MAX_RETRY 5
#define TMP_BUF_LEN 20

  char msg[20];
  char tmp[TMP_BUF_LEN+1];
  char mtk_init_cmp[TMP_BUF_LEN+1];
  char mtk_sys_cmp[TMP_BUF_LEN+1];
  int mtk_init_stat = 0;
  int mtk_sys_stat = 0;
  int r = 0;
  int n;

  // Testing for System Message
  strcpy_P(mtk_init_cmp, PSTR(MTK_INIT_MSG));
  strcpy_P(mtk_sys_cmp, PSTR(MTK_SYS_MSG));

  // flush GPS serial
  while (_serial->available())
    _serial->read();

  // hot restart GPS to catch init message
  strcpy_P(tmp, PSTR(MTK_HOT_RESTART));
  _serial->println(tmp);

  while (r < MAX_RETRY && (mtk_init_stat == 0 || mtk_sys_stat == 0))
  {
    // update number of time we ran the loop
    r++;

    // get next GPS line
    if (!gps_get_next_line(tmp, TMP_BUF_LEN, TIMEOUT))
      continue;

    // check there is a '$' at beginning
    n = 0;
    while (n < TMP_BUF_LEN && tmp[n] != '$')
      n++;
    if (n == TMP_BUF_LEN)
      continue;

    // if INIT not confirmed yet, check for it
    if (!mtk_init_stat)
      if (strncmp(mtk_init_cmp, tmp+n, strlen(mtk_init_cmp)) == 0)
      {
        mtk_init_stat = 1;
        continue;
      }

    // if SYS not confirmed yet, check for it
    if (!mtk_sys_stat)
      if (strncmp(mtk_sys_cmp, tmp+n, strlen(mtk_sys_cmp)) == 0)
        mtk_sys_stat = 1;
  }

  strcpy_P(msg, PSTR("GPS type MTK,"));
  Serial.print(msg);
  if (mtk_init_stat)
  {
    strcpy_P(msg, PSTR("yes"));
    Serial.println(msg);
  }
  else
  {
    strcpy_P(msg, PSTR("no"));
    Serial.println(msg);
  }

  strcpy_P(msg, PSTR("GPS system startup,"));
  Serial.print(msg);
  if (mtk_sys_stat)
  {
    strcpy_P(msg, PSTR("yes"));
    Serial.println(msg);
  }
  else
  {
    strcpy_P(msg, PSTR("no"));
    Serial.println(msg);
  }

}

/**************************************************************************/
/*!

*/
/**************************************************************************/
void gps_clear_flag()
{
  _updating = 1;
}



