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
 *
 *------------------------------------------------------------------------- */
#define progVersion "1.1"                   // Program version definition 
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
#define changeScale      4                  // Scale change

int state               = booting;          // Initial state

/* ------------------------------------------------------------------------- *
 *                                          Create object for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display1(0x27, 16, 2);    // Initialize display1 object


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
  debugstart(115000);   // make debug output fast

  Wire.begin();         // Start I2C

                        // Initialize display backlight on by default
  display1.init(); display1.backlight();

  pinMode(leftDetection, OUTPUT);
  pinMode(rightDetection, OUTPUT);

  pinMode(scaleSelection, INPUT_PULLUP);

debug(F("GAW_Speedometer v"));
debugln(progVersion);

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

      if (analogRead(leftSensor) < 200) {       // Left sensor detected
        state = detectedLeft;
        break;
      }

      if (analogRead(rightSensor) < 200) {      // Right sensor detected
        state = detectedRight;
        break;
      }

      if (!(digitalRead(scaleSelection)) ) {    // Select scale
        state = changeScale;
      }

      break;

    case detectedLeft:                      // Handle left to right traffic
      leftToRight();
      softBoot();
      state = waiting;
      break;

    case detectedRight:                     // Handle right to left traffic
      rightToLeft();
      softBoot();
      state = waiting;
      break;

    case changeScale:                       // choose scale
      chooseScale();
      state = waiting;
      break;

  }

}



/* ------------------------------------------------------------------------- *
 *       Measure leftToRight                                   leftToRight()
 * ------------------------------------------------------------------------- */
void leftToRight() {

debug("DetectedLeft, waitForRight - ");

  digitalWrite(leftDetection, HIGH);
  leftMillis = millis();

  do {
  } while ( analogRead(rightSensor) > 200 );

  detectionTime = millis() - leftMillis;
  digitalWrite(rightDetection, HIGH);
  showSpeed();

}



/* ------------------------------------------------------------------------- *
 *       Measure rightToLeft                                   RightToleft()
 * ------------------------------------------------------------------------- */
void rightToLeft() {

debug("DetectedRight, waitForLeft - ");

  digitalWrite(rightDetection, HIGH);
  rightMillis = millis();

  do {
  } while ( analogRead(leftSensor) > 200 );

  detectionTime = millis() - rightMillis;
  digitalWrite(leftDetection, HIGH);
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

  delay(5000);

}



/* ------------------------------------------------------------------------- *
 *       Choose and store scale                                chooseScale()
 * ------------------------------------------------------------------------- */
void chooseScale() {

  (scalePtr < 7)? scalePtr++ : scalePtr = 0;
  scaleFactor = scales[scalePtr];
                                            // re-calculate realDistance
  realDistance = sensorDistance * scaleFactor / 1000.0;

  LCD_display(display1, 1, 0, F("Scale:          ") );
  LCD_display(display1, 1, 7, scaleName[scalePtr]);

  delay(250);                               // debounce

}


/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
  screen.setCursor(col, row);
  screen.print(text);
}
