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
 *
 *------------------------------------------------------------------------- */
#define progVersion "1.2"                   // Program version definition 
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
#define DEBUG 1

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

#define scaleSelection   PD2                // Button to browse/select scale

#define leftDetection    PD3                // LED left detected
#define rightDetection   PD4                // LED right detected


/* ------------------------------------------------------------------------- *
 *                                                         State definitions
 * ------------------------------------------------------------------------- */
#define booting          0                  // From startup to ready
#define waiting          1                  // Startup ready
#define detectedLeft     2                  // Left sensor activated
#define detectedRight    3                  // Right sensor activated
#define buttonPressed    4                  // start CLI

int state               = booting;          // Initial state

/* ------------------------------------------------------------------------- *
 *                                          Create object for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display1(0x27, 16, 2);    // Initialize display1 object


/* ------------------------------------------------------------------------- *
 *       Create structure and object for settings to store them to EEPROM
 * ------------------------------------------------------------------------- */
struct Settings {
  int  senseDistance;                       // Distance between sensors
  int  selectedScale;                       // The scale the user selected
};
Settings mySettings;                        // Create the object


/* ------------------------------------------------------------------------- *
 *                                  Constants and variables for measurements
 * ------------------------------------------------------------------------- */
float sensorDistance  = 200.0;              // Distance in mm
long  detectionTime   = 0;                  // Time between detections
float scaleFactor     = 0.0;                // Scale Factor, default N-scale
float realDistance    = 0.0;                // Holds calculated distance
float realSpeed       = 0.0;                // Holds calculated speed


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
  Serial.begin(115000);                     // make debug output fast

  Wire.begin();                             // Start I2C

                                            // Initialize display backlight 
                                            //   on by default
  display1.init(); display1.backlight();

  pinMode(leftDetection, OUTPUT);
  pinMode(rightDetection, OUTPUT);

  pinMode(scaleSelection, INPUT_PULLUP);

debug(F("GAW_Speedometer v"));
debugln(progVersion);

/* ------- Uncomment this line one time, to store initial values ----------- */
//  storeSettings();                          // Store settings to EEPROM

  getSettings();                            // Get settings from EEPROM

  LCD_display(display1, 0, 0, F("GAW_Speedometer "));
  LCD_display(display1, 1, 0, F("  Version       "));
  LCD_display(display1, 1,10, String(progVersion));

  delay(1500);

  state = booting;

}



/* ------------------------------------------------------------------------- *
 *       Software initialization routine                          softBoot()
 * ------------------------------------------------------------------------- */
void softBoot() {

                        // Initial text on display
  scaleFactor = scales[scalePtr];

  LCD_display(display1, 0, 0, F("Speed:          "));
  LCD_display(display1, 1, 0, F("Scale:           ") );
  LCD_display(display1, 1, 7, scaleName[scalePtr]);

  digitalWrite(leftDetection,  LOW);
  digitalWrite(rightDetection, LOW);

  realDistance = sensorDistance * scaleFactor / 1000.0;

  state = waiting;

  Serial.println();
  Serial.println("---===###   Ready for operations   ###===---");
  Serial.println();

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

      if (analogRead(leftSensor) < 200) {   // Left sensor detected
        state = detectedLeft;
        break;
      }

      if (analogRead(rightSensor) < 200) {  // Right sensor detected
        state = detectedRight;
        break;
      }

      if (!(digitalRead(scaleSelection)) ) {  // Select scale
        state = buttonPressed;
      }

      break;

    case detectedLeft:                      // Handle left to right traffic
      leftToRight();
      softBoot();                           // soft reboot
      state = waiting;
      break;

    case detectedRight:                     // Handle right to left traffic
      rightToLeft();
      softBoot();                           // soft reboot
      state = waiting;
      break;

    case buttonPressed:                     // choose scale
      configMenu();                         // Handle user input on CLI
      state = booting;                      // soft reboot
      break;

  }

}



/* ------------------------------------------------------------------------- *
 *       Measure leftToRight                                   leftToRight()
 * ------------------------------------------------------------------------- */
void leftToRight() {

debug("DetectedLeft, waitForRight - ");

  digitalWrite(leftDetection, HIGH);        // light up left detected
  leftMillis = millis();

  do {
  } while ( analogRead(rightSensor) > 200 ); // wait for right detection

  detectionTime = millis() - leftMillis;
  digitalWrite(rightDetection, HIGH);       // light up right detected
  showSpeed();

}



/* ------------------------------------------------------------------------- *
 *       Measure rightToLeft                                   RightToleft()
 * ------------------------------------------------------------------------- */
void rightToLeft() {

debug("DetectedRight, waitForLeft - ");

  digitalWrite(rightDetection, HIGH);       // light up right detected
  rightMillis = millis();

  do {
  } while ( analogRead(leftSensor) > 200 ); // wait for left detection

  detectionTime = millis() - rightMillis;
  digitalWrite(leftDetection, HIGH);        // light up left detected
  showSpeed();

}



