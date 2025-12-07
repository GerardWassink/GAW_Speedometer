#ifndef _EEPROM
  #define _EEPROM


  #include <EEPROM.h>                       // EEPROM library

  /* ------------------------------------------------------------------------- *
  *       Store settings to EEPROM                            storeSettings()
  * ------------------------------------------------------------------------- */
  void store_EEPROM_Settings() {

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
  void get_EEPROM_Settings() {

    Serial.println(F("Retrieving settings from EEPROM"));
    
                                            // Get mySettings structure 
    EEPROM.get(0, mySettings);              //  from EEPROM

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


#endif