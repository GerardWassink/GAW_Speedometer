/* ------------------------------------------------------------------------- *
 * Name   : GAW-Speedometer
 * Author : Gerard Wassink
 * Date   : February 2025
 * Purpose: Measure model train speed depending on scale
 *            using Infrared detectors
 * Versions:
 *   0.1  : Initial code base
 *   0.2  : Working prototype
 *
 *------------------------------------------------------------------------- */
#define progVersion "0.2"              // Program version definition
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
#define initialize       0                  // Init system
#define detectedRight    1                  // Right sensor detected
#define detectedLeft     2                  // Left sensor detected
#define waitForRight     3                  // Wait for Right after Left
#define waitForLeft      4                  // Wait for Left after Right
#define setScale         5                  // Choose and set scale
#define reset            6                  // Reset button detected
#define NIL             99                  // Do nothing

int STATE;

/* ------------------------------------------------------------------------- *
 *                                          Create object for the LCD screen
 * ------------------------------------------------------------------------- */
LiquidCrystal_I2C display1(0x26, 20, 4);    // Initialize display1 object


/* ------------------------------------------------------------------------- *
 *                                  Constants and variables for measurements
 * ------------------------------------------------------------------------- */
float sensorDistance  = 200;                 // Distance in mm
float scaleFactor     = 0;                   // Scale Factor, default N-scale
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
int foundLeft  = 0;
int foundRight = 0;


/* ------------------------------------------------------------------------- *
 *                                                           Array of scales
 * ------------------------------------------------------------------------- */
float scales[8] = {45.2, 48, 64, 76, 87, 120, 160, 220};
int   scalePtr = 6;


/* ------------------------------------------------------------------------- *
 *                                       Buffers to build values for display
 * ------------------------------------------------------------------------- */
char strBuf1[20];                           // stringbuffer



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

}



/* ------------------------------------------------------------------------- *
 *       Initlialize system at reset                            initSystem()
 * ------------------------------------------------------------------------- */
void initSystem() {
                        // Initial text on display
  LCD_display(display1, 0, 0, F("GAW_Speedometer v   "));
  LCD_display(display1, 0,17, String(progVersion));
  LCD_display(display1, 3, 0, "Scale 1:");
  LCD_display(display1, 3, 8, String(scaleFactor,1));

  digitalWrite(leftDetection, LOW);
  digitalWrite(rightDetection, LOW);

  realDistance = sensorDistance * scaleFactor / 1000.0;

  foundLeft = foundRight = 0;
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
debug("Left - ");
      STATE = waitForRight;
      break;
      
    case detectedRight:
      rightMillis = millis();
      digitalWrite(rightDetection, HIGH);
debug("Right - ");
      STATE = waitForLeft;
      break;

    case waitForRight:
      if ( !digitalRead(rightSensor) ) {
        detectionTime = millis() - leftMillis;
        digitalWrite(rightDetection, HIGH);
        showSpeed();
debug("Right - ");
debug("Time: ");
debug(String(detectionTime));
        delay(1000);
        STATE = initialize;
      };
      break;
      
    case waitForLeft:
      if ( !digitalRead(leftSensor) ) {
        detectionTime = millis() - rightMillis;
        digitalWrite(leftDetection, HIGH);
        showSpeed();
debug("Left - ");
debug("Time: ");
debug(String(detectionTime));
        delay(1000);
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

  detect();

}



/* ------------------------------------------------------------------------- *
 *       Read inputs                                                detect()
 * ------------------------------------------------------------------------- */
void detect() {

  if (analogRead(leftSensor) <250) {
    if ( STATE != waitForLeft ) {
      STATE = detectedLeft;
    }
  }

  if (analogRead(rightSensor) <250) {
    if ( STATE != waitForRight ) {
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

  LCD_display(display1, 1,0, "                    ");
  LCD_display(display1, 2,0, "                    ");
  LCD_display(display1, 3,0, "                    ");

  do {
    scaleMillis = millis();

    if ( !digitalRead(scaleSelection) ) {
      (scalePtr < 7)? scalePtr++ : scalePtr = 0;
      scaleFactor = scales[scalePtr];
    }
    LCD_display(display1, 3, 0, "Scale 1:");
    LCD_display(display1, 3, 8, String(scaleFactor,1));

  } while (scaleMillis < 1000);
}


/* ------------------------------------------------------------------------- *
 *       Calculate and show RealSpeed                       calculateSpeed()
 * ------------------------------------------------------------------------- */
void showSpeed() {
  realSpeed = (realDistance * 3.6) / (detectionTime / 1000.0);

  LCD_display(display1, 2, 0, F("Speed:              "));
  LCD_display(display1, 2, 7, String(realSpeed) );

debug(" - Speed: ");
debug(String(realSpeed));
debugln();

}



/* ------------------------------------------------------------------------- *
 *       Routine to display stuff on the display of choice     LCD_display()
 * ------------------------------------------------------------------------- */
void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
  screen.setCursor(col, row);
  screen.print(text);
}
