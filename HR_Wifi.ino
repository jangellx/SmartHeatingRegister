const char *ssid;
const char *password;
const char *hostNameBase = "heatreg_";
String      hostName;

// Build Information for a NodeMCU:
// - Choose an ESP8266 variant, specifically Generic ESP8266 Module.
// - Set the Flash Size to 4 MB, No SPIFFS.   Mostly so we can do OTA updates.

// Web server information

// State related to moving the servo to a new location.  We wait a bit after the last command to actually move,
//  the move the servo either as fast as possible or at a given speed.
#define ACTION_DELAY      500     // Milliseconds to wait between when we get an HTTP action and when we act on it.
// Basic settings
int     actionSpeed     = 255;    // If 255, move as fast as possible; otherwise, step the servo over time, down to SERVO_SLOWEST_SPEED.
int     actionPos       =  -1;    // The position to move the damper to, 0-100.  -1 for no pending action.
int     actionTime      =   0;    // When the last action was recieved in milliseconds, after which we wait ACTION_DELAY ms before moving.  0 for no pending action.
// Speed Settings (when actionSpeed is not 255)
int     actionPosOld    =   0;    // The position we started from.  Used to smoothly move between two positoins when speed is not 255.
int     actionStartTime =   0;    // When we started moving with actionSpeed.  Used to compute how far we've moved so far.

bool    showHTTPStatusRequests = false;   // When true HTTP status requests are displayed in the serial output.  This happens often, so it's nice to hide it

// LED Blinking
#define MAX_BLINKS         50    // Maximum number of blinks before we stop blinking
#define BLINK_PERIOD      500    // Time in milliseconds before blinking
int     blinkTime       =   0;    // Time since we last changed the LED.  0 if blinking is off.
int     blinkCount      =   0;    // How many times we've blinked.

// Web server
ESP8266WebServer server(80);

