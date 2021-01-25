/*
   ___  _             ____         __ _   
  / _ \| | ___  _ __ / ___|  ___  / _| |_ 
 | | | | |/ _ \| '_ \\___ \ / _ \| |_| __|
 | |_| | | (_) | | | |___) | (_) |  _| |_ 
  \___/|_|\___/|_| |_|____/ \___/|_|  \__|

*/         

#include <Arduino.h>
#include <ESPcrashSave.h>
#include <Gill.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <WindFunc.h>
#include <common.h>
#include <helper.h>


#define SWSERIAL_BAUD_RATE 9600
#define SERIAL_BUFFER_SIZE 256

ESPCrashSave crashSave;
String crash_post_url =
    "http://studio19.gr/olonsoft/firmware/anemometer/debug.php";  // Location
                                                                  // where
                                                                  // images are
                                                                  // POSTED

// ======== pin definitions ========
// LED_BUILTIN          D4    // gpio2 already defined in pins_arduino.h
#define LED_BUILTIN_ON  LOW
#define LED_BUILTIN_OFF HIGH

#define LED_ONBOARD     D0  // gpio16 pcb led
#define LED_ONBOARD_ON  LOW
#define LED_ONBOARD_OFF HIGH

#define MOSFET_PIN      D7  // gpio13
#define MOSFET_ON       HIGH
#define MOSFET_OFF      LOW

#define PIN_SERIAL_RX   D5  // gpio14
#define PIN_SERIAL_TX   D6  // gpio12

// oled i2c SCL pin     D1    // gpio5
// oled i2c SDA pin     D2    // gpio4 
// #define BUTTON1      D3    // gpio0

// ===================== Serial ======================
SoftwareSerial swSer;

char receivedChars[SERIAL_BUFFER_SIZE];  // an array to store the received data
bool serialNewData = false;
bool serialEnabled = false;

// ===================== wind data ==================

Gill gill;
WindFunc windF;

uint16_t windDirectionCurrent;
uint16_t windDirectionAverage;
float windSpeedCurrent;
float windSpeedAverage;
float windSpeedMax;
float windSpeedMin;

const char dataPayload[] PROGMEM =
    "{\"time\":\"%s\","
    "\"speed\":\"%.2f\","
    "\"gust\":\"%.2f\","
    "\"lull\":\"%.2f\","
    "\"dir\":\"%d\""
    "}";

bool onBoardLedOn = false;
uint32_t onBoardLedTime = 0;

bool chipLedOn = false;
uint32_t chipLedTime = 0;

bool mosfetOn =
    true;  // set it true because first call on setup will set it off

bool firstWiFiConnection = true;

uint32_t lastHeartBeat = 0;
Ticker tickerWatchDog;
void isAlive();

void drawCompass() {

  display.setColor(BLACK);
  display.fillRect(0, 16, 127, 47);
  display.setColor(WHITE);

  int clockCenterX = screenW / 2 - 40;  // -40 place on the left
  // top yellow part is 16 px height
  int clockCenterY = ((screenH - 16) / 2) + 16;
  int clockRadius = 20;           // 23;

  display.drawCircle(clockCenterX, clockCenterY, clockRadius);

  float angle = (float)windDirectionCurrent;
  angle = (angle / 57.29577951);  // Convert degrees to radians
  int x1 = (clockCenterX + (sin(angle) * clockRadius));
  int y1 = (clockCenterY - (cos(angle) * clockRadius));

  int x2 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 2))));
  int y2 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 2))));
  display.drawLine(x1, y1, x2, y2);

  angle = (float)windDirectionCurrent - 15;
  if (angle < 0) angle = angle + 358;
  angle = (angle / 57.29577951);  // Convert degrees to radians
  x2 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 3))));
  y2 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 3))));
  display.drawLine(x1, y1, x2, y2);

  angle = (float)windDirectionCurrent + 15;
  if (angle > 359) angle = angle - 360;
  angle = (angle / 57.29577951);  // Convert degrees to radians
  x2 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 3))));
  y2 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 3))));
  display.drawLine(x1, y1, x2, y2);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(clockCenterX, clockCenterY - 6,
                     String(windDirectionCurrent));

  // display tmpWindSpeed, windSpeedMin, windSpeedMax
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(50, 16, "Spd: " + String(windSpeedCurrent));
  display.drawString(50, 32, "Min: " + String(windSpeedMin));
  display.drawString(50, 48, "Max: " + String(windSpeedMax));

  display.display();
}

