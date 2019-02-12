void handleRoot() // Main settings page
{
  String htmlDoc = R"=====(
  <!DOCTYPE html>
  <html>
    <head>
      <style type="text/css">
          button{ width 400px; height: 30px; margin-top: 40px;}
          #wrapper { display: flex; align-items: center; justify-content: center; }
      </style>
      <title>Clock Settings</title>
    </head>
    <body>
      <div id="wrapper">
        <h1><span id="currentTime">--:--:--</span></h1>
      </div>
      <div id="wrapper">
        <button type="submit" onclick="window.location.href='/wifi'">WiFi Settings</button>
      </div>
      <div id="wrapper">
        <button type="submit" onclick="window.location.href='/tz'">Time Zone</button>
      </div>    
    </body>
    
    <script>

        setInterval(function() {
          // Call a function repetatively with 1 Second interval
          getData("currentTime");
        }, 500); //1000mSeconds update rate
    
        function getData(myID) {
          
          var xhttp = new XMLHttpRequest();
          xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200) {
              document.getElementById(myID).innerHTML =
              this.responseText;
            }
          };

          if(myID == "currentTime"){
            xhttp.open("GET", "/timeNow", true);
          }
          else if(myID == "wifiStatus") {
            xhttp.open("GET", "/wifistatus", true);
          }
          
          xhttp.send();
        }
    </script>
  </html>
  )=====";
  webServer.send(200,"text/html", htmlDoc);
}





void handleTime()
{
  webServer.send(200, "text/plane", currentTime()); //Send ADC value only to client ajax request
}




void handleWiFiStatus()
{
  if(WiFi.status() == WL_CONNECTED) webServer.send(200, "text/plane", "CONNECTED");
  else webServer.send(200, "text/plane", "DISCONNECTED");
}
