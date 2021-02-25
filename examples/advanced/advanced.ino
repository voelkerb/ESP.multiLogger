/***************************************************
 Example file for using the multiLogger library.
 
 License: Creative Common V1. 

 Benjamin Voelker, voelkerb@me.com
 Embedded Systems Engineer
 ****************************************************/
#include <multiLogger.h>
#if defined(ESP32)
#include "WiFi.h"
#else
#include "ESP8266WiFi.h"
#endif

const char* SSID = "YourNetworkName";
const char* PWD =  "YourNetworkPassword";

// Printed to logger in front of log text to indicate logging
char * LOG_PREFIX_SERIAL = " - ";
char * LOG_PREFIX_STREAM = "";

// Create singleton here
MultiLogger& logger = MultiLogger::getInstance();
StreamLogger serialLog((Stream*)&Serial, &timeStr, &LOG_PREFIX_SERIAL[0], DEBUG);

// Currently only ESP32 supports spiff logging
#if defined(ESP32)
char * LOG_PREFIX_SPIFFS = "";
const char * LOG_FILE = "/log.txt";
#define SPIFFS_AUTO_FLUSH false
// SPIFFS logger
SPIFFSLogger spiffsLog(SPIFFS_AUTO_FLUSH, &LOG_FILE[0], &timeStr, &LOG_PREFIX_SERIAL[0], WARNING);
#endif

// TCP Logging on port 54321
#define MAX_CLIENTS 3
WiFiServer server(54321);
// TCP clients and current connected state, each client is assigned a logger
WiFiClient client[MAX_CLIENTS];
bool clientConnected[MAX_CLIENTS] = {false};
StreamLogger * streamLog[MAX_CLIENTS];

// Timer to send ping log message
unsigned long pingTimer = 0;
#define PING_TIME 2000

// Timer to check every tcp connection
unsigned long tcpUpdateTime = 0;
#define TCP_UPDATE_TIME 1000

void setup() {
  Serial.begin(9600);
  // Add loggers to multilogger
  logger.addLogger(&serialLog);
#if defined(ESP32)
  logger.addLogger(&spiffsLog);
#endif
  // init the stream logger array
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    StreamLogger * theStreamLog = new StreamLogger(NULL, &timeStr, &LOG_PREFIX_STREAM[0], INFO);
    streamLog[i] = theStreamLog;
  }

  // Set the time getter strings for multilogger
  logger.setTimeGetter(&timeStr);
  
  // Connect to WiFi
  WiFi.begin(SSID, PWD);
  logger.append("Connecting to WiFi.");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    logger.append(".");
  }
  logger.flushAppended(INFO);
  logger.log(INFO, "Connected to the WiFi network: %s with IP: %s", SSID, WiFi.localIP().toString().c_str());

  // Start the TCP server
  server.begin();

  // Log an error message
  String test = "A string! - ugh";
  logger.log(ERROR, "Do we have an error here: %s", test.c_str());


#if defined(ESP32)
  // Dump the log file content to serial
  char logMsg[_MAX_TEXT_LENGTH];
  Serial.println("*** LOGFile *** ");
  bool hasRow = spiffsLog.nextRow(&logMsg[0]);
  while(hasRow) {
    Serial.println(&logMsg[0]);
    hasRow = spiffsLog.nextRow(&logMsg[0]);
  }
  Serial.println("____________");

  // Clear complete spiffs log
  spiffsLog.clear();
#endif
  
  pingTimer = millis();
  tcpUpdateTime = millis();
  logger.log(INFO, "Setup done!");
}

void loop() {
#if defined(ESP32)
  // Serial output is flushed immiadiately, spiffs output only on explicit flush
  // if you require other flash processing, use SPIFFS_AUTO_FLUSH = False and flush 
  // whenever you are ready
  if (!SPIFFS_AUTO_FLUSH) spiffsLog.flush();
#endif
  
  // Send ping to each logger
  if ((long)(millis() - pingTimer) >= 0) {
    pingTimer += PING_TIME;
    if ((long)(millis() - pingTimer) >= 0) pingTimer = millis() + PING_TIME; 
    logger.log("ping!");
  }

  // Handle Serial requests
  if (Serial.available()) {
    handleEvent(&Serial);
  }

  // Handle tcp requests
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i] and client[i].available() > 0) {
      handleEvent(&client[i]);
    }
  }

  // TCP Updates once in a while
  if ((long)(millis() - tcpUpdateTime) >= 0) {
    tcpUpdateTime += TCP_UPDATE_TIME;
    if ((long)(millis() - tcpUpdateTime) >= 0) tcpUpdateTime = millis() + TCP_UPDATE_TIME; 
    // Handle connects
    WiFiClient newClient = server.available();
    if (newClient) {
      onClientConnect(newClient);
    }

    // Handle disconnect
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        if (!client[i].connected()) {
          onClientDisconnect(client[i], i);
          clientConnected[i] = false;
        }
      }
    }
  }
}

/****************************************************
 * You can handle commands here
 ****************************************************/
void handleEvent(Stream * getter) {
  String cmd = getter->readStringUntil('\n');
  getter->println("Understood");
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

/****************************************************
 * If a tcp client connects.
 * We store them in list and add logger
 ****************************************************/
void onClientConnect(WiFiClient &newClient) {
  logger.log("Client with IP %s connected on port %u", newClient.remoteIP().toString().c_str(), newClient.remotePort());
  // Loop over all clients and look where we can store the pointer... 
  for (size_t i = 0; i < MAX_CLIENTS; i++) {
    if (!clientConnected[i]) {
      client[i] = newClient;
      client[i].setNoDelay(true);
      client[i].setTimeout(10);
      // Set connected flag
      clientConnected[i] = true;
      streamLog[i]->_type = INFO;
      streamLog[i]->_stream = (Stream*)&client[i];
      logger.addLogger(streamLog[i]);
      return;
    }
  }
  logger.log("To much clients, could not add client");
  newClient.stop();
}

/****************************************************
 * If a tcp client disconnects.
 * We must remove the logger
 ****************************************************/
void onClientDisconnect(WiFiClient &oldClient, size_t i) {
  logger.log("Client disconnected %s port %u", oldClient.remoteIP().toString().c_str(), oldClient.remotePort());
  logger.removeLogger(streamLog[i]);
  streamLog[i]->_stream = NULL;
}
