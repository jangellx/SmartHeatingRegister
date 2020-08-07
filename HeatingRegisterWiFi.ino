#include <EEPROM.h>
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// Flash Memory Info
//  We store basic settings in flash memory, such as servo open/closed positions,
//  servo speed and the hostname.
// Key
//  U1: Unsigned Byte
//  Sx: String of x bytes
// Layout
//   Offset / Datatype and Size / Description
//   0  U1  Left Servo Open Position, 255 for not set
//   1  U1  Left Servo Closed Position, 255 for not set
//   2  U1  Right Servo Open Position, 255 for not set
//   3  U1  Right Servo Closed Position, 255 for not set
//   4  U1  Servo Speed.  255 is fastest, 0 is slowest
//   5  S32 Hostname (32 bytes)
//  37  U1  LED State.  1 for on, 0 for off.

#define FLASHOFFSET_SERVO_LEFT    0
#define FLASHOFFSET_SERVO_RIGHT   2
#define FLASHOFFSET_SERVO_SPEED   4
#define FLASHOFFSET_HOSTNAME      5
#define FLASHOFFSET_LED_STATE     37

int     ledState = 1;    // If the LED should be normally on or off.

void setup() {
  delay( 250 );   // Wait until hardware noise is clear before outputting to the serial port

  Serial.begin( 115200 );
  Serial.println( "\n...Starting up..." );

  pinMode(      LED_BUILTIN, OUTPUT );
  digitalWrite( LED_BUILTIN, LOW    );

  // Initialize the EEPROM emulation on the ESP8266.  64 bytes is more than enough.
  EEPROM.begin( 64 );

  // Set the LED state
  ledState = EEPROM.read( FLASHOFFSET_LED_STATE ) ? 1 : 0;

  digitalWrite( LED_BUILTIN, ledState ? LOW : HIGH );

  // Init other systems
  initServos();
  initWifi();

  Serial.println( "Ready.\n" );
}

void loop() {
  int    moved;

  // Handle wifi/web server stuff
  loopWifi();

  // handle serial input for calibration and testing
  if( Serial.available() ) {
    int c = Serial.read();

    if( !input_HTTP( c ) ) {
      input_Mode( c );
      moved = input_EditMode( c ) ;
      moved = moved ? true : input_GoMode( c );
  
      servoInfoMovedPrintResult( moved );
    }
  }
}
