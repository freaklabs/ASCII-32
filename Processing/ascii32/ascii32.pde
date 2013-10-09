import processing.serial.*;

color[] colors = {#9F9694, #791F33, #BA3D49, #F1E6D4, #E2E1DC};

Serial myPort;
int x, y, oldx, oldy;
int height, width;

// graph params
float graphLeftBorder = 0.12;
float graphRightBorder = 0.9;
float graphTopBorder = 0.1;
float graphBottomBorder = 0.9;
float tickHeight = 5;
float tickWidth = 5;
int textHeight = 20;
float labelPosition = 0.7;
float textPosition = 0.8;
PFont dataFont;
PFont tickFont;
PFont titleFont;

int startFreq = 400;
int stopFreq = 960;
int freqIntv = stopFreq - startFreq;
int startDB = -120;
int stopDB = 0;
int dBIntv = stopDB - startDB;

String[] labels = {"Date: ", "Time: ", "Lat: ", "Lon: "};

void setup()
{
  height = 500;
  width = 500;
  println(Serial.list());
  size(width, height);
  myPort = new Serial(this, Serial.list()[1], 57600);
  titleFont = loadFont("Garamond-24.vlw");
  dataFont = loadFont("Garamond-18.vlw");
  tickFont = loadFont("Garamond-12.vlw");
  oldx = 0; 
  oldy = 0;
}

void draw()
{
  while (myPort.available() > 0)
  {
    String inBuffer = myPort.readStringUntil('\n');
    
    if (inBuffer != null)
    {
      String delim = ", \n";
      String[] list = splitTokens(inBuffer, delim);
       
      if (list[0].equals("gps") == true)
      {
        //println(list);
        fill(colors[3], 200);
        textAlign(LEFT, CENTER);
        textFont(dataFont);
        
        for (int i=1; i<5; i++)
        {
          text(labels[i-1], width*graphRightBorder*labelPosition, height*graphTopBorder + (i*textHeight));
        }
        
        // date
        text(parseDate(list[1]), width*graphRightBorder*textPosition, height*graphTopBorder + (1*textHeight));
        
        // time
        text(parseTime(list[2]), width*graphRightBorder*textPosition, height*graphTopBorder + (2*textHeight));
        
        // latitude
        text(list[3], width*graphRightBorder*textPosition, height*graphTopBorder + (3*textHeight));
        
        // longitude
        text(list[4], width*graphRightBorder*textPosition, height*graphTopBorder + (4*textHeight));   
      }
      else
      {
        int[] nums = int(list);
        //println(nums);
        if (nums[0] == startFreq) 
        {
          background(colors[0]);
          oldx = round(width*graphLeftBorder);
          oldy = round(height*graphBottomBorder*0.9);
          
          // draw title
          fill(colors[3], 200);
          textFont(titleFont);
          textAlign(CENTER);
          text("ASCII-32 Spectrum Scanning/Mapping", width/2, height*0.05);
          
          // draw axes
          stroke(colors[3], 200);
          strokeWeight(1);
          line(width*graphLeftBorder, height*graphBottomBorder, width*graphLeftBorder, height*graphTopBorder); // yaxis
          line(width*graphLeftBorder, height*graphBottomBorder, width*graphRightBorder, height*graphBottomBorder);
          
          // draw axes titles
          textFont(dataFont);
          text("Frequency (MHz)", width/2, height*0.99);  
          
          pushMatrix();
          translate(width*0.04, height/2);
          rotate(-PI/2);        
          text("Signal Level (dB)", 0, 0);
          popMatrix();
          
          fill(0, 180);
          textFont(tickFont);
          textAlign(CENTER, CENTER);
          
          // x axis tick marks
          for (int i=10; i<=freqIntv; i+=50)
          {
            float tickX = map(i, 0, freqIntv, width*graphLeftBorder, width*graphRightBorder);
            line(tickX, height*graphBottomBorder-tickHeight, tickX, height*graphBottomBorder+tickHeight);
            text(startFreq + i, tickX, height*graphBottomBorder+4*tickHeight);
          }
          
          // y axis tick marks
          for (int i=10; i<=dBIntv; i+=10)
          {
              float tickY = map(i, 0, dBIntv, height*graphBottomBorder, height*graphTopBorder);
              line(width*graphLeftBorder-tickWidth, tickY, width*graphLeftBorder+tickWidth, tickY);
              text(startDB + i, width*graphLeftBorder-4*tickWidth, tickY);
          }
        }
        
        x = round(map(nums[0], startFreq, stopFreq, width*graphLeftBorder, width * graphRightBorder));
        y = round(map(nums[1], -120, 0, height * graphBottomBorder, height*graphTopBorder));
        
        stroke(colors[1], 200);
        strokeWeight(2);
        line(oldx, oldy, x, y);
        oldx = x;
        oldy = y;
      }     
    }
  }
}

String parseDate(String dateIn)
{
  // expects date in DDMMYY
  String day, month, year;
  day = dateIn.substring(0, 2);
  month = dateIn.substring(2, 4);
  year = dateIn.substring(4, 6);
  return (month + "/" + day + "/" + "20" + year);  
}

String parseTime(String timeIn)
{
  String hour, minute, second;
  hour = timeIn.substring(0, 2);
  minute = timeIn.substring(2, 4);
  second = timeIn.substring(4, 6);
  return (hour + ":" + minute + ":" + second);
}
