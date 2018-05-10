//Included in IDE/ESP32 Native Libraries
#include <WiFi.h>
#include <WiFiUdp.h>
#include <SPI.h>

//Need to be downloaded
#include <SFE_MicroOLED.h>//https://github.com/sparkfun/SparkFun_Micro_OLED_Arduino_Library/tree/V_1.0.0
#include <DNSServer.h>    //https://github.com/zhouhan0126/DNSServer---esp32
#include <WebServer.h>    //https://github.com/zhouhan0126/WebServer-esp32
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <Timezone.h>     //https://github.com/JChristensen/Timezone



//Define Daylight Savings Time Rules
//US Mountain Time Zone
TimeChangeRule myDST = {"MDT", Second, Sun, Mar, 2, -360};    //Daylight time = UTC - 6 hours
TimeChangeRule mySTD = {"MST", First, Sun, Nov, 2, -420};     //Standard time = UTC - 7 hours
Timezone myTZ(myDST, mySTD);

TimeChangeRule *tcr;    //pointer to the time change rule, use to get TZ abbrev
time_t utc, local;

// NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";
//static const char ntpServerName[] = "time.nist.gov";

WiFiUDP Udp;                    //Initialize UDP library
unsigned int localPort = 8888;  // local port to listen for UDP packets

//IO Pin Constants For OLED Breakout Boards
//Digit 0
#define PIN_RESET_0 12
#define PIN_DC_0    22
#define PIN_CS_0    13

//Digit 1
#define PIN_RESET_1 17
#define PIN_DC_1    22
#define PIN_CS_1    16

//Digit 2
#define PIN_RESET_2 4
#define PIN_DC_2    22
#define PIN_CS_2    0

//Digit 3
#define PIN_RESET_3 2
#define PIN_DC_3    22
#define PIN_CS_3    15


//7-Seg Pixel Constants for OLED
#define A_X 59
#define A_Y 14

#define B_X 35
#define B_Y 38

#define C_X 6
#define C_Y 38

#define D_X 0
#define D_Y 14

#define E_X 6
#define E_Y 7

#define F_X 35
#define F_Y 7

#define G_X 29 
#define G_Y 14

//Initialize Displays
MicroOLED oled0(PIN_RESET_0, PIN_DC_0, PIN_CS_0);
MicroOLED oled1(PIN_RESET_1, PIN_DC_1, PIN_CS_1);
MicroOLED oled2(PIN_RESET_2, PIN_DC_2, PIN_CS_2);
MicroOLED oled3(PIN_RESET_3, PIN_DC_3, PIN_CS_3);

byte old_minute=0,old_hour=0;
time_t prev = 0, prevNow=0;

