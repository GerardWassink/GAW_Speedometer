/* ------------------------------------------------------------------------- *
 * Name   : GAW-Speedometer
 * Author : Gerard Wassink
 * Date   : November 2025
 * Purpose: Measure model train speed depending on scale
 *            using Infrared detectors
 * Versions:
 *   0.1  : Initial code base
 *   0.2  : Working prototype
 *   0.3  : Corrected little errors
 *   0.4  : Switched back to 16x02 LCD
 *   0.5  : Code- and timing improvements
 *   0.6  : Built in Measurement button & LED
 *          Restructured code, removed STATE machine
 *            resulting in more reliable measurements
 *   0.7  : Replaced soft reset with hard reset
 *   0.8  : Improved README, schematic added
 *   0.9  : Built in pause after showing speed
 *   1.0  : First Release
 *   1.1  : Code change, automatic measurement start
 *            for that purpose: re-instated state-machine
 *   1.2  : Built in CLI user interface for:
 *            - scale selection
 *            - entering sensor distance in mm
 *   2.0  : Final touch up for release 2.0
 *   2.1  : Added the possibillity to change the sensor treshold values
 *   2.1  : Built in F() function for literals, minor improvements
 *
 *------------------------------------------------------------------------- */
#define progVersion "2.2"                   // Program version definition 
/* ------------------------------------------------------------------------- *
 *             GNU LICENSE CONDITIONS
 * ------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ------------------------------------------------------------------------- *
 *       Copyright (C) May 2024 Gerard Wassink
 * ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- *
 *                                                        DEBUGGING ON / OFF
 * Compiler directives to switch debugging on / off
 * Do not enable DEBUG when not needed, Serial coms takes space and time!
 * ------------------------------------------------------------------------- */
#define DEBUG 0

#if DEBUG == 1
  #define debugstart(x) Serial.begin(x)
  #define debug(x) Serial.print(x)
  #define debugln(x) Serial.println(x)
#else
  #define debugstart(x)
  #define debug(x)
  #define debugln(x)
#endif


/* ------------------------------------------------------------------------- *
 *                                             Include headers for libraries
 * ------------------------------------------------------------------------- */
#include <Wire.h>                           // I2C comms library
#include <LiquidCrystal_I2C.h>              // LCD library
#include <EEPROM.h>                         // EEPROM library


/* ------------------------------------------------------------------------- *
 *                                                           Pin definitions
 * ------------------------------------------------------------------------- */
#define leftSensor       A2                 // Input pins
#define rightSensor      A3                 //   from IR detectors

#define startConfig      PD2                // Button to start configuration

#define leftDetection    PD3                // LED left detected
#define rightDetection   PD4                // LED right detected


/* ------------------------------------------------------------------------- *
 *                                                State definitions for loop
 * ------------------------------------------------------------------------- */
#define booting          0                  // Perform soft boot
#define waiting          1                  // Wait for events
#define detectedLeft     2                  // Left sensor activated
#define detectedRight    3                  // Right sensor activated
#define buttonPressed    4                  // start CLI configuration

int state               = booting;          // Initial state

/* ------------------------------------------------------------------------- *
 *                                          Create object for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display1(0x27, 16, 2);    // Initialize display1 object


/* ------------------------------------------------------------------------- *
 *       Create structure and object for settings to store them to EEPROM
 * ------------------------------------------------------------------------- */
struct Settings {
  int   senseDistance;                       // Distance between sensors
  int   selectedScale;                       // The scale the user selected
  int   leftTreshold;                        // treshold value for left sensor
  int   rightTreshold;                       // treshold value for right sensor
};
Settings mySettings;                         // Create the object


/* ------------------------------------------------------------------------- *
 *                                  Constants and variables for measurements
 * ------------------------------------------------------------------------- */
float         sensorDistance  = 200;        // Distance in mm
unsigned long detectionTime   = 0;          // Time between detections
float         scaleFactor     = 0.0;        // Scale Factor, default N-scale
float         realDistance    = 0.0;        // Holds calculated distance
float         realSpeed       = 0.0;        // Holds calculated speed
int           tresholdLeft    = 200;        // Working value for left sensor
int           tresholdRight   = 200;        // Working value for right sensor


/* ------------------------------------------------------------------------- *
 *                                            Array of NMRA scales and names
 * ------------------------------------------------------------------------- */
float scales[8]        = {45.2,             // for O(17) scale
                          48.0,             // for O, On3, On2 scale
                          64.0,             // for Sn3, S scale
                          76.0,             // for OO scale
                          87.0,             // for HO scale
                          120.0,            // for TT scale
                          160.0,            // for N, Nn3 scale
                          220.0             // for Z scale
                         };

