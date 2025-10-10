/* ------------------------------------------------------------------------- *
 * Name   : GAW-Speedometer
 * Author : Gerard Wassink
 * Date   : February 2025
 * Purpose: Measure model train speed depending on scale
 *            using Infrared detectors
 * Versions:
 *   0.1  : Initial code base
 *   0.2  : Working prototype
 *   0.3  : Corrected little errors
 *   0.4  : Switched back to 16x04 LCD
 *   0.5  : Code- and timing improvements
 *   0.6  : Built in Measurement button & LED
 *          Restructured code, removed STATE machine
 *            resulting in more reliable measurements
 *   0.7  : Replaced soft reset with hard reset
 *   0.8  : Improved README, schematic added
 *   0.9  : Built in pause after showing speed
 *   1.0  : First Release
 *
 *------------------------------------------------------------------------- */
#define progVersion "1.0"                   // Program version definition 
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


/* ------------------------------------------------------------------------- *
 *                                                           Pin definitions
 * ------------------------------------------------------------------------- */
#define leftSensor       A2                 // Input pins
#define rightSensor      A3                 //   from IR detectors

#define scaleSelection   PD2                // Button to browse/seletc scale

#define leftDetection    PD3                // LED left detection
#define rightDetection   PD4                // LED right detection
#define measurement      PD5                // LED measurement active

#define doMeasurement    PD6                // Button to start measurement

/* ------------------------------------------------------------------------- *
 *                                                        Defines for states
 * ------------------------------------------------------------------------- */
#define debounceWait     500                // time to wait after detection

/* ------------------------------------------------------------------------- *
 *                                          Create object for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display1(0x27, 16, 4);    // Initialize display1 object


/* ------------------------------------------------------------------------- *
 *                                  Constants and variables for measurements
 * ------------------------------------------------------------------------- */
float sensorDistance  = 200.0;               // Distance in mm
float scaleFactor     = 0.0;                 // Scale Factor, default N-scale
float realDistance    = sensorDistance       // Real Distance in meters
                        * scaleFactor  
                        / 1000;
long  detectionTime   = 0;                   // Time between detections
float realSpeed       = 0.0;


/* ------------------------------------------------------------------------- *
 *                                                Variables timing detection
 * ------------------------------------------------------------------------- */
unsigned long leftMillis  = 0;
unsigned long rightMillis = 0;


/* ------------------------------------------------------------------------- *
 *                                            Array of NMRA scales and names
 * ------------------------------------------------------------------------- */
float scales[8]        = {45.2, 
                          48.0, 
                          64.0, 
                          76.0, 
                          87.0, 
                          120.0, 
                          160.0, 
                          220.0
                         };
char  scaleName[8][12] = {"O(17)",
                          "O, On3, On2", 
                          "Sn3, S", 
                          "OO", 
                          "HO", 
                          "TT", 
                          "N, Nn3", 
                          "Z"
                         };
                         
int   scalePtr = 6;                         // set default here



/* ------------------------------------------------------------------------- *
 *       Initialization routine                                      setup()
 * ------------------------------------------------------------------------- */
void setup() {
  debugstart(115000);   // make debug output fast

  debug("GAW_Speedometer v");
  debugln(progVersion);

  Wire.begin();         // Start I2C

                        // Initialize display backlight on by default
  display1.init(); display1.backlight();

  pinMode(leftDetection, OUTPUT);
  pinMode(rightDetection, OUTPUT);
  pinMode(measurement, OUTPUT);

  pinMode(scaleSelection, INPUT_PULLUP);
  pinMode(doMeasurement, INPUT_PULLUP);

  scaleFactor = scales[scalePtr];

                        // Initial text on display
  LCD_display(display1, 0, 0, F("GAW_Speedometer "));
  LCD_display(display1, 1, 0, F("  Version       "));
  LCD_display(display1, 1,10, String(progVersion));

  delayFor(3000);

  LCD_display(display1, 0, 0, F("Speed:          "));
  LCD_display(display1, 1, 0, F("Scale:           ") );
  LCD_display(display1, 1, 7, scaleName[scalePtr]);

  realDistance = sensorDistance * scaleFactor / 1000.0;

}



/* ------------------------------------------------------------------------- *
 *       Initlialize system at reset                            initSystem()
 * ------------------------------------------------------------------------- */
void initSystem() {

  digitalWrite(leftDetection,  LOW);
  digitalWrite(rightDetection, LOW);
  digitalWrite(measurement,    LOW);

}



/* ------------------------------------------------------------------------- *
 *       Repeating loop                                               loop()
 * ------------------------------------------------------------------------- */
void loop() {

  if ( !digitalRead(doMeasurement) ) {
debug("Measurement starts - ");
    digitalWrite( measurement, HIGH );
    measure();
  }

  if ( !digitalRead(scaleSelection) ) {
debugln("Select Scale");
    chooseScale();
  }

}



/* ------------------------------------------------------------------------- *
 *       Do measurment                                         measurement()
 * ------------------------------------------------------------------------- */
void measure() {

  do {
  } while ( (analogRead(leftSensor)  > 200) && 
            (analogRead(rightSensor) > 200)
          );

  if (analogRead(leftSensor) < 200) {
    leftToRight();
  } else 
  if (analogRead(rightSensor) < 200) {
    rightToLeft();
  }

  initSystem();

}



/* ------------------------------------------------------------------------- *
 *       Measure leftToRight                                   leftToRight()
 * ------------------------------------------------------------------------- */
void leftToRight() {

  int rValue = 999;

  leftMillis = millis();
  debug("detectedLeft, waitForRight - ");
  digitalWrite(leftDetection, HIGH);
//  delayFor(debounceWait);

  do {
    rValue = analogRead(rightSensor);
  } while ( rValue > 200 );

  detectionTime = millis() - leftMillis;
  digitalWrite(rightDetection, HIGH);

  showSpeed();

}



/* ------------------------------------------------------------------------- *
 *       Measure rightToLeft                                   RightToleft()
 * ------------------------------------------------------------------------- */
void rightToLeft() {

  int lValue = 999;

  rightMillis = millis();
  debug("detectedRight, waitForLeft - ");
  digitalWrite(rightDetection, HIGH);
//  delayFor(debounceWait);

  do {
    lValue = analogRead(leftSensor);
  } while ( lValue > 200 );

  detectionTime = millis() - rightMillis;
  digitalWrite(leftDetection, HIGH);

  showSpeed();

}



/* ------------------------------------------------------------------------- *
 *       Calculate and show RealSpeed                       calculateSpeed()
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

  delayFor(500);

}



/* ------------------------------------------------------------------------- *
 *       Delay operations for ms milliseconds                     delayFor()
 * ------------------------------------------------------------------------- */
void delayFor(unsigned long ms) {
  unsigned long start = millis();
  do {
    // Nothing
  } while ( millis() < start + ms );
}



/* ------------------------------------------------------------------------- *
 *       Choose and store scale                                chooseScale()
 * ------------------------------------------------------------------------- */
void chooseScale() {
  unsigned long scaleMillis;

  scaleMillis = millis();

  do {

    if ( !digitalRead(scaleSelection) ) {
      (scalePtr < 7)? scalePtr++ : scalePtr = 0;
      scaleFactor = scales[scalePtr];
    }

    LCD_display(display1, 1, 0, F("Scale:          ") );
    LCD_display(display1, 1, 7, scaleName[scalePtr]);

    delayFor(250);

  } while ( (millis() - scaleMillis) < 250);             // give max one second to change

  realDistance = sensorDistance * scaleFactor / 1000.0;

}


/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
  screen.setCursor(col, row);
  screen.print(text);
}
