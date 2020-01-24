// Servo defines
#define PIN_SERVO_L   14  // GPIO14, D5, servo closest to the microcontroller
#define PIN_SERVO_R   12  // GPIO12, D6, other servo, rotating the other direction

#define SERVO_LEFT    0   // Left servo array index.
#define SERVO_RIGHT   1   // Right servo array index
#define SERVO_NONE   -1   // Invalid index

#define SERVO_MOVE_TIME       333    // Time in milliseconds to wait after moving the servo before detatching from it

#define POS_NONE              255    // Indicates no position is stored

#define SERVO_MAX             254

#define SERVO_SLOWEST_SPEED  5000   // Slowest speed the servo can move, in milliseconds
int servoSpeed = 255;   // Speed to move the servos at.  0 is the slowest, 255 is the fastests

// Note that the left servo will have an open value that is lower than the closed value (ie: 20 and 60).
//  The right servo will be the other way around, as it turns the opposite direction due to how it is
//  mounted.

// Class storing the servo state (pin, position, extents, object, etc),
//  and providing servo control.
class ServoState {
  public: 
    ServoState( int _pin, int startAddr ) {
      pin = _pin;
      openAddr    = startAddr;
      closedAddr  = startAddr + 1;

      openPos   = POS_NONE;
      closedPos = POS_NONE;
    }

    // Load from the EEPROM.  If the value is 255, the open/closed position wasn't stored yet.
    void loadFromEEPROM( bool loadAndUse=true ) {
      int o = EEPROM.read( openAddr );
      if( o != POS_NONE )
        o = constrain( o, 0, SERVO_MAX );

      int c = EEPROM.read( closedAddr );
      if( c != POS_NONE )
        c = constrain( c, 0, SERVO_MAX );

      if( loadAndUse ) {
        openPos   = o;
        closedPos = c;
      }

      Serial.print( pin == PIN_SERVO_L ? "- Left " : "- Right" );
      Serial.print( " Servo Initialized; " );
      if( o == POS_NONE ) {
        Serial.print( "No open position" );
      } else {
        Serial.print( "Open position " );
        Serial.print( o );
      }

      if( c == POS_NONE ) {
        Serial.print( ", No closed position" );
      } else {
        Serial.print( ", Closed position " );
        Serial.print( c );
      }

      Serial.println( "" );
    }

    // Store the open/closed positions
    void storeOpenPos( int pos=POS_NONE ) {
      if( pos == openPos )
        return;

      openPos = (pos == POS_NONE) ? curPos : constrain( pos, 0, SERVO_MAX );
      EEPROM.write( openAddr, openPos );
      EEPROM.commit(); 
    }

    void storeClosedPos( int pos=POS_NONE ) {
      if( pos == closedPos )
        return;

      closedPos = (pos == POS_NONE) ? curPos : constrain( pos, 0, SERVO_MAX );
      EEPROM.write( closedAddr, closedPos );
      EEPROM.commit(); 
    }

    // Set the current position, but do not change the servo position to it
    void setCurPos( int pos=POS_NONE ) {
      if( pos == curPos )
        return;

      if( pos == POS_NONE )
        curPos = POS_NONE;
      else
        curPos = constrain( pos, 0, SERVO_MAX );
    }

    // Attach to the servo and write the position.  Does nothing
    //  if pos is 255 (POS_NONE, which means no value).
    void attachAndWritePos( int pos, const char *sentFrom=NULL ) {
      if( pos == POS_NONE )
        return;

      curPos = constrain( pos, 0, SERVO_MAX );

      this->attach();
      this->writePos( curPos, sentFrom );
    }

    // Same, but as a percentage (0.0-1.0)
    void attachAndWritePercent( float percent, const char *sentFrom=NULL ) {
      attachAndWritePos( map( (int)(percent * (float)(SERVO_MAX)), 0, SERVO_MAX, closedPos, openPos ), sentFrom );
    }

    // Just write the position to the servo.  Does nothing if not attached
    //  or pos is POS_NONE.
    void writePos( int pos, const char *sentFrom=NULL ) {
      if( pos == POS_NONE )
        return;

      curPos = constrain( pos, 0, SERVO_MAX );

      #ifdef DEBUG_WRITE_SERVO
        Serial.print( pin == PIN_SERVO_L ? "** Left: Writing pos via " : "** Right: Writing pos via " );
        Serial.print( sentFrom == NULL ? "(unknown)" : sentFrom );
        Serial.print( ": " );
        Serial.println( curPos );
      #endif

     // Flip for the right servo
     int c = curPos;
     if( pin == PIN_SERVO_R )
        c = SERVO_MAX - c;

      servo.write( c );
    }
 
