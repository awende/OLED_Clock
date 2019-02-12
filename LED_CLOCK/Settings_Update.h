void handleUpdate() //Updates settings like WiFi, Time Zone, DST, Overnight Brightness, LED settings
{
  bool restart_needed = 0;
  
  if (webServer.args() > 0 ) {
    for ( uint8_t i = 0; i < webServer.args(); i++ ) {
      if(webServer.argName(i) == "ssid") //Network SSID
      {
        String mySSID = WiFi.SSID(webServer.arg(i).toInt());
        EEPROM.writeString(0,mySSID);
        Serial.print(webServer.argName(i));
        Serial.print('\t');
        Serial.println(mySSID);
      }
      else if(webServer.argName(i) == "pass"){  //Network Password
        String myPass = webServer.arg(i);
        EEPROM.writeString(32,myPass);
        Serial.print(webServer.argName(i));
        Serial.print('\t');
        Serial.println(myPass);
        restart_needed = 1;
      }
      else if(webServer.argName(i) == "tz"){  //Time Zone
        byte timeZone = webServer.arg(i).toInt();
        EEPROM.write(96,timeZone);
        EEPROM.write(97,0);
        Serial.print(webServer.argName(i));
        Serial.print('\t');
        Serial.println(timeZone);
      }
      else if(webServer.argName(i) == "dst"){ //Daylight Saving Time
        bool dst = webServer.arg(i).toInt();
        EEPROM.write(97,dst);
        
        Serial.print(webServer.argName(i));
        Serial.print('\t');
        Serial.println(dst);
      }
      else
      {
        Serial.print(webServer.argName(i));
        Serial.print('\t');
        Serial.println(webServer.arg(i));
      }
    }
    EEPROM.commit();
    getNtpTime();
  }

  IPAddress myIP = WiFi.localIP();
  
  String htmlDoc = R"=====(
  <!DOCTYPE html>
  <html>
  <head>
  <meta http-equiv="Refresh" content="5;url=/">  
  </head>
  
  <body>
  <p>Applying new settings.</p>
  <p>If the WiFi settings were changed, the clock will automatically restart and use the new WiFi settings.</p>
  </body>
  
  </html>
  )=====";
  webServer.send(200, "text/html", htmlDoc);
  
  if(restart_needed){
    system_restart();
  }
}
