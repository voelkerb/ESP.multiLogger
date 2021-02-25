# ESP.multiLogger
Ever wanted to log easily using one or multiple outputs (serial, spiffs, tcp, udp)?

Supports ESP32 and ESP8266. 
This library includes a Stream logger (Serial, TCP, UDP) and a SPIFFS logger. Log entries include the current time and and the log flag DEBUG, INFO, ERROR, WARNING. Logging can be done in a ```printf``` style.

```C++
#include <multiLogger.h>

// Printed to console in front of log text to indicate logging
char * LOG_PREFIX_SERIAL = " - ";
char * LOG_PREFIX_SPIFFS = "";
const char * LOG_FILE = "/log.txt";

#define SPIFFS_AUTO_FLUSH false

// Create singleton here
MultiLogger& logger = MultiLogger::getInstance();
StreamLogger serialLog((Stream*)&Serial, &timeStr, &LOG_PREFIX_SPIFFS[0], DEBUG);
// SPIFFS logger
SPIFFSLogger spiffsLog(SPIFFS_AUTO_FLUSH, &LOG_FILE[0], &timeStr, &LOG_PREFIX_SERIAL[0], WARNING);

char logMsg[_MAX_TEXT_LENGTH];

void setup() {
  Serial.begin(9600);
  // Add seriallogger to multilogger
  logger.addLogger(&serialLog);
  logger.addLogger(&spiffsLog);
  // Init the logging modules
  logger.setTimeGetter(&timeStr);
  
  String test = "A string! - ugh";
  logger.log(ERROR, "Do we have an error here: %s", test.c_str());

  // Dump the log file content to serial
  Serial.println("*** LOGFile *** ");
  bool hasRow = spiffsLog.nextRow(&logMsg[0]);
  while(hasRow) {
    Serial.println(&logMsg[0]);
    hasRow = spiffsLog.nextRow(&logMsg[0]);
  }
  Serial.println("____________");

  // Clear complete spiffs log
  spiffsLog.clear();
  
  logger.log(INFO, "Setup done!");
}

void loop() {
  // Serial output is flushed immiadiately, spiffs output only on explicit flush
  // if you require other flash processing, use SPIFFS_AUTO_FLUSH = False and flush 
  // whenever you are ready
  if (!SPIFFS_AUTO_FLUSH) spiffsLog.flush();
  ...
}

/****************************************************
 * Used for multilogger instance to produce time stamp
 ****************************************************/
#define MAX_TIME_LEN 15
char timeStr2[MAX_TIME_LEN] = {'\0'};
char * timeStr() {
  snprintf(&timeStr2[0], MAX_TIME_LEN, "%lu", long(millis()/1000));
  return &timeStr2[0];
}
```