    // Attach to the servo
    void attach() {
      if( !servo.attached() )
        servo.attach( pin );
    }
    
    // Detach from the servo
    void detach() {
      if( servo.attached() )
        servo.detach();
    }

    // Output current info about the servo to the serial port
    void dumpInfo() {
      Serial.print( "Info (" );
      Serial.print( pin == PIN_SERVO_L ? "Left" : "Right" );
      Serial.println( ")" );
      Serial.print( "  Open position:    " );
      Serial.println( getOpenPos() );
      Serial.print( "  Closed position:  " );
      Serial.println( getClosedPos() );
      Serial.print( "  Current position: " );
      Serial.println( getCurPos() );
      Serial.print( "  Attached:         " );
      Serial.println( isAttached() ? "Yes" : "No" );
    }

    // Accessors
    int getCurPos()    { return constrain(    curPos, 0, SERVO_MAX ); }
    int getOpenPos()   { return constrain(   openPos, 0, SERVO_MAX ); }
    int getClosedPos() { return constrain( closedPos, 0, SERVO_MAX ); }

    int hasOpenPos()   { return (  openPos != POS_NONE); }
    int hasClosedPos() { return (closedPos != POS_NONE); }

    int isAttached()   { return servo.attached(); }

  private:
    // Variables
    int   pin;                                                 // Servo pin
    int   curPos, openPos, closedPos;                          // Current, open and closed position from 0-SERVO_MAX, or POS_NONE if not stored yet
  
    int   openAddr, closedAddr;                                // EEPROM memory addresses for the opened and closed states
  
    Servo servo;                                               // Servo object
};

// Instances of the servo objects
ServoState *servoObj[2] = { NULL, NULL };

// Initialize the servos.  Called from setup().
void initServos() {
  Serial.println( "Servo init" );

  // Load the servo speed
  servoSpeed = EEPROM.read( FLASHOFFSET_SERVO_SPEED );
    
  // Initialize the servo array.  Each requires a pin and two consecutive EEPROM addresses 
  servoObj[ SERVO_LEFT  ] = new ServoState( PIN_SERVO_L, FLASHOFFSET_SERVO_LEFT );
  servoObj[ SERVO_RIGHT ] = new ServoState( PIN_SERVO_R, FLASHOFFSET_SERVO_RIGHT );

  // Read the min/max from the EEPROM
  servoObj[ SERVO_LEFT ]->loadFromEEPROM();
  servoObj[ SERVO_RIGHT ]->loadFromEEPROM();

  // Go to the 50% position
  servoObj[ SERVO_LEFT  ]->attachAndWritePercent( 0.5, "initServos" );
  servoObj[ SERVO_RIGHT ]->attachAndWritePercent( 0.5, "initServos" );

  delay( SERVO_MOVE_TIME );
  
  servoObj[ SERVO_LEFT  ]->detach();
  servoObj[ SERVO_RIGHT ]->detach();

  Serial.println( "- Servos moved to 50% position" );
}

// Move the damper to a given percentage of open.  0 is closed, 100 is open.
void openDamperTo_Attach() {
  servoObj[ SERVO_LEFT  ]->attach();
  servoObj[ SERVO_RIGHT ]->attach( );
}

void openDamperTo_Move( int pos ) {
  int curPos[2] = { POS_NONE, POS_NONE };

  // Figure out the position
  curPos[ SERVO_LEFT  ] = map( pos, 0, 100, servoObj[ SERVO_LEFT  ]->getClosedPos(), servoObj[ SERVO_LEFT  ]->getOpenPos() );
  curPos[ SERVO_RIGHT ] = map( pos, 0, 100, servoObj[ SERVO_RIGHT ]->getClosedPos(), servoObj[ SERVO_RIGHT ]->getOpenPos() );

  // Attach and send the position
  servoObj[ SERVO_LEFT  ]->writePos( curPos[ SERVO_LEFT  ], "openDamperTo" );
  servoObj[ SERVO_RIGHT ]->writePos( curPos[ SERVO_RIGHT  ], "openDamperTo" );
}

