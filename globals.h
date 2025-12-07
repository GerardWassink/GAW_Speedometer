/* ------------------------------------------------------------------------- *
 *                                                        DEBUGGING ON / OFF
 * Compiler directives to switch debugging on / off
 * Do not enable DEBUG when not needed, Serial coms takes space and time!
 * ------------------------------------------------------------------------- */
#ifndef GLOBALS
  #define GLOBALS

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
   *                            Defines to make measurement code more readable
   * ------------------------------------------------------------------------- */
  #define WAITING_FOR_RIGHT ( analogRead(rightSensor) > tresholdRight )
  #define WAITING_FOR_LEFT  ( analogRead(leftSensor) > tresholdLeft )


  /* ------------------------------------------------------------------------- *
   *                                            Array of NMRA scales and names
   * ------------------------------------------------------------------------- */
  float scales[8]        = {45.2,           // for O(17) scale
                            48.0,           // for O, On3, On2 scale
                            64.0,           // for Sn3, S scale
                            76.0,           // for OO scale
                            87.0,           // for HO scale
                            120.0,          // for TT scale
                            160.0,          // for N, Nn3 scale
                            220.0           // for Z scale
                          };

  char  scaleName[8][12] = {"O(17)",        // NMRA descriptions
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
   *                                        Initial constants for measurements
   * ------------------------------------------------------------------------- */
  float         sensorDistance  = 200;      // Distance in mm
  float         scaleFactor     = 160.0;    // Scale Factor, default N-scale
  int           tresholdLeft    = 200;      // Working value for left sensor
  int           tresholdRight   = 200;      // Working value for right sensor


  /* ------------------------------------------------------------------------- *
   *                                                Variables for measurements
   * ------------------------------------------------------------------------- */
  float         realDistance    = 0.0;      // Holds calculated distance
  float         realSpeed       = 0.0;      // Holds calculated speed
  unsigned long detectionTime   = 0;        // Time between detections



  /* ------------------------------------------------------------------------- *
   *          Create structure and object for settings to store them to EEPROM
   * ------------------------------------------------------------------------- */
  struct Settings {
    int   senseDistance;                    // Distance between sensors
    int   selectedScale;                    // The scale the user selected
    int   leftTreshold;                     // treshold value for left sensor
    int   rightTreshold;                    // treshold value for right sensor
  };
  Settings mySettings;                      // Create the object


  /* ------------------------------------------------------------------------- *
   *                                                           Pin definitions
   * ------------------------------------------------------------------------- */
  #define leftSensor       A2               // Input pins
  #define rightSensor      A3               //   from IR detectors

  #define leftDetection    PD3              // LED left detected
  #define rightDetection   PD4              // LED right detected


  /* ------------------------------------------------------------------------- *
   *                                            Variables for timing detection
   * ------------------------------------------------------------------------- */
  unsigned long leftMillis  = 0;
  unsigned long rightMillis = 0;


#endif
