void handleWiFi() //Sets the WiFi credetials
{
  String st = "";
  byte n = WiFi.scanNetworks();
  st = "<label>SSID</label><select name=\"ssid\">";
  st += "<option>Choose a Network</option>";
  for (int i = 0; i < n; ++i){
    // Print SSID and RSSI for each network found
    st += "<option value=";
    st += String(i);
    st += ">";
    st += WiFi.SSID(i);
    st += " (";
    st += String(WiFi.RSSI(i));
    st += "dBm)";
    st += "</option>";
  }
  st += "</select><br>"; 

  String htmlDoc = R"=====(
  <!DOCTYPE html>
  <html>
    <head>
        <style type="text/css">
          ul{ list-style: none;}
          label{ display: inline-block; width: 80px; margin: 3px 0;}
          button{ width 120px; margin-top: 10px;}
        </style>
      <title>Clock Settings</title>
    </head>
    <body>
      <fieldset>
        <ul>
          <li>Fill in the information below, and press submit.</li>
          <form method="post" action="/update" id="clock_info">
            <li>
  )=====";
  
  htmlDoc += st;

  htmlDoc += R"=====(
            <br>
            </li>
            <li>
              <label>Password</label>
              <input type="password" name='pass'/>
            </li>
            <br>
            <li>
              <button type="submit">Submit</button>
            </li>
          </form>
          </ul>
        </fieldset>
    </body>
  </html>
  )=====";  
  webServer.send(200,"text/html", htmlDoc);
}
