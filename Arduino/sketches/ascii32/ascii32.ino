#include <chibi.h>
#include <ascii32.h>
#include <SPI.h>

#define LINE_SZ 100

static char line[LINE_SZ];
uint8_t sdCsPin = 10;
uint8_t radioCsPin = 30;
uint8_t radioSdnPin = 23; 

gps_t *gps;
scan_t scanBuf[1000]; // buffer to hold the scan points

// this is for printf
static FILE uartout = {0};  

/**************************************************************************/
/*!
  Setup
*/
/**************************************************************************/
void setup()
{
  // fill in the UART file descriptor with pointer to writer.
  fdev_setup_stream (&uartout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  
  // The uart is the standard output device STDOUT.
  stdout = &uartout ;
  
  Serial.begin(57600);
  //chibiCmdInit(57600);
  Serial1.begin(9600);
  ascii32.begin(radioCsPin, radioSdnPin, &Serial1, line);
  
  chibiCmdAdd("rd", cmdRadioRead);
  chibiCmdAdd("wr", cmdRadioWrite);
  chibiCmdAdd("freq", cmdChangeFreq);
  chibiCmdAdd("scan", cmdScan);
  
  gps = gps_getData();
}

/**************************************************************************/
/*!
  Loop
*/
/**************************************************************************/
void loop()
{
  int i, len;

  //chibiCmdPoll();
  
  gps_update();
  if (gps_available())
  {
    gps_clear_flag();    // clear the available flag since we read it already
    len = ascii32.radioScan(400, 960, scanBuf);
    
    for (i=0; i<len; i++)
    {
      printf("%d, %d\n", scanBuf[i].freq, scanBuf[i].db);
    }
    printf("gps, %s, %s, %s, %s\n", gps->date, gps->utc, gps->lat, gps->lon);
  }
}


/*********************************************************************/
//
//
/*********************************************************************/
void cmdRadioRead(int arg_cnt, char **args)
{
  uint8_t val, addr;
  
  addr = chibiCmdStr2Num(args[1], 16);
  val = ascii32.radioReadReg(addr);
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
  ascii32.radioWriteReg(addr, val);
  printf("Addr %02X = %02X.\n", addr, val);
}

/*********************************************************************/
//
//
/*********************************************************************/
void cmdChangeFreq(int arg_cnt, char **args)
{
  int freq = chibiCmdStr2Num(args[1], 10);
  ascii32.radioChangeFreq(freq);
}

/*********************************************************************/
//
//
/*********************************************************************/
void cmdScan(int arg_cnt, char **args)
{
  int i, len;
  len = ascii32.radioScan(840, 960, scanBuf);
  
  for (i=0; i<len; i++)
  {
    printf("%d, %d\n", scanBuf[i].freq, scanBuf[i].db);
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