void setup(void)
{
  Serial.begin(115200);

  //Setup Displays
  oled0.begin();
  oled0.clear(PAGE);
  oled1.begin();
  oled1.clear(PAGE);
  oled2.begin();
  oled2.clear(PAGE);
  oled3.begin();
  oled3.clear(PAGE);
  
  //Start WiFi Manager
  WiFiManager wifiManager;
  wifiManager.autoConnect("ESP32 Clock");
  
  //Print WiFi Debug
  Serial.print("IP number assigned by DHCP is ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  Udp.begin(localPort);

  Serial.println("waiting for sync");

  //Try to get time from NTP Server
  byte attempt = 0;
  for(byte i=0;i<5;i++)
  {
    Serial.println("attempt:"+String(i+1));
    if(getNtpTime() != 0) break;
    delay(1000);
  }
  
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  
  
  //Display Current Time
  Update_Digit(oled0,hourFormat12()/10,32);
  Update_Digit(oled1,hourFormat12()%10,32);
  Update_Digit(oled2,minute()/10,32);
  Update_Digit(oled3,minute()%10,32);
  prev = now();
  prevNow = now()/60;
  
}

void loop(void)
{
  //Print the current time to Serial (debugging)
  if(now() != prevNow)
  {
    prevNow = now();
    Serial.print(hour());
    Serial.print(' ');
    Serial.print(minute());
    Serial.print(' ');
    Serial.print(second());
    Serial.println();
  }

  //Only update when the minutes change
  if(now()/60 != prev)
  {
    prev = now()/60;

    //Update Display
    for(byte i=0;i<33;i++)
    {
      if(hourFormat12() != old_hour)  //Does hour need to be updated?
      {
        if((hourFormat12()/10)!= old_hour/10) //Which hour digit needs to update? Both?
        {
          
          Update_Digit(oled0,hourFormat12()/10,i);
          Update_Digit(oled1,hourFormat12()%10,i);
        }
        else  //Just update the first hour digit
        {
          Update_Digit(oled1,hourFormat12()%10,i);
        }
        
      }

      if(minute() != old_minute)  //Does the minutes need to updated?
      {
        if((minute()/10)!= old_minute/10) //Which digit needs to be updated? Both?
        {
          Update_Digit(oled2,minute()/10,i);
          Update_Digit(oled3,minute()%10,i);
        }
        else  //Just update the first minute digit
        {
          Update_Digit(oled3,minute()%10,i);
        }
        
      }
      delay(5); //Wait 5ms to slow down the animations
    }
    old_hour = hourFormat12();
    old_minute = minute();
  }
}

//Animations for changing numbers
void Update_Digit(MicroOLED &oled,byte number, byte i)
{
  oled.clear(PAGE);
  switch(number)
  {
    case 0:
      if(i<17)
      {
        oled.rectFill(A_X+(64-i*4),A_Y,4,22); //A
        oled.rectFill(A_X-(i*3.6),A_Y,4,22);  //A
        oled.rectFill(B_X,B_Y,22,4); //B
        oled.rectFill(C_X,C_Y,22,4); //C
        oled.rectFill(E_X+(32-i*2),E_Y,22,4); //E
        oled.rectFill(F_X+(32-i*2),F_Y,22,4); //F
        oled.rectFill(G_X-(i*4),G_Y,4,22); //G
      }
      else
      {
        oled.rectFill(A_X,A_Y,4,22); //A
        oled.rectFill(B_X,B_Y,22,4); //B
        oled.rectFill(C_X,C_Y,22,4); //C
        oled.rectFill(D_X,D_Y,4,22); //D
        oled.rectFill(E_X,E_Y,22,4); //E
        oled.rectFill(F_X,F_Y,22,4); //F
      }
      
    break;

    case 1:
      oled.rectFill(A_X-(i*2),A_Y,4,22); //A
      oled.rectFill(B_X,B_Y,22,4); //B
      oled.rectFill(C_X,C_Y,22,4); //C
      oled.rectFill(D_X-(i*2),D_Y,4,22); //D
      oled.rectFill(E_X-(i*2),E_Y,22,4); //E
      oled.rectFill(F_X-(i*2),F_Y,22,4); //F
    break;
    
    case 2:
      oled.rectFill(A_X+(64-i*2),A_Y,4,22); //A
      oled.rectFill(B_X,B_Y,22,4); //B
      oled.rectFill(C_X-(i*2),C_Y,22,4); //C
      oled.rectFill(D_X+(64-i*2),D_Y,4,22); //D
      oled.rectFill(E_X+(64-i*2),E_Y,22,4); //E
      oled.rectFill(G_X+(64-i*2),G_Y,4,22); //G
    break;
    
    case 3:
      oled.rectFill(A_X,A_Y,4,22); //A
      oled.rectFill(B_X+(32-i),B_Y,22,4); //B
      oled.rectFill(C_X+(32-i),C_Y,22,4); //C
      oled.rectFill(D_X,D_Y,4,22); //D
      oled.rectFill(E_X-(i*2),E_Y,22,4); //E
      oled.rectFill(G_X,G_Y,4,22); //G
    break;
    
    case 4:
      oled.rectFill(A_X-i+2,A_Y,4,22); //A
      oled.rectFill(B_X,B_Y,22,4); //B
      oled.rectFill(C_X,C_Y,22,4); //C
      oled.rectFill(D_X-i,D_Y,4,22); //D
      oled.rectFill(F_X+(32-i),F_Y,22,4); //F
      oled.rectFill(G_X-i*1.0625,G_Y,4,22); //G
    break;
    
    case 5:
      oled.rectFill(A_X+(32-i),A_Y,4,22); //A
      oled.rectFill(B_X-i*0.9,B_Y,22,4); //B
      oled.rectFill(C_X-i,C_Y,22,4); //C
      oled.rectFill(G_X+(32-i),G_Y,4,22);
      oled.rectFill(F_X,F_Y,22,4); //F
      oled.rectFill(G_X-i*0.9,G_Y,4,22); //G
    break;
    
    case 6:
      oled.rectFill(A_X,A_Y,4,22); //A
      oled.rectFill(C_X,C_Y,22,4); //C
      oled.rectFill(D_X,D_Y,4,22); //D
      if(i>3)oled.rectFill(E_X+(32-i),E_Y,22,4); //E
      else oled.rectFill(F_X,F_Y,22,4); //E
      oled.rectFill(F_X+(32-i),F_Y,22,4); //F
      oled.rectFill(G_X,G_Y,4,22); //G
    break;
    
    case 7:
      oled.rectFill(A_X,A_Y,4,22); //A
      oled.rectFill(B_X+(64-i*2),B_Y,22,4); //B
      oled.rectFill(C_X,C_Y,22,4); //C
      oled.rectFill(D_X-(i*2),D_Y,4,22); //D
      oled.rectFill(E_X-(i*2),E_Y,22,4); //E
      oled.rectFill(F_X-(i*2),F_Y,22,4); //F
      oled.rectFill(G_X-(i*2),G_Y,4,22); //G
    break;
    
    case 8:
      oled.rectFill(A_X+(64-i*2),A_Y,4,22); //A
      oled.rectFill(A_X-(i*1.9),A_Y,4,22); //A
      oled.rectFill(B_X,B_Y,22,4); //B
      oled.rectFill(C_X,C_Y,22,4); //C
      oled.rectFill(E_X+(64-i*2),E_Y,22,4); //E
      oled.rectFill(F_X+(64-i*2),F_Y,22,4); //F
      oled.rectFill(G_X+(64-i*2),G_Y,4,22); //G
    break;
    
    case 9:
      oled.rectFill(A_X,A_Y,4,22); //A
      oled.rectFill(B_X,B_Y,22,4); //B
      oled.rectFill(C_X,C_Y,22,4); //C
      oled.rectFill(D_X-(i*2),D_Y,4,22); //D
      oled.rectFill(E_X-(i*2),E_Y,22,4); //E
      oled.rectFill(F_X,F_Y,22,4); //F
      oled.rectFill(G_X,G_Y,4,22); //G
    break;

    default:
    break;
  }
  oled.display();
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];

      utc = secsSince1900 - 2208988800UL;
      return myTZ.toLocal(utc, &tcr); //Return local time (with time change rules)
    }
  }
  Serial.println("No NTP Response :(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
