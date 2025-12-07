#ifndef _Settings
  #define _Settings


  #define startConfig      PD2              // Button to start configuration


  /* ------------------------------------------------------------------------- *
   *                                              Initialize configuration_pin
   * ------------------------------------------------------------------------- */
  void Settings_init() {
    pinMode(startConfig, INPUT_PULLUP);
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
            store_EEPROM_Settings();
          }
          stop = 1;
          continue;

        case '2':
          save_float = sensorDistance;
          new_float = getSensorDistance();  // Ask for new sensor distance
          if (new_float != save_float) {    // only store when changed
            sensorDistance = new_float;
            store_EEPROM_Settings();
          }
          stop = 1;
          break;

        case '3':
          save_left = tresholdLeft;
          new_left = getLeftTreshold();     // Ask for new value for left sensor
          if (new_left != save_left) {      // only store when changed
            tresholdLeft = new_left;
            store_EEPROM_Settings();
          }
          stop = 1;
          break;

        case '4':
          save_right = tresholdRight;
          new_right = getRightTreshold();   // Ask for new value for left sensor
          if (new_right != save_right) {    // only store when changed
            tresholdRight = new_right;
            store_EEPROM_Settings();
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



#endif