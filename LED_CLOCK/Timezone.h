void handleTZ() //Sets the clock's timezone and DST adjustment
{
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
              <label>Time Zone</label>
              <select name="tz">
                <option value=0>Choose Time Zone</option>
                <option value=4>Eastern</option>
                <option value=5>Central</option>
                <option value=6>Mountain</option>
                <option value=7>Pacific</option>
              </select>
            </li>
            <li>
              <input type="checkbox" name="dst" value="1">Adjust for daylight savings time
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