char  scaleName[8][12] = {"O(17)",          // NMRA descriptions
                          "O, On3, On2", 
                          "Sn3, S", 
                          "OO", 
                          "HO", 
                          "TT", 
                          "N, Nn3", 
                          "Z"
                         };
                         
int   scalePtr = 6;                         // set default scale here


/* ------------------------------------------------------------------------- *
 *                                            Variables for timing detection
 * ------------------------------------------------------------------------- */
unsigned long leftMillis  = 0;
unsigned long rightMillis = 0;




/* ------------------------------------------------------------------------- *
 *       Initialization routine                                      setup()
 * ------------------------------------------------------------------------- */
void setup() {
  Serial.begin(115000);

  Wire.begin();                             // Start I2C

                                            // Initialize display backlight 
  display1.init();                          //   on by default
  display1.backlight();

  pinMode(leftDetection, OUTPUT);
  pinMode(rightDetection, OUTPUT);

  pinMode(startConfig, INPUT_PULLUP);

  Serial.println();
  Serial.print(F("---===### "));
  Serial.print(F("GAW_Speedometer v"));
  Serial.print(progVersion);
  Serial.println(F(" ###===---"));
  Serial.println();

//  storeSettings();                          // uncomment to store initial settings
  getSettings();                            // Get settings from EEPROM

  LCD_display(display1, 0, 0, F("GAW_Speedometer "));
  LCD_display(display1, 1, 0, F("  Version       "));
  LCD_display(display1, 1,10, String(progVersion));

  delay(1500);                              // show for 1,5 second

  state = booting;                          // do softboot first

}



/* ------------------------------------------------------------------------- *
 *       Software initialization routine                          softBoot()
 * ------------------------------------------------------------------------- */
void softBoot() {

                        // Initial text on display
  LCD_display(display1, 0, 0, F("Speed:          "));
  LCD_display(display1, 1, 0, F("Scale:           ") );
  LCD_display(display1, 1, 7, scaleName[scalePtr]);

  digitalWrite(leftDetection,  LOW);
  digitalWrite(rightDetection, LOW);

  scaleFactor = scales[scalePtr];
  realDistance = sensorDistance * scaleFactor / 1000.0;

  Serial.println();
  Serial.println(F("---===### Ready for operation  ###===---"));
  Serial.println();

  state = waiting;

}



/* ------------------------------------------------------------------------- *
 *       Repeating loop                                               loop()
 * ------------------------------------------------------------------------- */
void loop() {

  switch (state) {

    case booting:                           // Re-initialize
      softBoot();
      break;

    case waiting:                           // Wait for action

      if (analogRead(leftSensor) < tresholdLeft) {   // Left sensor detected
        state = detectedLeft;
        break;
      }

      if (analogRead(rightSensor) < tresholdRight) {  // Right sensor detected
        state = detectedRight;
        break;
      }

      if (!(digitalRead(startConfig)) ) {   // Config button pressed
        state = buttonPressed;
      }

      break;

    case detectedLeft:
      leftToRight();                        // Handle left to right traffic
      softBoot();                           // soft reboot
      state = waiting;
      break;

    case detectedRight:
      rightToLeft();                        // Handle right to left traffic
      softBoot();                           // soft reboot
      state = waiting;
      break;

    case buttonPressed:
      configMenu();                         // Go config on CLI
      state = booting;                      // soft reboot
      break;

  }

}



/* ------------------------------------------------------------------------- *
 *       Measure leftToRight                                   leftToRight()
 * ------------------------------------------------------------------------- */
void leftToRight() {

debug(F("DetectedLeft, waitForRight - "));

  digitalWrite(leftDetection, HIGH);        // indicate left detected
  leftMillis = millis();

  do {
  } while ( analogRead(rightSensor) > tresholdRight ); // wait for right detection

  detectionTime = millis() - leftMillis;
  digitalWrite(rightDetection, HIGH);       // indicate right detected

  showSpeed();                              // show result

}



/* ------------------------------------------------------------------------- *
 *       Measure rightToLeft                                   RightToleft()
 * ------------------------------------------------------------------------- */
void rightToLeft() {

debug(F("DetectedRight, waitForLeft - "));

  digitalWrite(rightDetection, HIGH);       // indicate right detected
  rightMillis = millis();

  do {
  } while ( analogRead(leftSensor) > tresholdLeft ); // wait for left detection

  detectionTime = millis() - rightMillis;
  digitalWrite(leftDetection, HIGH);        // indicate left detected

  showSpeed();                              // show result

}



