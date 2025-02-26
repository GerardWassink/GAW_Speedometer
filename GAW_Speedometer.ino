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
 *
 *------------------------------------------------------------------------- */
#define progVersion "0.4"              // Program version definition 
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

#define resetButton      PD2                // Reset the detection system
#define scaleSelection   PD3                // Button to browse/seletc scale

#define leftDetection    PD4                // LED indicating left detection
#define rightDetection   PD5                // LED indicating rightt detection


/* ------------------------------------------------------------------------- *
 *                                                        Defines for states
 * ------------------------------------------------------------------------- */
#define initialize        0                 // Init system
#define detectedRight     1                 // Right sensor detected
#define detectedLeft      2                 // Left sensor detected
#define waitForRight      3                 // Wait for Right after Left
#define waitForLeft       4                 // Wait for Left after Right
#define setScale          5                 // Choose and set scale
#define reset             6                 // Reset button detected
#define NIL              99                 // Do nothing

int STATE;

/* ------------------------------------------------------------------------- *
 *                                          Create object for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display1(0x27, 16, 4);    // Initialize display1 object


/* ------------------------------------------------------------------------- *
 *                                  Constants and variables for measurements
 * ------------------------------------------------------------------------- */
float sensorDistance  = 200;                 // Distance in mm
float scaleFactor     = 160;                 // Scale Factor, default N-scale
float realDistance    = sensorDistance       // Real Distance in meters
                        * scaleFactor  
                        / 1000;
long  detectionTime   = 0;
float realSpeed       = 0;


/* ------------------------------------------------------------------------- *
 *                                           Variables controlling detection
 * ------------------------------------------------------------------------- */
unsigned long leftMillis  = 0;
unsigned long rightMillis = 0;


/* ------------------------------------------------------------------------- *
 *                                            Array of NMRA scales and names
 * ------------------------------------------------------------------------- */
float scales[8] = {45.2, 48, 64, 76, 87, 120, 160, 220};
char  scaleName[8][12] = {"O(17)",
                          "O, On3, On2", 
                          "Sn3, S", 
                          "OO", 
                          "HO", 
                          "TT", 
                          "N, Nn3", 
                          "Z"
                         };
int   scalePtr = 6;



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

  pinMode(resetButton, INPUT_PULLUP);
  pinMode(scaleSelection, INPUT_PULLUP);

  scaleFactor = scales[scalePtr];

  STATE = initialize;

                        // Initial text on display
  LCD_display(display1, 0, 0, F("GAW_Speedometer "));
  LCD_display(display1, 1, 0, F("  Version       "));
  LCD_display(display1, 1,10, String(progVersion));

  delayFor(3000);

  LCD_display(display1, 0, 0, F("Speed:          "));
  LCD_display(display1, 1, 0, F("Scale:           ") );
  LCD_display(display1, 1, 7, scaleName[scalePtr]);

}



/* ------------------------------------------------------------------------- *
 *       Initlialize system at reset                            initSystem()
 * ------------------------------------------------------------------------- */
void initSystem() {

debugln("Clearing Detection");
  digitalWrite(leftDetection, LOW);
  digitalWrite(rightDetection, LOW);

  realDistance = sensorDistance * scaleFactor / 1000.0;

  STATE = NIL;

}



/* ------------------------------------------------------------------------- *
 *       Repeating loop, state machine                                loop()
 * ------------------------------------------------------------------------- */
void loop() {

  switch (STATE) {

    case initialize:
      initSystem();
      break;

    case detectedLeft:
      leftMillis = millis();
      digitalWrite(leftDetection, HIGH);
debug("detectedLeft - ");
      delayFor(500);
      STATE = waitForRight;
      break;
      
    case detectedRight:
      rightMillis = millis();
      digitalWrite(rightDetection, HIGH);
debug("detectedRight - ");
      delayFor(500);
      STATE = waitForLeft;
      break;

    case waitForRight:
      if ( !digitalRead(rightSensor) ) {
        detectionTime = millis() - leftMillis;
        digitalWrite(rightDetection, HIGH);
debug("waitForRight - ");
debug("Time: ");
debug(String(detectionTime));
        showSpeed();
        delayFor(500);
        STATE = initialize;
      };
      break;
      
    case waitForLeft:
      if ( !digitalRead(leftSensor) ) {
        detectionTime = millis() - rightMillis;
        digitalWrite(leftDetection, HIGH);
debug("waitForLeft - ");
debug("Time: ");
debug(String(detectionTime));
        showSpeed();
        delayFor(500);
        STATE = initialize;
      };
      break;
      
    case setScale:
      chooseScale();
      STATE = initialize;
      break;
      
    case reset:
      STATE = initialize;
      break;

    default:
      break;
      
  }

  detect();                                 // Read sensors / pins

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
 *       Read inputs and set next state accordingly                 detect()
 * ------------------------------------------------------------------------- */
void detect() {

  if (analogRead(leftSensor) <250) {
    if ( STATE != waitForRight ) {
      STATE = detectedLeft;
    }
  }

  if (analogRead(rightSensor) <250) {
    if ( STATE != waitForLeft ) {
      STATE = detectedRight;
    }
  }

  if ( !digitalRead(resetButton) ) {
    STATE = reset;
  }

  if ( !digitalRead(scaleSelection) ) {
    STATE = setScale;
  }

}



/* ------------------------------------------------------------------------- *
 *       Choose and store scale                                chooseScale()
 * ------------------------------------------------------------------------- */
void chooseScale() {
  unsigned long scaleMillis;

  do {
    scaleMillis = millis();

    if ( !digitalRead(scaleSelection) ) {
      (scalePtr < 7)? scalePtr++ : scalePtr = 0;
      scaleFactor = scales[scalePtr];
    }
    LCD_display(display1, 1, 0, F("Scale:          ") );
    LCD_display(display1, 1, 7, scaleName[scalePtr]);

    delayFor(250);

  } while (scaleMillis < 1000);             // give max one second to change
}


/* ------------------------------------------------------------------------- *
 *       Calculate and show RealSpeed                       calculateSpeed()
 * ------------------------------------------------------------------------- */
void showSpeed() {
  realSpeed = (realDistance * 3.6) / (detectionTime / 1000.0);

  LCD_display(display1, 0, 0, F("Speed:          "));
  LCD_display(display1, 0, 7, String(realSpeed) );

debug(" - Speed: ");
debugln(String(realSpeed));

}



/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
  screen.setCursor(col, row);
  screen.print(text);
}