void initWifi() {
  // Set the device mode to "station" to avoid avoid creating a FairlyLink AP
  //  https://arduino.stackexchange.com/questions/47806/what-is-farylink-access-point
  WiFi.mode( WIFI_STA );

  // Initialize defaults from HR_WifiConfig
  initWifiConfig();

  // Print the MAC Address
  Serial.println( "Wifi Init" );
  byte mac[6];
  WiFi.macAddress(mac);

  Serial.print( "- MAC Address:  " );
  Serial.print(  mac[5],HEX); Serial.print(":");
  Serial.print(  mac[4],HEX); Serial.print(":");
  Serial.print(  mac[3],HEX); Serial.print(":");
  Serial.print(  mac[2],HEX); Serial.print(":");
  Serial.print(  mac[1],HEX); Serial.print(":");
  Serial.println(mac[0],HEX);

  // Load hostname from the EEPROM.  Note that 255 means "uninitialized", so we need to test for that.
  char c = EEPROM.read( FLASHOFFSET_HOSTNAME );
  if( (c != 0) && (c != 255) ) {
    hostName = String( "" );
    for( int i=0; i < 32; i++ ) {
        c = EEPROM.read( FLASHOFFSET_HOSTNAME+i );
        if( (c == 0) && (c == 255) )
          break;

        hostName += c;
    }
  }

  if( hostName.length() == 0 ) {
    // Default hostname
    hostName = String( hostNameBase );
    hostName.concat( String( mac[5], HEX ) );
    hostName.concat( String( mac[4], HEX ) );
    hostName.concat( String( mac[3], HEX ) );
    hostName.concat( String( mac[2], HEX ) );
    hostName.concat( String( mac[1], HEX ) );
    hostName.concat( String( mac[0], HEX ) );
  }

  // Update our host name
//  WiFi.hostname( hostName.c_str() );                // Doesn't work for whatever reason
//  wifi_station_set_hostname( hostName.c_str() );    // Also doesn't work, I guess
  ArduinoOTA.setHostname( hostName.c_str() );         // This actually works!
  Serial.print( "- Hostname: " );
  Serial.println( hostName );

  // Initilaize OTA update support (ArduinoOTA) (pretty much just copied from https://randomnerdtutorials.com/esp8266-ota-updates-with-arduino-ide-over-the-air/ )
  ArduinoOTA.onStart([]() {
    Serial.println("OTA: Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("OTA: End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA: Progress: %u%%\r", (progress / (total / 100)));

    static int ledState = 0;
    digitalWrite( LED_BUILTIN, ledState++ ? HIGH : LOW );
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if      (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR)     Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("- ArduinoOTA Initialized" );
  
  // Start up wifi and connect
  Serial.print("- Connecting to wifi at " );
  Serial.print( ssid );
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }

  Serial.println(" done." );
  Serial.print("- Connected to ");
  Serial.println(ssid);
  Serial.print("- IP address: ");
  Serial.println(WiFi.localIP());

  // Start mDNS
  String  mdnsName( hostName + ".local" ); 
  if (MDNS.begin(mdnsName, WiFi.localIP())) {
    MDNS.addService("http", "tcp", 80);
    Serial.print("- MDNS responder started:  ");
    Serial.println( mdnsName );
  }

  // Set up the web server, including the on/off/dim and 404 callbacks
  server.on("/", [mdnsName](){
    server.send(200, "text/plain", mdnsName);
  });
 
  server.on("/on", [](){
    server.send(200, "text/plain", "Register open (on)");
    Serial.println("HTTP: Register open (on)");
    actionTime  = millis();
    actionPos   = 100;
    actionSpeed = servoSpeed;
  });
 
  server.on("/off", [](){
    server.send(200, "text/plain", "Register closed (off)");
    Serial.println("HTTP: Register closed (off)");
    actionTime  = millis();
    actionPos   = 0;
    actionSpeed = servoSpeed;
  });
 
  server.on("/status", [](){
    server.send(200, "text/plain", damperIsOpen() ? "1" : "0");
    if( showHTTPStatusRequests )
      Serial.println("HTTP: Status request processed");
  });

  server.on("/config", [](){
    if( server.method() == HTTP_GET ) {
      // Get request; return the config page
      server.send(200, "text/html", generateConfigPage());
      Serial.println("HTTP: Config page requested");
    }

    if( server.method() == HTTP_POST ) {
      if( server.hasArg( "hostname" ) ) {
        String newHostName = String( server.arg( "hostname" ) );
        if( newHostName != hostName ) {
          hostName = newHostName;
          for( int i=0; i < 32; i++ )
            EEPROM.write( FLASHOFFSET_HOSTNAME + i,  i < hostName.length() ? hostName[i] : 0 );
            EEPROM.commit(); 
        }
      }

      if( server.hasArg( "servoLOpen"    ) )  servoObj[ SERVO_LEFT  ]->storeOpenPos(   server.arg( "servoLOpen"    ).toInt() );
      if( server.hasArg( "servoLClosed"  ) )  servoObj[ SERVO_LEFT  ]->storeClosedPos( server.arg( "servoLClosed"  ).toInt() );
      if( server.hasArg( "servoLCurrent" ) )  servoObj[ SERVO_LEFT  ]->setCurPos(      server.arg( "servoLCurrent" ).toInt() );

      if( server.hasArg( "servoROpen"    ) )  servoObj[ SERVO_RIGHT ]->storeOpenPos(   server.arg( "servoROpen"    ).toInt() );
      if( server.hasArg( "servoRClosed"  ) )  servoObj[ SERVO_RIGHT ]->storeClosedPos( server.arg( "servoRClosed"  ).toInt() );
      if( server.hasArg( "servoRCurrent" ) )  servoObj[ SERVO_RIGHT ]->setCurPos(      server.arg( "servoRCurrent" ).toInt() );

      if( server.hasArg( "servoSpeed" ) ) {
        int newServoSpeed = server.arg( "servoSpeed" ).toInt();
        if( newServoSpeed != servoSpeed ) {
          servoSpeed = newServoSpeed;
          EEPROM.write( FLASHOFFSET_SERVO_SPEED, servoSpeed );
          EEPROM.commit(); 
        }
      }

      int newLEDState = 0;
      if( server.hasArg( "LEDState" ) )
        newLEDState = server.arg( "LEDState" ).toInt();

      if( newLEDState != ledState) {
        ledState = newLEDState;
        EEPROM.write( FLASHOFFSET_LED_STATE, ledState );
        EEPROM.commit(); 
        digitalWrite( LED_BUILTIN, ledState ? LOW : HIGH );
      }

      Serial.println("HTTP: Config changed and stored");

      server.send(200, "text/html", generateConfigPage());

      // Update the servo positions
      servoObj[ SERVO_LEFT  ]->attachAndWritePos( servoObj[ SERVO_LEFT  ]->getCurPos() );
      servoObj[ SERVO_RIGHT ]->attachAndWritePos( servoObj[ SERVO_RIGHT ]->getCurPos() );

      delay( SERVO_MOVE_TIME );

      servoObj[ SERVO_LEFT  ]->detach();
      servoObj[ SERVO_RIGHT ]->detach();
    }
  });

  // Toggle LED blinking
  server.on("/config/blink", [](){
    blinkCount = 0;
    if( blinkTime == 0 ) {
      // Start
      blinkTime = millis();
    } else {
      // Stop
      digitalWrite( LED_BUILTIN, LOW );
      blinkTime = 0;
    }

    server.send(200, "text/html", generateConfigPage());
    Serial.print("HTTP: Blink LED set to ");
    Serial.println( blinkTime == 0 ? "off" : "on" );
  });

  // Open/close as fast as possible
  server.on("/config/onFast", [](){
    actionTime = millis();
    actionPos  = 100;
    server.send(200, "text/html", generateConfigPage());
    Serial.println("HTTP: config/onFast: damper opened");
  });

  server.on("/config/offFast", [](){
    actionTime  = millis();
    actionPos = 0;
    server.send(200, "text/html", generateConfigPage());
    Serial.println("HTTP: config/offFast: damper closed");
  });

  // Open/close using the current speed setting
  server.on("/config/on", [](){
    actionTime  = millis();
    actionPos   = 100;
    actionSpeed = servoSpeed;
    server.send(200, "text/html", generateConfigPage());
    Serial.println("HTTP: config/on: damper opened");
  });

  server.on("/config/off", [](){
    actionTime  = millis();
    actionPos   = 0;
    actionSpeed = servoSpeed;
    server.send(200, "text/html", generateConfigPage());
    Serial.println("HTTP: config/off: damper closed");
  });

  // Reboot, which is necessary after changing the hostname.
  server.on("/config/reboot", [](){
    openDamperTo( 0 );
    server.send(200, "text/html", rebootingPage());
    Serial.println("HTTP: config/reboot: rebooting...");
    delay( 2000 );      // Wait for the "rebooting" page to actually be sent before rebooting
    ESP.restart();
  });

  server.onNotFound( [] {
    server.send(404, "text/plain", "File Not Found\n");
  });

  server.onNotFound( [] {
    server.send(404, "text/plain", "File Not Found\n");
  });

  server.begin();
  Serial.println("- HTTP server started");
}

bool input_HTTP( char c ) {
  if( c == 'h' ) {
    showHTTPStatusRequests = !showHTTPStatusRequests;
    Serial.print( "HTTP: Status request info now " );
    Serial.println( showHTTPStatusRequests ? "visible" : "hidden" );
    return true;
  }

  return false;
}

int lastPrint = 0;
void loopWifi() {
  // OTA and web server handling
  ArduinoOTA.handle();
  server.handleClient();

  // LED blinking.  We blink up to MAX_BLINK times, going high on the odd number blink counts
  if( blinkTime != 0 ){
    if( millis() > blinkTime + BLINK_PERIOD ) {
      if( blinkCount++ > MAX_BLINKS ) {
        blinkTime = blinkCount = 0;
        digitalWrite( LED_BUILTIN, LOW );
      } else {
        blinkTime = millis();
        digitalWrite( LED_BUILTIN, blinkCount % 2 ? LOW : HIGH );
      }
    }
  }

  // Check pending damper actions
  if( (actionTime != 0) && (actionPos != -1) ) {
    // Wait for the action delay to pass
    if( (millis() - actionTime) >= ACTION_DELAY ) {
      if( actionSpeed == 255 ) {
        // Move as fast as possible
        Serial.print( "Moving damper to " );
        Serial.print( actionPos );
        Serial.print( "% open..." );
  
        openDamperTo( actionPos );
  
        Serial.println( "done." );
  
        actionTime   =  0;
        actionPosOld =  actionPos;
        actionPos    = -1;

      } else {
        // Use the given speed to move the servo more slowly
        int speedInMillis = map( actionSpeed, 255, 0, 0, SERVO_SLOWEST_SPEED );
        if( actionStartTime == 0 ) {
          // Begin moving; attach the servos
          actionStartTime = millis();
          Serial.print( "Moving damper from " );
          Serial.print( actionPosOld );
          Serial.print( "% to " );
          Serial.print( actionPos );
          Serial.print( "% open over " );
          Serial.print( speedInMillis );
          Serial.print( " ms (speed: " );
          Serial.print( actionSpeed );
          Serial.print( ")..." );
  
          openDamperTo_Attach();
        }
  
        int actionMillisDiff = millis() - actionStartTime;
        if( actionMillisDiff > speedInMillis ) {
          // Done moving
          openDamperTo_Move( actionPos );
  
          actionTime       =   0;
          actionPosOld     =   actionPos;
          actionPos        =  -1;
          actionSpeed      = 255;
          actionStartTime =    0;
  
//          Serial.printf( "  %d > %d -- end\n", actionMillisDiff, speedInMillis );
          Serial.println( "done." );
  
        } else {
          // Move the servos to the new position
          if( millis() - lastPrint > 50 ) {
            int mappedTo = map( actionMillisDiff, 0, speedInMillis, actionPosOld, actionPos );
            openDamperTo_Move( mappedTo );
  
            lastPrint = millis();
//            Serial.printf( "   %d = map( %d, 0, %d, %d, %d )\n", mappedTo, actionMillisDiff, speedInMillis, actionPosOld, actionPos );
          }
        }
      }
    }
  }
}
