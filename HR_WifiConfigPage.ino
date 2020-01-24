// Configuration page
//
// We could have saved some RAM by removing the spaces, but this is a simple program
//  and we have enough RAM left over to allow us to splurge on readability.
//
// If you add more HTML and it crashes after generating the page, increase the buffer
//  size.  It's probalby overflowing the buffer.

char       generatedPageBuffer[ 5500 ] = "";
const char *configPage =
"<head>\n"
"  <style>\n"
"    table         { width: 400px; border-collapse: separate; border-spacing: 10px 0px}\n"
"    input         { width: 250px}\n"
"    .tableLabel   { text-align: right; width: 33%%}\n"
"    .tableInput   { text-align: left;}\n"
"    .indent       { margin-left: 20px;}\n"
"  </style>\n"
"</head>\n"
"\n"
"<body style=\"margin:20px\">\n"
"  <h1 align=\"center\">Heating Register Configuration</h1>\n"
"  <hr>\n"
"\n"
"  <form action=\"/config\" method=\"POST\">\n"
"    <h3 stlye=\"line-height:0px margin-bottom:0px\">WiFi Setup</h3>\n"
"    <p style=\"line-height:0px; margin-top:-10px\"><small><em>.local</em> is automatically appended to the host name for mDNS.  <span style=\"color:red;\">Reboot required after changing host name.</span></small></p>\n"
"    <table class=\"indent\">\n"
"      <tr>\n"
"        <td class=\"tableLabel\" align=\"right\" width=\"40%%\">Host Name</td>\n"
"        <td class=\"tableInput\"><input type=\"text\" name=\"hostname\" value=\"%s\"></td>\n"
"      </tr>\n"
"      <tr>\n"
"        <td class=\"tableLabel\" align=\"right\" width=\"40%%\">MAC Address</td>\n"
"        <td class=\"tableInput\">%02x:%02x:%02x:%02x:%02x:%02x</td>\n"
"      </tr>\n"
"    </table>\n"
"\n"
"    <h3 stlye=\"line-height:0px margin-bottom:0px\">Servo Positions</h3>\n"
"    <p style=\"line-height:0px; margin-top:-10px\"><small>0-254; 255 means &quot;unset&quot;.</small></p>\n"
"    <table class=\"indent\">\n"
"      <tr>\n"
"       <td class=\"tableLabel\">Open Left</td>\n"
"       <td class=\"tableInput\"><input type=\"number\" name=\"servoLOpen\" value=\"%d\" min=\"0\" max=\"255\"></td>\n"
"      </tr>\n"
"      <tr>\n"
"      <td class=\"tableLabel\">Closed Left</td>\n"
"      <td class=\"tableInput\"><input type=\"number\" name=\"servoLClosed\" value=\"%d\" min=\"0\" max=\"255\"></td>\n"
"      </tr>\n"
"      <tr style=\"border-bottom: 1px solid ghostwhite;\">\n"
"        <td class=\"tableLabel\">Current Position</td>\n"
"       <td class=\"tableInput\"><input type=\"number\" name=\"servoLCurrent\" value=\"%d\" min=\"0\" max=\"255\"></td>\n"
"      </tr>\n"
"      <tr><td></td></tr>\n"
"      <tr style=\"padding-top: 5px;\">\n"
"       <td class=\"tableLabel\">Open Right</td>\n"
"       <td class=\"tableInput\"><input type=\"number\" name=\"servoROpen\" value=\"%d\" min=\"0\" max=\"255\"></td>\n"
"      </tr>\n"
"      <tr>\n"
"       <td class=\"tableLabel\">Closed Right</td>\n"
"       <td class=\"tableInput\"><input type=\"number\" name=\"servoRClosed\" value=\"%d\" min=\"0\" max=\"255\"></td>\n"
"      </tr>\n"
"      <tr>\n"
"        <td class=\"tableLabel\">Current Position</td>\n"
"       <td class=\"tableInput\"><input type=\"number\" name=\"servoRCurrent\" value=\"%d\" min=\"0\" max=\"255\"></td>\n"
"      </tr>\n"
"    </table>\n"
"\n"
"    <h3 stlye=\"line-height:0px margin-bottom:0px\">Servo Speed</h3>\n"
"    <p style=\"line-height:0px; margin-top:-10px\"><small>0-255; 255 is the fastest speed, 0 is the slowest.</small></p>\n"
"    <table class=\"indent\">\n"
"      <tr>\n"
"       <td class=\"tableLabel\">Speed</td>\n"
"       <td class=\"tableInput\"><input type=\"number\" name=\"servoSpeed\" value=\"%d\" min=\"0\" max=\"255\"></td>\n"
"      </tr>\n"
"    </table>\n"
"\n"
"  <h3 stlye=\"line-height:0px margin-bottom:0px\">LED State</h3>\n"
"  <p style=\"line-height:0px; margin-top:-10px\"><small>Toggle if the LED is normally on or off.</small></p>\n"
"  <table>\n"
"    <tr>\n"
"      <td class=\"tableLabel\"></td>\n"
"      <td>\n"
"        <input style=\"width:auto\" type=\"checkbox\" id=\"LEDState\" name=\"LEDState\" value=\"1\"%s>\n"
"        <label for=\"LEDState\">LED Normally On</label>\n"
"      </td>\n"
"    </tr>\n"
"  </table>\n"
"\n"
"    <h3 stlye=\"line-height:0px margin-bottom:0px\">Submit Changes</h3>\n"
"    <input class=\"indent\" style=\"margin-top:-10px\" type=\"submit\" value=\"Submit\">\n"
"  </form>\n"
"\n"
"  <hr>\n"
"  <h3 stlye=\"line-height:0px margin-bottom:0px\">Testing</h3>\n"
"  <table>\n"
"    <tr>\n"
"      <td>\n"
"        <form action=\"/config/onFast\" method=\"POST\">\n"
"          <input style=\"margin-top:-10px\" type=\"submit\" value=\"Open Fast\">\n"
"        </form>\n"
"      </td>\n"
"      <td>\n"
"       <form action=\"/config/offFast\" method=\"POST\">\n"
"         <input style=\"margin-top:-10px\" type=\"submit\" value=\"Close Fast\">\n"
"       </form>\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td>\n"
"        <form action=\"/config/on\" method=\"POST\">\n"
"          <input style=\"margin-top:-10px\" type=\"submit\" value=\"Open\">\n"
"        </form>\n"
"      </td>\n"
"      <td>\n"
"       <form action=\"/config/off\" method=\"POST\">\n"
"         <input style=\"margin-top:-10px\" type=\"submit\" value=\"Close\">\n"
"       </form>\n"
"      </td>\n"
"    </tr>\n"
"    <tr>\n"
"      <td align=\"center\" colspan=\"2\">\n"
"        <form action=\"/config/blink\" method=\"POST\">\n"
"          <input style=\"margin-top:-10px\" type=\"submit\" value=\"Blink LED\">\n"
"        </form>\n"
"      </td>\n"
"    </tr>\n"
"  </table>\n"
"  <hr>\n"
"  <h3 stlye=\"line-height:0px margin-bottom:0px\">Reboot</h3>\n"
"  <p style=\"line-height:0px; margin-top:-10px\"><small>Rebooting is required after changing the host name.</small></p>\n"
"    <form action=\"/config/reboot\" method=\"POST\">\n"
"     <input class=\"indent\" style=\"margin-top:-10px\" type=\"submit\" value=\"Reboot\">\n"
"   </form>\n"
"</body>";

  const char *
generateConfigPage() {
  byte mac[6];
  WiFi.macAddress(mac);

  sprintf( generatedPageBuffer, configPage,
           hostName.c_str(),
           mac[5], mac[4], mac[3], mac[2], mac[1], mac[0],
           servoObj[ SERVO_LEFT  ]->getOpenPos(), servoObj[ SERVO_LEFT  ]->getClosedPos(), servoObj[ SERVO_LEFT  ]->getCurPos(),
           servoObj[ SERVO_RIGHT ]->getOpenPos(), servoObj[ SERVO_RIGHT ]->getClosedPos(), servoObj[ SERVO_RIGHT ]->getCurPos(),
           servoSpeed,
           ledState ? " checked" : "" );

  return generatedPageBuffer;
}

// When a reboot is triggered, we load the rebooting page instead, which waits about 10 seconds
//  before loading the config page again.  This gives the hardware enough time to reboot and
//  reconnect.
  const char *
rebootingPage() {
  return "<head>>\n"
         "  <meta http-equiv=\"refresh\" content=\"10; /config\">\n"
         "</head>\n"
         "<body>\n"
         "  <h2 align=\"center\">Rebooting; config page will auto-reload in 10 seconds...</h2>\n"
         "</body>\n";
}
