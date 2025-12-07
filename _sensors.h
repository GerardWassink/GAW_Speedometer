#ifndef _SENSORS
  #define _SENSORS


  /* ------------------------------------------------------------------------- *
   *                                                 Initialize detection pins
   * ------------------------------------------------------------------------- */
  void Sensors_init() {
    pinMode(leftDetection, OUTPUT);
    pinMode(rightDetection, OUTPUT);
  }



  /* ------------------------------------------------------------------------- *
   *                                                 Switch of detection LED's
   * ------------------------------------------------------------------------- */
  void Detection_init() {
    digitalWrite(leftDetection,  LOW);
    digitalWrite(rightDetection, LOW);

    scaleFactor = scales[scalePtr];
    realDistance = sensorDistance * scaleFactor / 1000.0;
  }


  /* ------------------------------------------------------------------------- *
   *       Measure leftToRight                                   leftToRight()
   * ------------------------------------------------------------------------- */
  void leftToRight() {

  debug(F("DetectedLeft, waitForRight - "));

    digitalWrite(leftDetection, HIGH);      // indicate left detected
    leftMillis = millis();

    do {
    } while ( WAITING_FOR_RIGHT );          // wait for right detection

    detectionTime = millis() - leftMillis;
    digitalWrite(rightDetection, HIGH);     // indicate right detected

  }



  /* ------------------------------------------------------------------------- *
   *       Measure rightToLeft                                   RightToleft()
   * ------------------------------------------------------------------------- */
  void rightToLeft() {

  debug(F("DetectedRight, waitForLeft - "));

    digitalWrite(rightDetection, HIGH);     // indicate right detected
    rightMillis = millis();

    do {
    } while ( WAITING_FOR_LEFT );           // wait for left detection

    detectionTime = millis() - rightMillis;
    digitalWrite(leftDetection, HIGH);      // indicate left detected

  }


#endif