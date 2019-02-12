#include <WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <SPI.h>

#include <SFE_MicroOLED.h>//https://github.com/sparkfun/SparkFun_Micro_OLED_Arduino_Library/tree/V_1.0.0

#include <WebServer.h>    //https://github.com/zhouhan0126/WebServer-esp32
#include <Timezone.h>     //https://github.com/JChristensen/Timezone



WebServer webServer(80);

//Define Daylight Savings Time Rules
// US Eastern Time Zone (New York, Detroit)
TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time = UTC - 4 hours
TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time = UTC - 5 hours
Timezone usET_DST(usEDT, usEST);
Timezone usET_STD(usEST, usEST);

// US Central Time Zone (Chicago, Houston)
TimeChangeRule usCDT = {"CDT", Second, dowSunday, Mar, 2, -300};
TimeChangeRule usCST = {"CST", First, dowSunday, Nov, 2, -360};
Timezone usCT_DST(usCDT, usCST);
Timezone usCT_STD(usCST, usCST);

// US Mountain Time Zone (Denver, Salt Lake City)
TimeChangeRule usMDT = {"MDT", Second, dowSunday, Mar, 2, -360};
TimeChangeRule usMST = {"MST", First, dowSunday, Nov, 2, -420};
Timezone usMT_DST(usMDT, usMST);
Timezone usMT_STD(usMST, usMST);

// US Pacific Time Zone (Las Vegas, Los Angeles)
TimeChangeRule usPDT = {"PDT", Second, dowSunday, Mar, 2, -420};
TimeChangeRule usPST = {"PST", First, dowSunday, Nov, 2, -480};
Timezone usPT_DST(usPDT, usPST);
Timezone usPT_STD(usPST, usPST);

time_t utc;

const char ntpServerName[14][25] = {
  //Maryland
  "time-a-g.nist.gov",
  "time-b-g.nist.gov",
  "time-c-g.nist.gov",
  "time-d-g.nist.gov",
  //Fort Collins
  "time-a-wwv.nist.gov",
  "time-b-wwv.nist.gov",
  "time-c-wwv.nist.gov",
  "time-d-wwv.nist.gov",
  //Boulder
  "time-a-b.nist.gov",
  "time-b-b.nist.gov",
  "time-c-b.nist.gov",
  "time-d-b.nist.gov",
  //University of Colorado, Boulder
  "utcnist.colorado.edu",
  "utcnist2.colorado.edu"
};

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

//Digit 4
#define PIN_RESET_4 21
#define PIN_DC_4    22
#define PIN_CS_4    5

//Digit 5
#define PIN_RESET_5 25
#define PIN_DC_5    22
#define PIN_CS_5    26


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
MicroOLED oled4(PIN_RESET_4, PIN_DC_4, PIN_CS_4);
MicroOLED oled5(PIN_RESET_5, PIN_DC_5, PIN_CS_5);

byte old_second=0, old_minute=0,old_hour=0;
time_t prevNow=0;

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

void getNtpTime()
{
  bool goodTimeReturned = 0;
  if(WiFi.status() != WL_CONNECTED) Serial.println("Not Connected to WiFi");
  
  for(byte i=0; i<14; i++)
  {
    IPAddress ntpServerIP;
  
    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    
    WiFi.hostByName(ntpServerName[i], ntpServerIP);
    sendNTPpacket(ntpServerIP);
    
    uint32_t beginWait = millis();
    while ((millis() - beginWait) < 1000) {
      int size = Udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) {
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
  
        utc = secsSince1900 - 2208988800UL;
        setTime(utc);
        goodTimeReturned = 1;
        break;
      }
    }
    if(goodTimeReturned) break;
  }
  if(!goodTimeReturned)Serial.println("No response from any server");
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

TaskHandle_t Task1;

byte led_effect = 0;
uint32_t led_color = 0;
byte rainbow_speed = 0;
byte led_brightness = 0;
byte alt_led_brightness= 0;
byte alt_hr_start,alt_min_start,alt_hr_end,alt_min_end;
bool alt_brightness = 0;

