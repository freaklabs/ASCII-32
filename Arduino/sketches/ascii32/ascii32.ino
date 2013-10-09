#include <avr/wdt.h>
#include <chibi.h>
#include <SPI.h>
#include <SdFat.h>
#include "ascii32.h"

#define DATECODE "2013-05-07"

static char line[100];

// this is for printf
static FILE uartout = {0};  

int sdCsPin = 10;
int radioCsPin = 30;
int radioSdnPin = 31;

SdFat sd;
SdFile myFile;

/*********************************************************************/
//
//
/*********************************************************************/
void setup()
{
  // fill in the UART file descriptor with pointer to writer.
  fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  
  // The uart is the standard output device STDOUT.
  stdout = &uartout ;
  
  // init radio chip select
  pinMode(radioCsPin, OUTPUT);
  digitalWrite(radioCsPin, HIGH);
  
  // init radio shutdown pin
  pinMode(radioSdnPin, OUTPUT);
  digitalWrite(radioSdnPin, LOW);
  
  chibiCmdInit(57600); 
  Serial1.begin(9600); // gps
  
  chibiCmdAdd("rd", cmdRadioRead);
  chibiCmdAdd("wr", cmdRadioWrite);
  chibiCmdAdd("test", cmdTest);
  chibiCmdAdd("scan", cmdScan);
  
  // initialize SPI:
  SPI.begin(); 
  
  //////////////////////////////////////////
  // begin initialization display
  //////////////////////////////////////////
  radioInit();
  
  welcomeMsg();
}

/*********************************************************************/
//
//
/*********************************************************************/
void loop()
{
  chibiCmdPoll();
  
  // print gps data if available
  if (Serial1.available())
  {
    char c = Serial1.read();
    Serial.print(c);
  }
}

/*********************************************************************/
//
//
/*********************************************************************/
void welcomeMsg()
{ 
  uint8_t type, ver;
  
  printf("ASCII-32 Geotagging Sub-1 GHz Spectrum Scanner\n");
  printf("Last Updated: %s\n\n", DATECODE);
  
  // check radio device type
  type = radioRead(SI4313_DEVTYPE);
  if (type == SI4313_TYPE)
  {
    printf("Silicon Labs SI4313 radio receiver detected.\n");
    
    // check radio version  
    ver = radioRead(SI4313_VERSION);
    if (ver == SI4313_B1VER)
    {
      printf("Hardware version: B1\n");
    }
    else
    {
      printf("Unknown hardware version.\n");
    }
  }
  else
  {
    printf("Unknown radio receiver detected.\n");
  }  
  
  // init the SD card
  if (!sd.begin(sdCsPin)) 
  {
    Serial.println("Card failed, or not present");
    sd.initErrorHalt();
    return;
  }
  printf("SD Card detected and initialized.\n");
}

/*********************************************************************/
//
//
/*********************************************************************/
void radioInit()
{
  uint8_t intpStatus[2];
  
  // clear radio interrupt regs
  radioRead(SI4313_INTPSTAT1);
  radioRead(SI4313_INTPSTAT2);
  
  // issue sw reset
  radioWrite(SI4313_CONTROL1, 1<<BIT_SWRES);
  
  // wait for power on reset status flag to be set
  while (!(radioRead(SI4313_INTPSTAT2) & (1<<BIT_IPOR)));
  
  // clear intp regs
  radioRead(SI4313_INTPSTAT2);
  
  // wait for chip ready indicator
  while (!(radioRead(SI4313_INTPSTAT2) & (1<<BIT_ICHIPRDY)));
  
  // clear intp regs
  radioRead(SI4313_INTPSTAT2);
  
  // configure receiver based on excel spreadsheet
  // FSK, 20ppm tx/rx xtal tolerance, Fc = 905 mhz
  // bitrate 9.6 kbps, AFC disabled, freq deviation = 70 khz 
  radioWrite(SI4313_FREQSEL,      0x75);
  radioWrite(SI4313_FREQCARR1,    0x4B);
  radioWrite(SI4313_FREQCARR0,    0x00);
  radioWrite(SI4313_IFBW,         0xAE);
  radioWrite(SI4313_CLKRATIO,     0x39);
  radioWrite(SI4313_CLKOFFS2,     0x20);
  radioWrite(SI4313_CLKOFFS1,     0x68);
  radioWrite(SI4313_CLKOFFS0,     0xDC);
  radioWrite(SI4313_CLKLPGAIN1,   0x00);
  radioWrite(SI4313_CLKLPGAIN0,   0x10);
  radioWrite(SI4313_AFCOVRD,      0x00);
  radioWrite(SI4313_AFCCTRL,      0x0A);
  radioWrite(SI4313_AFCLIMITER,   0x50);
  radioWrite(SI4313_CLKOVERD,     0x03);
  radioWrite(SI4313_AGCOVERD1,    0x60);
  radioWrite(SI4313_PRECTRL,      0x20); // disable rssi adjustment
  
  // enable receiver
  radioWrite(SI4313_MODCTRL2, 0x02); // enable FSK
  radioWrite(SI4313_CONTROL1, 0x05); // enable receiver & xtal
}