void openDamperTo_Detach() {
  // Detach from the servos to avoid annoying buzzing sounds
  servoObj[ SERVO_LEFT  ]->detach();
  servoObj[ SERVO_RIGHT ]->detach();
}

// Do the above in one call.
void openDamperTo( int pos ) {
  // Attach and send the position
  openDamperTo_Attach();
  openDamperTo_Move( pos );

  // Wait for the servos to finish moving
  delay( SERVO_MOVE_TIME );

  // Detach
  openDamperTo_Detach();
}

// Return true if either servo is currently not in the closed position.
int damperIsOpen() {
  if( servoObj[ SERVO_LEFT  ]->getCurPos() != servoObj[ SERVO_LEFT  ]->getClosedPos() )   return 1;
  if( servoObj[ SERVO_RIGHT ]->getCurPos() != servoObj[ SERVO_RIGHT ]->getClosedPos() )   return 1;

  return 0;
}

// Calibration and Testing Input
//  Go Modes:
//   g:  Go mode.  0-9 go from stored closed to open for both servos
//   o, p:  Go to stored open position
//   c, x:  Go to stored closed position
//  Configuration Modes:
//   l, L:  Configure left servo.  0-9 go from 0 to SERVO_MAX degrees
//   r, R:  Configure right servo.
//   -:  Rotate -1 degree
//   _:  Rotate -5 degrees
//   =:  Rotate 1 degree
//   +:  Rotate 5 degrees
//   o:  Go to the stored open position
//   c:  Go to the stored closed position
//   O:  Store the open position
//   C:  Store the closed position

// Press g to go into Go mode.  Press L or R to go into Calibrate mode for that servo.
//  The other commands will then edit that servo.

ServoState *calibrateServo = NULL;   // NULL in go mode; otherwise, the servo to be calibrated

#define IN_GO_MODE (calibrateServo == NULL)

// Check for a mode switch (go, calibrate left or calibrate right)
void input_Mode( char c ) {

  switch( c ) {
    case 'g':
      if( calibrateServo != NULL )
        calibrateServo->detach();

      calibrateServo = NULL;
      Serial.println( "-- Go mode --" );
      break;

    case 'L':
    case 'l':
      if( calibrateServo != NULL )
        calibrateServo->detach();

      calibrateServo = servoObj[ SERVO_LEFT  ];
      Serial.println( "-- Calibrate Left --" );
      break;
    
    case 'R':
    case 'r':
      if( calibrateServo != NULL )
        calibrateServo->detach();

      calibrateServo = servoObj[ SERVO_RIGHT ];
      Serial.println( "-- Calibrate Right --" );
      break;
  }
}

// Go mode inputs
bool input_GoMode( char c ) {
  if( !IN_GO_MODE )
    return false;

  int curPos[2] = { POS_NONE, POS_NONE };

  switch( c ) {
    // Go to stored close positin
    case 'c':
    case 'x':
      Serial.println("(g) Moving to closed position" );
      curPos[ SERVO_LEFT  ] = servoObj[ SERVO_LEFT  ]->getClosedPos();
      curPos[ SERVO_RIGHT ] = servoObj[ SERVO_RIGHT ]->getClosedPos();
      break;

    // Go to or store open position
    case 'o':
    case 'p':
      Serial.println("(g) Moving to open position" );
      curPos[ SERVO_LEFT  ] = servoObj[ SERVO_LEFT  ]->getOpenPos();
      curPos[ SERVO_RIGHT ] = servoObj[ SERVO_RIGHT ]->getOpenPos();
      break;

    // Specific Position between open and closed position
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      Serial.print(   "(g) Moving to " );
      Serial.print(   (c - '0') * 10 );
      Serial.println( "% position" );
      curPos[ SERVO_LEFT  ] = map( (c - '0'), 0, 9, servoObj[ SERVO_LEFT  ]->getOpenPos(), servoObj[ SERVO_LEFT  ]->getClosedPos() );
      curPos[ SERVO_RIGHT ] = map( (c - '0'), 0, 9, servoObj[ SERVO_RIGHT ]->getOpenPos(), servoObj[ SERVO_RIGHT ]->getClosedPos() );
      break;

    case 'i':
      servoObj[ SERVO_LEFT  ]->dumpInfo();
      servoObj[ SERVO_RIGHT ]->dumpInfo();
      break;
  }

  // See if we got any input
  if( curPos[ SERVO_LEFT ] == POS_NONE )
    return false;

  // Store and move to the new position
  servoObj[ SERVO_LEFT  ]->attachAndWritePos( curPos[ SERVO_LEFT  ], "Go Mode" );
  servoObj[ SERVO_RIGHT ]->attachAndWritePos( curPos[ SERVO_RIGHT ], "Go Mode" );

  delay( SERVO_MOVE_TIME );
  
  servoObj[ SERVO_LEFT  ]->detach();
  servoObj[ SERVO_RIGHT ]->detach();

  return true;
}