// oled screen screen saver and draw compass if on.
void oledLoop() {
  static unsigned long last_data_sent = 0;
  // need to proceed not less than 1000ms
  if ((last_data_sent > 0) && ((millis() - last_data_sent) < 1000)) return;
  last_data_sent = millis();
  if (screenOn && (last_data_sent - lastTimeScreenOn > screenSaverTime * 1000)) {
    screenOn = false;
    display.displayOff();
    statusString = "";
  }
  if (screenOn /* && windSpeedSamples > 0 */) {
    drawCompass();
  }
}

void flashOnBoardLed() {
  digitalWrite(LED_ONBOARD, LED_ONBOARD_ON);
  onBoardLedTime = millis();
  onBoardLedOn = true;
}

void flashChipLed() {
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  chipLedTime = millis();
  chipLedOn = true;
}

void ledsLoop() {
  uint32_t t = millis();

  if (onBoardLedOn && t - onBoardLedTime > 100) {
    digitalWrite(LED_ONBOARD, LED_ONBOARD_OFF);
    onBoardLedOn = false;
  }

  if (chipLedOn && t - chipLedTime > 100) {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
    chipLedOn = false;
  }
}

bool serialDataReceived() {
  static uint16_t ndx = 0;
  char _LF = '\n';  // <LF> (should I check <CR>\r instead ?)
  char _CR = '\r';
  char rc;

  while (swSer.available() > 0 && serialNewData == false) {
    rc = swSer.read();
    if (rc != _LF) {
      if (rc != _CR) {
        receivedChars[ndx++] = rc;
        if (ndx >= SERIAL_BUFFER_SIZE) ndx--;
      }
    } else {
      receivedChars[ndx] = '\0';  // terminate the string
      ndx = 0;
      serialNewData = true;
    }
  }
  return (serialNewData == true);
}

bool mqttSendDeviceData(char *str) {
  if (mqttConnect()) {
    LOGDEBUG(str);

    String topic;
    topic = String(appSettings.mqttTopic) + FPSTR(_topicStatus);

    if (mqttClient.publish(topic.c_str(), str) == true) {
      mqttClient.loop();
      flashChipLed();      
      TLOGDEBUGF_P(PSTR("[MQTT] Success sending message.\n"));
      return true;
    } else {
      TLOGDEBUGF_P(PSTR("[MQTT] Error sending message.\n"));
    }
  }
  return false;
}

void parseDeviceData(char *str) { mqttSendDeviceData(str); }

void addDeviceValues(float wSpeed, uint16_t wDir) {
  windF.addWindSpeed(wSpeed);
  windF.addWindDirection(wDir);
  windF.getWindSpeed(windSpeedCurrent, windSpeedAverage, windSpeedMin,
                     windSpeedMax);
  windDirectionCurrent = wDir;
}

bool getAndResetWindValues() {
  bool result = false;
  float tmpWindSpeedCurrent;
  if (windF.getWindSpeed(tmpWindSpeedCurrent, windSpeedAverage, windSpeedMin,
                         windSpeedMax))
    result = true;
  if (result) {
    uint16_t tmpWindDirectionCurrent;
    result =
        windF.getWindDirection(tmpWindDirectionCurrent, windDirectionAverage);
  }
  windF.resetWindSpeed();
  windF.resetWindDirection();
  return result;
}

void mqttSendWindData() {
  if (mqttConnect()) {
    // char buffer[strlen_P(payload) + 30];
    char buffer[100] = {};
    snprintf_P(buffer, sizeof(buffer), dataPayload,
               timeToString().c_str(), windSpeedAverage, windSpeedMax,
               windSpeedMin, windDirectionAverage);

    TLOGDEBUGF_P(PSTR("[MQTT BUFFER] %s\n"), buffer);

    String topic;
    topic = String(appSettings.mqttTopic) + FPSTR(_topicData);
    TLOGDEBUG(topic.c_str());

    if (mqttClient.publish(topic.c_str(), buffer) == true) {
      mqttClient.loop();
      flashChipLed();      
      TLOGDEBUGF_P(PSTR("[MQTT] Success sending message.\n"));
    } else {
      TLOGDEBUGF_P(PSTR("[MQTT] Error sending message.\n"));
    }
  }
}