/*********************************************************************/
//
//
/*********************************************************************/
uint8_t radioRead(uint8_t addr)
{
  uint8_t val;
  addr &= ~(1<<7);  // set bit 7 low for read
  
  cli();
  digitalWrite(radioCsPin, LOW);
  val = SPI.transfer(addr); // send address
  val = SPI.transfer(0);  // receive data
  digitalWrite(radioCsPin, HIGH);
  sei();
  
  return val;
}

/*********************************************************************/
//
//
/*********************************************************************/
void radioWrite(uint8_t addr, uint8_t data)
{
  addr |= (1<<7);  // set bit 7 high for write
  
  cli();
  digitalWrite(radioCsPin, LOW);
  SPI.transfer(addr); // send address
  SPI.transfer(data);  // receive data
  digitalWrite(radioCsPin, HIGH);
  sei();
}

/*********************************************************************/
//
//
/*********************************************************************/
void cmdRadioRead(int arg_cnt, char **args)
{
  uint8_t val, addr;
  
  addr = chibiCmdStr2Num(args[1], 16);
  val = radioRead(addr);
  printf("Addr %02X = %02X.\n", addr, val);
}

/*********************************************************************/
//
//
/*********************************************************************/
void cmdRadioWrite(int arg_cnt, char **args)
{
  uint8_t val, addr;
  
  addr = chibiCmdStr2Num(args[1], 16);
  val = chibiCmdStr2Num(args[2], 16);
  printf("Addr %02X = %02X.\n", addr, val);
}

/*********************************************************************/
//
//
/*********************************************************************/
void cmdTest(int arg_cnt, char **args)
{
  uint16_t freq, temp, fb, fc;
  uint8_t val;
  
  freq = chibiCmdStr2Num(args[1], 10);
  temp = freq - 480;
  fb = temp/20;  
  fb |= 0x60;
  
  fc = temp % 20;
  fc *= 0xC80;
  
  radioWrite(0x75, fb);
  
  val = fc >> 8;
  radioWrite(0x76, val);
  
  radioWrite(0x77, fc);
  
  val = radioRead(0x26);
  printf("Rssi: %d\n", val);
}

/*********************************************************************/
//
//
/*********************************************************************/
void cmdScan(int arg_cnt, char **args)
{
  uint16_t i, j, fb, fc, val, freq;
  int db;
  
  for (i=18; i<24; i++)
  {
    for (j=0; j<20; j++)
    {
      fb = i | 0x60;
      radioWrite(0x75, fb);
      
      fc = j * 0xC80;
      val = fc >> 8;
      radioWrite(0x76, val);
      radioWrite(0x77, fc);
      
      delay(1); // give some time for pll to tune
      val = radioRead(0x26);
      
      // convert val to db
      db = map(val, 20, 205, -118, -20);
      
      freq = 480 + ((20*i) + j);
      printf("%03d, %d\n", freq, db);      
    }
  }
}


/**************************************************************************/
// This is to implement the printf function from within arduino
/**************************************************************************/
static int uart_putchar (char c, FILE *stream)
{
    Serial.write(c);
    return 0;
}