/* ------------------------------------------------------------------------- *
 *       Calculate and show RealSpeed                            showSpeed()
 * ------------------------------------------------------------------------- */
void showSpeed() {

debug(F("Time: "));
debug(String(detectionTime));

                                            // Calculate speed
  realSpeed = (realDistance * 3.6) / (detectionTime / 1000.0);

debug(F(" - Speed: "));
debugln(String(realSpeed));

                                            // Show speed
  LCD_display(display1, 0, 0, F("Speed:          "));
  LCD_display(display1, 0, 7, String(realSpeed) );
  LCD_display(display1, 0,13, F("Kmh"));

  delay(5000);                              // show for 5 seconds

}


/* ------------------------------------------------------------------------- *
 *       Start command line interface (CLI)                     configMenu()
 * ------------------------------------------------------------------------- */
void configMenu(){
  int save_int, stop, choice;
  float save_float, new_float;
  int save_left, new_left, save_right, new_right;

  do {
    Serial.println();
    Serial.println(F("|****************************|"));
    Serial.println(F("|**   Configuration menu   **|"));
    Serial.println(F("|****************************|"));
    Serial.println();
    Serial.println(F("Select one of the following options:"));
    Serial.println(F("1 Select scale"));
    Serial.println(F("2 Set sensor distance"));
    Serial.println(F("3 Set treshold for left sensor"));
    Serial.println(F("4 Set treshold for right sensor"));
    Serial.println(F("X Exit configuration menu"));
    Serial.println();

    do {
      stop = 0;
      choice = toupper(Serial.read());
      switch(choice) {

        case '1':
          save_int = scalePtr;
          chooseScale();                    // Ask for new scale
          if (scalePtr != save_int) {       // only store when changed
            storeSettings();
          }
          stop = 1;
          continue;

        case '2':
          save_float = sensorDistance;
          new_float = getSensorDistance();  // Ask for new sensor distance
          if (new_float != save_float) {    // only store when changed
            sensorDistance = new_float;
            storeSettings();
          }
          stop = 1;
          break;

        case '3':
          save_left = tresholdLeft;
          new_left = getLeftTreshold();     // Ask for new value for left sensor
          if (new_left != save_left) {      // only store when changed
            tresholdLeft = new_left;
            storeSettings();
          }
          stop = 1;
          break;

        case '4':
          save_right = tresholdRight;
          new_right = getRightTreshold();     // Ask for new value for left sensor
          if (new_right != save_right) {      // only store when changed
            tresholdRight = new_right;
            storeSettings();
          }
          stop = 1;
          break;

        case 'X':
debugln(F("Leaving configuration menu"));
          stop = 1;
          continue;

        default:
          break;
      }

    } while(stop==0);

  } while(choice != 'X');

 }


/* ------------------------------------------------------------------------- *
 *       Get new treshold for left sensor                  getLeftTreshold()
 * ------------------------------------------------------------------------- */
int getLeftTreshold() {
  int treshold;
  bool valid = false;

  Serial.println();
  Serial.println(F("|****************************|"));
  Serial.println(F("|**   Set treshold for     **|"));
  Serial.println(F("|**   left sensor          **|"));
  Serial.println(F("|****************************|"));
  Serial.println(F(""));
  Serial.print(F("Current treshold for left sensor = "));
  Serial.println(tresholdLeft);
  Serial.println();
  Serial.print(F("Specify desired treshold: "));

  do {
    if (Serial.available()) {
      treshold = Serial.parseInt();
      if (treshold != 0) {
        valid = true;
        Serial.print("You entered: ");
        Serial.println(treshold);
      }
    }
  } while(!valid);

  return(treshold);
}


/* ------------------------------------------------------------------------- *
 *       Get new treshold for right sensor                getRightTreshold()
 * ------------------------------------------------------------------------- */
int getRightTreshold() {
  int treshold;
  bool valid = false;

  Serial.println();
  Serial.println(F("|****************************|"));
  Serial.println(F("|**   Set treshold for     **|"));
  Serial.println(F("|**   right sensor         **|"));
  Serial.println(F("|****************************|"));
  Serial.println();
  Serial.print(F("Current treshold for right sensor = "));
  Serial.println(tresholdRight);
  Serial.println("");
  Serial.print(F("Specify desired treshold: "));

  do {
    if (Serial.available()) {
      treshold = Serial.parseInt();
      if (treshold != 0) {
        valid = true;
        Serial.print(F("You entered: "));
        Serial.println(treshold);
      }
    }
  } while(!valid);

  return(treshold);
}


