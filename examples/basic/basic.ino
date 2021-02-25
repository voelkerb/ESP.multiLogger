/***************************************************
 Example file for using the multiLogger library.
 
 License: Creative Common V1. 

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems Engineer
 ****************************************************/
#include <multiLogger.h>

// Printed to console in front of log text to indicate logging
char * LOG_PREFIX_SERIAL = " - ";

// Create singleton here
MultiLogger& logger = MultiLogger::getInstance();
StreamLogger serialLog((Stream*)&Serial, &timeStr, &LOG_PREFIX_SERIAL[0], DEBUG);

void setup() {
  Serial.begin(9600);
  // Add seriallogger to multilogger
  logger.addLogger(&serialLog);
  // Init the logging modules
  logger.setTimeGetter(&timeStr);
  
  // Log an error message
  String test = "A string! - ugh";
  logger.log(ERROR, "Do we have an error here: %s", test.c_str());
  logger.log(INFO, "Setup done!");
}

void loop() {
  delay(5000);
  logger.log(INFO, "Ping!");
}

/****************************************************
 * Used for multilogger instance to produce time stamp
 ****************************************************/
#define MAX_TIME_LEN 15
char timeStr2[MAX_TIME_LEN] = {'\0'};
char * timeStr() {
  snprintf(&timeStr2[0], MAX_TIME_LEN, "%lus", long(millis()/1000));
  return &timeStr2[0];
}