byte timeZoneVal = 0;
bool dst = 0;
bool wifi_connected = 0;

#include "Settings_Update.h"
#include "Timezone.h"
#include "WiFi_Credentials.h"
#include "Root.h"

void setup() {
  EEPROM.begin(110);
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
  oled4.begin();
  oled4.clear(PAGE);
  oled5.begin();
  oled5.clear(PAGE);
  
  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  
  delay(500);

  
  
  test_wifi();

  //--------------- Saved Settings ---------------//
  Serial.println("\nSaved Settings:");
  Serial.print("SSID:\t\t");
  for(byte i=0;i<32;i++)
  {
    char myChar = EEPROM.read(i);
    if(myChar == 0 || myChar == 255) break;
    else            Serial.print(myChar);
  }
  Serial.println();

  Serial.print("Password:\t");
  for(byte i=32;i<96;i++)
  {
    char myChar = EEPROM.read(i);
    if(myChar == 0 || myChar == 255) break;
    else            Serial.print(myChar);
  }
  Serial.println();

  Serial.print("Timezone:\t");
  Serial.println(EEPROM.read(96));

  Serial.print("DST:\t\t");
  Serial.println(EEPROM.read(97));

  Serial.print("Brightness Start:\t");
  Serial.print(EEPROM.read(98));
  Serial.print(':');
  Serial.println(EEPROM.read(99));
  
  Serial.print("Brightness End:\t");
  Serial.print(EEPROM.read(100));
  Serial.print(':');
  Serial.println(EEPROM.read(101));

  Serial.print("Brightness Value:\t");
  Serial.println(EEPROM.read(102));
  
  //----------------------------------------------//
  webServer.on("/",handleRoot);
  webServer.on("/wifi",handleWiFi);
  webServer.on("/tz",handleTZ);
  webServer.on("/update",handleUpdate);
  webServer.on("/timeNow",handleTime);
  webServer.on("/wifistatus",handleWiFiStatus);
  
  // simple HTTP server to see that DNS server is working
  webServer.onNotFound([]() {
    String message = "Hello World!\n\n";
    message += "URI: ";
    message += webServer.uri();

    webServer.send(200, "text/plain", message);
  });
  webServer.begin();

  time_t t = getLocal();
  
  //Display Current Time
  Update_Digit(oled0,hourFormat12(t)/10,32);
  Update_Digit(oled1,hourFormat12(t)%10,32);
  Update_Digit(oled2,minute(t)/10,32);
  Update_Digit(oled3,minute(t)%10,32);
  Update_Digit(oled4,second(t)/10,32);
  Update_Digit(oled5,second(t)%10,32);

  prevNow = now();
}

//Task1code: blinks an LED every 1000 ms
void Task1code( void * pvParameters ){
  uint32_t start = millis();
  for(;;)
  {
    vTaskDelay(10);
    webServer.handleClient();
    if((millis()-start > 10000) && wifi_connected)
    {
      getNtpTime();
      start = millis();
    }
  }
}

void loop() {
  check_serial();
  //Only update when the secconds change
  if(now() != prevNow)
  {
    prevNow = now();
    
    time_t t = getLocal();

    //Update Display
    for(byte i=0;i<33;i++)
    {
      if(hourFormat12(t) != old_hour)  //Does hour need to be updated?
      {
        if((hourFormat12(t)/10)!= old_hour/10) //Which hour digit needs to update? Both?
        {
          
          Update_Digit(oled0,hourFormat12(t)/10,i);
          Update_Digit(oled1,hourFormat12(t)%10,i);
        }
        else  //Just update the first hour digit
        {
          Update_Digit(oled1,hourFormat12(t)%10,i);
        }
        
      }

      if(minute(t) != old_minute)  //Does the minutes need to updated?
      {
        if((minute(t)/10)!= old_minute/10) //Which digit needs to be updated? Both?
        {
          Update_Digit(oled2,minute(t)/10,i);
          Update_Digit(oled3,minute(t)%10,i);
        }
        else  //Just update the first minute digit
        {
          Update_Digit(oled3,minute(t)%10,i);
        }
        
      }

      if(second(t) != old_second)  //Does the minutes need to updated?
      {
        if((second(t)/10)!= old_second/10) //Which digit needs to be updated? Both?
        {
          Update_Digit(oled4,second(t)/10,i);
          Update_Digit(oled5,second(t)%10,i);
        }
        else  //Just update the first minute digit
        {
          Update_Digit(oled5,second(t)%10,i);
        }
        
      }
      delay(5); //Wait 5ms to slow down the animations
    }
    old_hour = hourFormat12(t);
    old_minute = minute(t);
    old_second = second(t);
    //Serial.println(currentTime());
  }
}

