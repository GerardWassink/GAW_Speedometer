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
 *   2.2  : Built in F() function for literals, minor improvements
 *   2.3  : Added GNU Public License V2 text and License file GPLv2.md
 *   2.4  : Reorganized code into functional .h files
 *
 *------------------------------------------------------------------------- */
#define progVersion "2.4"                   // Program version definition 
/* ------------------------------------------------------------------------- *
 *             GNU LICENSE CONDITIONS
 * ------------------------------------------------------------------------- *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation version 2
 * of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <https://www.gnu.org/licenses/>.
 * 
 * ------------------------------------------------------------------------- *
 *       Copyright (C) December 2025 Gerard Wassink
 * ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- *
 *                                                  Include functional files
 * ------------------------------------------------------------------------- */
#include "globals.h"                        // global definitions
#include "_EEPROM.h"                        // code for handling EEPROM
#include "_I2C_display.h"                   // code for I2C display
#include "_Settings.h"                      // code for configurations
#include "_sensors.h"                       // code for sensors

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
 *       Initialization routine                                      setup()
 * ------------------------------------------------------------------------- */
void setup() {
  Serial.begin(115000);

  Display_init();                           // Initialize I2C_display
  Settings_init();                          // Initialize Settings (config menus)
  Detection_init();                         // Initialize Detection
  Sensors_init();                           // Initialize Sensors

//  store_EEPROM_Settings();                // uncomment to store initial settings
  get_EEPROM_Settings();                    // Get settings from EEPROM

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

  Detection_init();                         // (Re)initialize detection

  Serial.println();
  Serial.println(F("---===### Ready for operation  ###===---"));
  Serial.println();

  state = waiting;

}



/* ------------------------------------------------------------------------- *
 *       Repeating loop                                               loop()
 * ------------------------------------------------------------------------- */
void loop() {

  switch (state) {                          // What to do?

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
      showSpeed();                              // show result
      state = booting;
      break;

    case detectedRight:
      rightToLeft();                        // Handle right to left traffic
      showSpeed();                              // show result
      state = booting;
      break;

    case buttonPressed:
      configMenu();                         // Go config on CLI
      state = booting;                      // soft reboot
      break;

  }

}