/* ------------------------------------------------------------------------- *
 *       Calculate and show RealSpeed                            showSpeed()
 * ------------------------------------------------------------------------- */
void showSpeed() {

debug("Time: ");
debug(String(detectionTime));

  realSpeed = (realDistance * 3.6) / (detectionTime / 1000.0);

  LCD_display(display1, 0, 0, F("Speed:          "));
  LCD_display(display1, 0, 7, String(realSpeed) );
  LCD_display(display1, 0,13, F("Kmh"));

debug(" - Speed: ");
debugln(String(realSpeed));

  delay(5000);                              // show for 5 seconds

}


/* ------------------------------------------------------------------------- *
 *       Start command line interface                            configMenu()
 * ------------------------------------------------------------------------- */
void configMenu(){
  int save_int, stop, choice;
  float save_float, new_float;

  do {
    Serial.println();
    Serial.println("|****************************|");
    Serial.println("|**|  Configuation menu   |**|");
    Serial.println("|****************************|");
    Serial.println("");
    Serial.println("Select one of the following options:");
    Serial.println("1 Select scale");
    Serial.println("2 Set sensor distance");
    Serial.println("X Exit configuration menu");
    Serial.println();

    do {
      stop = 0;
      choice = Serial.read();
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

        case 'X':
          Serial.println("Leaving configuration menu");
          stop = 1;
          continue;

        default:
          break;
      }

    } while(stop==0);

  } while(choice != 'X');

 }


/* ------------------------------------------------------------------------- *
 *       Get new sensor distance in mm                   getSensorDistance()
 * ------------------------------------------------------------------------- */
float getSensorDistance() {
  float dist_float;
  bool valid = false;

  Serial.println();
  Serial.println("|****************************|");
  Serial.println("|**|  Set sensor distance |**|");
  Serial.println("|**|  in millimeters      |**|");
  Serial.println("|****************************|");
  Serial.println("");
  Serial.print("Current sensor distance = ");
  Serial.println(sensorDistance);
  Serial.println("");
  Serial.print("Specify desired sensor distance: ");

  do {
    if (Serial.available()) {
      dist_float = Serial.parseFloat();
      if (dist_float != 0) {
        valid = true;
        Serial.print("You entered: ");
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

  int choice = 'Z';

  do {
    Serial.println();
    Serial.println("|****************************|");
    Serial.println("|**|  Scale selection     |**|");
    Serial.println("|****************************|");
    Serial.println("");
    Serial.println("Select a scale by choosing the number:");
    Serial.println("");

    for (int i=0; i<=7; i++) {
      Serial.print(i);
      Serial.print(" - ");
      Serial.println( scaleName[i] );
    }
    Serial.println("X - Leave");

    for (;;) {
      choice = Serial.read();

      switch(choice) {

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          Serial.print("   Selected scale: ");
          Serial.print(choice-48);
          Serial.print(" - ");
          Serial.println( scaleName[choice-48] );

          scalePtr = choice-48;
          scaleFactor = scales[scalePtr];           // re-calculate realDistance
          realDistance = sensorDistance * scaleFactor / 1000.0;

          LCD_display(display1, 1, 0, F("Scale:          ") );
          LCD_display(display1, 1, 7, scaleName[scalePtr]);
          Serial.println( F("Leaving scale selection") );
          return;

        case 'X':
          Serial.println( F("Leaving scale selection") );
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

debugln(F("Store settings to EEPROM"));
                                            // Put settings in 
                                            //  mySettings structure
  mySettings.senseDistance = sensorDistance;
  mySettings.selectedScale = scalePtr;

                                            // Store mySettings structure 
                                            //  to EEPROM
  EEPROM.put(0, mySettings);

debugln();
debug("Stored: selected scale (");
debug(scaleName[scalePtr]);
debug(") - and sensor distance (");
debug(mySettings.senseDistance);
debugln(")");

}



/* ------------------------------------------------------------------------- *
 *       Retrieve settings from EEPROM                         getSettings()
 * ------------------------------------------------------------------------- */
void getSettings() {

  debugln(F("Retrieving settings from EEPROM"));
  
                                            // Get mySettings structure 
                                            //  from EEPROM
  EEPROM.get(0, mySettings);

                                            // Get settings from 
                                            //  mySettings structure
  sensorDistance = mySettings.senseDistance;
  scalePtr       = mySettings.selectedScale;

  debugln("Retrieved Sensor distance in mm:" + String(sensorDistance) );
  debugln("Retrieved Scale: " + String(scalePtr) + " being: " + scaleName[scalePtr]);
}


/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
  screen.setCursor(col, row);
  screen.print(text);
}