bool input_EditMode( char c ) {
  if( IN_GO_MODE )
    return false;

//  Serial.print( "(calibrate " );
//  Serial.print( (calibrateServo == servoObj[ SERVO_LEFT ]) ? "left" : "right" );
//  Serial.println( ")" );

  int curPos = calibrateServo->getCurPos();

  // Test the input
  switch( c) {
    // Close
    case 'c':   curPos = constrain( curPos+1, 0, SERVO_MAX );    Serial.print( "(c) +1; now " );  Serial.println( curPos );  break;
    case 'C':   curPos = constrain( curPos+5, 0, SERVO_MAX );    Serial.print( "(c) +5; now " );  Serial.println( curPos );  break;

    // Open
    case 'o':   curPos = constrain( curPos-1, 0, SERVO_MAX );    Serial.print( "(c) -1; now " );  Serial.println( curPos );  break;
    case 'O':   curPos = constrain( curPos-5, 0, SERVO_MAX );    Serial.print( "(c) -5; now " );  Serial.println( curPos );  break;

    // Go to or store close position
    case 'x':   curPos = calibrateServo->getClosedPos();   Serial.print(   "(c) Moving to closed position " );  Serial.println( curPos );  break;
    case 'X':   calibrateServo->storeClosedPos( curPos );  Serial.println( "(c) Closed position stored" );   break;

    // Go to or store open position
    case 'p':   curPos = calibrateServo->getOpenPos();     Serial.print(   "(c) Moving to open position "  );  Serial.println( curPos );  break;
    case 'P':   calibrateServo->storeOpenPos( curPos );    Serial.println( "(c) Open position stored" );   break;

    // Specific Position
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      curPos = map( (c - '0'), 0, 9, 0, SERVO_MAX );
      Serial.print( "(c): Changing current positon to " );
      Serial.print( c );
      Serial.print( ": " );
      Serial.println( curPos );
      break;

     case 'a':
      calibrateServo->attach();
      Serial.println( "(c) Servo attached" );
      break;

     case 'd':
      calibrateServo->detach();
      Serial.println( "(c) Servo detached" );
      break;

    case 't':
      Serial.println( calibrateServo->isAttached() ? "(c) Currently attached" : "(c) Not currently attached" );
      break;

   case 'e':
      Serial.println( "Reloading EEPROM Values" );
      servoObj[ SERVO_LEFT ]->loadFromEEPROM( true );
      servoObj[ SERVO_RIGHT ]->loadFromEEPROM( true);
      break;

   case 'E':
      Serial.println( "Loading EEPROM Values (view only)" );
      servoObj[ SERVO_LEFT ]->loadFromEEPROM( false );
      servoObj[ SERVO_RIGHT ]->loadFromEEPROM( false );
      break;

  case  'i':
      calibrateServo->dumpInfo();
      break;
  }

  if( curPos == POS_NONE )
    return false;

  // Store and move to the new position
  bool wasAttached = false;
  if( calibrateServo->isAttached() ) {
    wasAttached = true;
    calibrateServo->writePos( curPos, "Calirbate (attached)" );
  } else {
    calibrateServo->attachAndWritePos( curPos, "Calirbate (normal)" );
  }

  delay( SERVO_MOVE_TIME );

  if( !wasAttached )
    calibrateServo->detach();

  return true;
}

// If moved is true, print the current position to the serial output
void servoInfoMovedPrintResult( int moved ) {
  if( !moved )
    return;

  Serial.print( "New position:  " );
  if( IN_GO_MODE ) {
    Serial.print( "L:  " );
    Serial.print( servoObj[ SERVO_LEFT  ]->getCurPos() );
    Serial.print( "  R: ");
    Serial.println( servoObj[ SERVO_RIGHT ]->getCurPos() );

  } else {
    Serial.println( calibrateServo->getCurPos()  );
  }
}