void check_serial()
{
  if(Serial.available())
  {
    delay(5);
    String message = "";
    while(Serial.available())
    {
      message += char(Serial.read());
    }

    if(message.startsWith("ssid:"))
    {
      message = message.substring(5);
      Serial.println("Setting SSID to:" + message);
      uint8_t i=0;
      for(i=0;i<32;i++)
      {
        if(message[i] == '\r' || message[i] == '\n' || message[i] == 0) break;
        else
        {
          char myChar = message[i];
          EEPROM.write(i,myChar);
        }
      }
      EEPROM.write(i,0);
      EEPROM.commit();
    }
    else if(message.startsWith("psk:"))
    {
      message = message.substring(4);
      Serial.println("Setting PSK to:" + message);
      uint8_t i=0;
      for(i=0;i<64;i++)
      {
        if(message[i] == '\r' || message[i] == '\n' || message[i] == 0) break;
        else
        {
          char myChar = message[i];
          EEPROM.write(i+32,myChar);
        }
      }
      EEPROM.write(i+32,0);
      EEPROM.commit();
    }
  }
}

void test_wifi()
{
  char storedSSID[32];
  for(byte i=0;i<32;i++)
  {
    char myChar = EEPROM.read(i);
    if(myChar == 0 || myChar == 255)
    {
      storedSSID[i] = 0;
      break;
    }
    else storedSSID[i] = myChar;
  }

  char storedPass[64];
  for(byte i=0;i<64;i++)
  {
    char myChar = EEPROM.read(i+32);
    if(myChar == 0 || myChar == 255)
    {
      storedPass[i] = 0;
      break;
    }
    else storedPass[i] = myChar;
  }
  
  WiFi.begin(storedSSID, storedPass);
  Serial.println();

  // Wait for connection
  double start_time = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print(".");
    if(millis()-start_time > 5000) break;
  }
  Serial.println("");

  if(WiFi.status() != WL_CONNECTED) //Generate AP
  {
    Serial.println("Unable to connect to saved WiFi network :(");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("WiFi Clock", "password");
  }
  else
  {
    wifi_connected = 1;
    WiFi.mode(WIFI_STA);
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  
    Serial.println("Starting UDP");
    Udp.begin(localPort);
    getNtpTime();
    prevNow = now();
  }
}

time_t getLocal()
{
  byte timeZoneVal = EEPROM.read(96);
  bool dst = EEPROM.read(97);
  Timezone tz = usET_DST;

  if(timeZoneVal == 4)
  {
    if(dst) tz = usET_DST;
    else    tz = usET_STD;
  }
  else if(timeZoneVal == 5)
  {
    if(dst) tz = usCT_DST;
    else    tz = usCT_STD;
  }
  else if(timeZoneVal == 6)
  {
    if(dst) tz = usMT_DST;
    else    tz = usMT_STD;
  }
  else if(timeZoneVal == 7)
  {
    if(dst) tz = usPT_DST;
    else    tz = usPT_STD;
  }
  
  TimeChangeRule *tcr;
  return tz.toLocal(now(),&tcr);
}

String currentTime()
{
  time_t t = getLocal();
  
  String timeNow = "";
  timeNow += String(hourFormat12(t)/10);
  timeNow += String(hourFormat12(t)%10);
  timeNow += ':';
  timeNow += String(minute(t)/10);
  timeNow += String(minute(t)%10);
  timeNow += ':';
  timeNow += String(second(t)/10);
  timeNow += String(second(t)%10);
  return timeNow;
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