void sendNow() {
  if (getAndResetWindValues()) {
    // swSer.enableRx(false);
    // swSer.flush();
    mqttSendWindData();
    // swSer.enableRx(true);
  }
}

void mainLoop() {
  ledsLoop();

  static unsigned long last_data_sent = 0;
  if ((last_data_sent > 0) &&
      ((millis() - last_data_sent) < appSettings.mqttTopicDataInterval * 1000))
    return;
  last_data_sent = millis();
  sendNow();
}

void isAlive() {
  TLOGDEBUGF_P(PSTR("[WD] Checking alive status...\n"));
   // if there is no heartbeat for 15 minutes, restart ESP
  if (millis() - lastHeartBeat > 900000) 
  {
    TLOGDEBUGF_P(PSTR("[WD] No HeartBeat for 15 minutes. Restarting...\n"));
    ESP.restart();
    delay(1000);
  }
  TLOGDEBUGF_P(PSTR("[WD] OK.\n"));
}

void switchMosfet(bool value) {
  if (mosfetOn != value) {
    mosfetOn = value;
    digitalWrite(MOSFET_PIN, (mosfetOn) ? MOSFET_ON : MOSFET_OFF);
  }
}

void enableSerial(bool _on) {
  static bool lastValue = false;
  if (lastValue != _on) {
    lastValue = _on;
    swSer.enableRx(_on);
  }
}

void setup() {
  // start with mosfet output switched off
  pinMode(MOSFET_PIN, OUTPUT);
  switchMosfet(MOSFET_OFF);

  // set led pins as output
  pinMode(LED_ONBOARD, OUTPUT);
  digitalWrite(LED_ONBOARD, LED_ONBOARD_OFF);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);

  Serial.begin(115200);
  TLOGDEBUGF_P(PSTR("\n\nStarting %s\n"), APP_NAME);
  Serial.setDebugOutput(true);
  

  tickerWatchDog.attach(60, isAlive);  // check every 1 minute if we are alive.
  initialize();
  // app specific functions

  gill.setSpeedUnit(wsKnots);
  swSer.begin(SWSERIAL_BAUD_RATE, SWSERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX, false,
              SERIAL_BUFFER_SIZE);
              
  // swSer.enableRx(false);
  // enableSerial(false);
}

void loop() {
  
  // when connected first time with wifi send debug info if exist
  if (firstWiFiConnection && WiFi.status() == WL_CONNECTED) {
    firstWiFiConnection = false;
    if (crashSave.crashLogFileExists()) {
      crashSave.printCrashLog();
      crashSave.sendCrashLogToWeb(crash_post_url, "1919");
      if (crashSave.getFSFreeSpace() < 512) {
        crashSave.clearCrashLog();
      }
    }
  }

  wifiLoop();
  appLoop();
  button1.tick();
  oledLoop();
  mainLoop();
  lastHeartBeat = millis();

  // switch on mosfet only first time the mqttClient is connected
  if (!mosfetOn && mqttClient.connected()) {
    switchMosfet(MOSFET_ON);
  }
  // enableSerial(mqttClient.connected());

  if (serialDataReceived()) {
    flashOnBoardLed();
    SerialDataResult_t sdr = gill.decodeSerialData(receivedChars);
    switch (sdr) {
      case srOK:
        addDeviceValues(gill.getSpeed(), gill.getDirection());
        break;
      case srNoControlChars:  // if there is no control char, it's a message
                              // from Gill. Sent it to mqtt status.
        parseDeviceData(receivedChars);
        break;
      default:
        TLOGDEBUGF_P(PSTR("[PARSE] Error %d parsing wind data.\n"), sdr);
        break;
    }
    serialNewData = false;
    // DEBUG_PRINT_F(PSTR("FreeHeap: %d\n"), ESP.getFreeHeap());
  }
}