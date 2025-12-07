#ifndef _I2C_display
  #define _I2C_display

  #include <Wire.h>                         // I2C comms library
  #include <LiquidCrystal_I2C.h>            // LCD library


  /* ------------------------------------------------------------------------- *
   *                                          Create object for the LCD screen
   * ------------------------------------------------------------------------- */
  LiquidCrystal_I2C display1(0x27, 16, 2);  // Initialize display1 object


  void Display_init() {
    Wire.begin();                           // Start I2C

                                            // Initialize display backlight 
    display1.init();                        //   on by default
    display1.backlight();

    Serial.println();
    Serial.print(F("---===### "));
    Serial.print(F("GAW_Speedometer v"));
    Serial.print(progVersion);
    Serial.println(F(" ###===---"));
    Serial.println();
  
    delay(1500);                            // show for 1,5 second

  }



  /* ------------------------------------------------------------------------- *
   *       Routine to display stuff on the display of choice     LCD_display()
   * ------------------------------------------------------------------------- */
  void LCD_display(LiquidCrystal_I2C screen, int row, int col, String text) {
    screen.setCursor(col, row);
    screen.print(text);
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

    delay(5000);                            // show for 5 seconds

  }



#endif