/* ------------------------------------------------------------------------- *
 *       Get new sensor distance in mm                   getSensorDistance()
 * ------------------------------------------------------------------------- */
float getSensorDistance() {
  float dist_float;
  bool valid = false;

  Serial.println();
  Serial.println(F("|****************************|"));
  Serial.println(F("|**   Set sensor distance  **|"));
  Serial.println(F("|**   in millimeters       **|"));
  Serial.println(F("|****************************|"));
  Serial.println();
  Serial.print(F("Current sensor distance = "));
  Serial.println(sensorDistance);
  Serial.println("");
  Serial.print(F("Specify desired sensor distance: "));

  do {
    if (Serial.available()) {
      dist_float = Serial.parseFloat();
      if (dist_float != 0) {
        valid = true;
        Serial.print(F("You entered: "));
        Serial.println(dist_float);
      }
    }
  } while(!valid);

  return(dist_float);
}


/* ------------------------------------------------------------------------- *
 *       Choose scale                                          chooseScale()
 * ------------------------------------------------------------------------- */
void chooseScale() {

  int choice;

  do {
    Serial.println();
    Serial.println(F("|****************************|"));
    Serial.println(F("|**   Scale selection      **|"));
    Serial.println(F("|****************************|"));
    Serial.println();
    Serial.println(F("Select a scale by choosing the number:"));
    Serial.println();

    for (int i=0; i<=7; i++) {
      Serial.print(i);
      Serial.print(" - ");
      Serial.println( scaleName[i] );
    }
    Serial.println(F("X - Leave"));

    for (;;) {
      choice = toupper(Serial.read());

      switch(choice) {

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          scalePtr = choice-48;             // convert char to int

          Serial.print(F("   Selected scale: "));
          Serial.print(scalePtr);
          Serial.print(F(" - "));
          Serial.println( scaleName[scalePtr] );

          scaleFactor = scales[scalePtr];   // re-calculate realDistance
          realDistance = sensorDistance * scaleFactor / 1000.0;

          LCD_display(display1, 1, 0, F("Scale:          ") );
          LCD_display(display1, 1, 7, scaleName[scalePtr]);
debugln( F("Leaving scale selection") );
          return;

        case 'x':
        case 'X':
debugln( F("Leaving scale selection") );
          return;

        default:
          continue;
      } 
    }
  } while(choice != 'X');
}


/* ------------------------------------------------------------------------- *
 *       Store settings to EEPROM                            storeSettings()
 * ------------------------------------------------------------------------- */
void storeSettings() {

  Serial.println();
  Serial.println(F("Store settings to EEPROM"));
                                            // Put settings in 
                                            //  mySettings structure
  mySettings.senseDistance = sensorDistance;
  mySettings.selectedScale = scalePtr;
  mySettings.leftTreshold  = tresholdLeft;
  mySettings.rightTreshold = tresholdRight;

                                            // Store mySettings structure 
                                            //  to EEPROM
  EEPROM.put(0, mySettings);

debug(F("Stored: selected scale ("));
debug(scaleName[scalePtr]);
debug(F(") - and sensor distance ("));
debug(mySettings.senseDistance);
debugln(F(")"));

debug(F("Stored: treshold left ("));
debug(String(tresholdLeft));
debug(F(") and treshold right ("));
debug(String(tresholdRight));
debugln(F(")"));
debugln();

}



/* ------------------------------------------------------------------------- *
 *       Retrieve settings from EEPROM                         getSettings()
 * ------------------------------------------------------------------------- */
void getSettings() {

  Serial.println(F("Retrieving settings from EEPROM"));
  
                                            // Get mySettings structure 
                                            //  from EEPROM
  EEPROM.get(0, mySettings);

                                            // Get settings from 
                                            //  mySettings structure
  sensorDistance = mySettings.senseDistance;
  scalePtr       = mySettings.selectedScale;
  tresholdLeft   = mySettings.leftTreshold;
  tresholdRight   = mySettings.rightTreshold;

debug(F("Retrieved Sensor distance in mm: "));
debugln( String(sensorDistance) );
debug(F("Retrieved Scale: "));
debug( String(scalePtr));
debug(F(" being: "));
debugln(scaleName[scalePtr]);

debug(F("Retrieved treshold for left sensor: "));
debugln( String(tresholdLeft) );
debug(F("Retrieved treshold for right sensor: "));
debugln( String(tresholdRight) );

}


/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
  screen.setCursor(col, row);
  screen.print(text);
}
