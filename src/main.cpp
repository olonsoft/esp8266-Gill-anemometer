/*
   ___  _             ____         __ _
  / _ \| | ___  _ __ / ___|  ___  / _| |_
 | | | | |/ _ \| '_ \\___ \ / _ \| |_| __|
 | |_| | | (_) | | | |___) | (_) |  _| |_
  \___/|_|\___/|_| |_|____/ \___/|_|  \__|

*/

#include <Arduino.h>
#include <project_config.h>
#include <ESPcrashSave.h>
#include <SoftwareSerial.h>
#include <Ticker.h>
#include <helper_general.h>
#include <common.h>
#include <WindFunc.h>
#include <Gill.h>

ESPCrashSave crashSave;

// ===================== Serial ======================
SoftwareSerial swSer;

char      _serial_data[SERIAL_BUFFER_SIZE];  // an array to store the received data
bool      _serial_has_new_data = false;
bool      _is_serial_enabled   = false;

// ===================== wind data ==================

Gill      gill;
WindFunc  windF;

uint16_t  _wind_direction_current;
uint16_t  _wind_direction_average;
float     _wind_speed_current;
float     _wind_speed_average;
float     _wind_speed_max;
float     _wind_speed_min;

const char _payload_wind_data_instant[] PROGMEM =
    "{\"time\":\"%s\","
    "\"speed\":%.2f,"
    "\"dir\":%d"
    "}";

const char _payload_wind_data_average[] PROGMEM =
    "{\"time\":\"%s\","
    "\"speed\":%.2f,"
    "\"gust\":%.2f,"
    "\"lull\":%.2f,"
    "\"dir\":%d"
    "}";

bool      _is_chip_led_on     = false;
uint32_t  _chip_led_on_time   = 0;

// set it true because first call on setup will set it off
bool      _is_mosfet_on       = true;

bool      _is_first_wifi_connection = true;

uint32_t  _last_check_alive_time = 0;

Ticker    tickerWatchDog;

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

  float angle = (float)_wind_direction_current;
  angle = (angle / 57.29577951);  // Convert degrees to radians
  int x1 = (clockCenterX + (sin(angle) * clockRadius));
  int y1 = (clockCenterY - (cos(angle) * clockRadius));

  int x2 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 2))));
  int y2 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 2))));
  display.drawLine(x1, y1, x2, y2);

  angle = (float)_wind_direction_current - 15;
  if (angle < 0) angle = angle + 358;
  angle = (angle / 57.29577951);  // Convert degrees to radians
  x2 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 3))));
  y2 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 3))));
  display.drawLine(x1, y1, x2, y2);

  angle = (float)_wind_direction_current + 15;
  if (angle > 359) angle = angle - 360;
  angle = (angle / 57.29577951);  // Convert degrees to radians
  x2 = (clockCenterX + (sin(angle) * (clockRadius - (clockRadius / 3))));
  y2 = (clockCenterY - (cos(angle) * (clockRadius - (clockRadius / 3))));
  display.drawLine(x1, y1, x2, y2);

  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(clockCenterX, clockCenterY - 6,
                     String(_wind_direction_current));

  // display tmpWindSpeed, _wind_speed_min, _wind_speed_max
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(50, 16, "Spd: " + String(_wind_speed_current));
  display.drawString(50, 32, "Min: " + String(_wind_speed_min));
  display.drawString(50, 48, "Max: " + String(_wind_speed_max));


}

// oled screen screen saver and draw compass if on.
void oledLoop() {
  static uint32_t timeToCheck = 0;
  // need to proceed not less than 1000ms
  if (helper_time::timeReached(timeToCheck)) {
    helper_time::setNextTimeInterval(timeToCheck, 1000);

    if (screenOn && helper_time::timePassedSince(screenOnTime) > SCREEN_SAVER_TIME) {
      screenOn = false;
      display.displayOff();
      _status_text = "";
    }

    if (screenOn) {
      _oledUpdateStatusText();
      _drawWifiQuality( helper_wifi::wifiGetRssiAsQuality(WiFi.RSSI()) );
      drawCompass();
      display.display();
    }
  }


}

void flashChipLed() {
  digitalWrite(LED_BUILTIN, LED_BUILTIN_ON);
  _chip_led_on_time = millis();
  _is_chip_led_on = true;
}

void ledsLoop() {
  if (_is_chip_led_on && helper_time::timePassedSince(_chip_led_on_time) > 100) {
    digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);
    _is_chip_led_on = false;
  }
}

bool serialDataReceived() {
  static uint16_t ndx = 0;
  // device sends <CR><LF> or <CR> depending on setting Lx
  // common is <CR> so I check for <CR> as ending char
  char LF = '\n';
  char CR = '\r';
  char rc;

  while (swSer.available() && !_serial_has_new_data) {
    rc = swSer.read();
    if (rc != CR) {
      if (rc != LF) {
        _serial_data[ndx++] = rc;
        if (ndx >= SERIAL_BUFFER_SIZE) ndx--;
      }
    } else {
      _serial_data[ndx] = '\0';  // terminate the string
      ndx = 0;
      _serial_has_new_data = true;
    }
  }
  return (_serial_has_new_data == true);
}

bool mqttSendDeviceData(char *str) {
  if (mqttConnect()) {
    LOGDEBUG(str);

    String topic = helper_general::addMacAddress(String(appSettings.mqttTopic));
    topic = helper_general::addTrailingSlash(topic) + FPSTR(_topicStatus);

    if (mqttClient.publish(topic.c_str(), str) == true) {
      //flashChipLed();
      TLOGDEBUGF_P(PSTR("%sSuccess sending message.\n"), MQTT_STR);
      return true;
    } else {
      TLOGDEBUGF_P(PSTR("%sError sending message.\n"), MQTT_STR);
    }
  }
  return false;
}

void parseDeviceData(char *str) { mqttSendDeviceData(str); }

void addDeviceValues(float wSpeed, uint16_t wDir) {
  windF.addWindSpeed(wSpeed);
  windF.addWindDirection(wDir);
  windF.getWindSpeed(_wind_speed_current, _wind_speed_average, _wind_speed_min,
                     _wind_speed_max);
  _wind_direction_current = wDir;
}

bool getAndResetWindValues() {
  bool result = false;
  float tmpWindSpeedCurrent;
  if (windF.getWindSpeed(tmpWindSpeedCurrent, _wind_speed_average, _wind_speed_min,
                         _wind_speed_max))
    result = true;
  if (result) {
    uint16_t tmpWindDirectionCurrent;
    result =
        windF.getWindDirection(tmpWindDirectionCurrent, _wind_direction_average);
  }
  windF.resetWindSpeed();
  windF.resetWindDirection();
  return result;
}

void mqttSendCurrentWindData(float speed, int direction) {
  if (mqttConnect()) {
    // char buffer[strlen_P(payload) + 30];
    char buffer[100] = {};
    snprintf_P(buffer, sizeof(buffer), _payload_wind_data_instant,
               helper_time::timeToString().c_str(), speed, direction);

    TLOGDEBUGF_P(PSTR("%sBuffer: %s\n"), MQTT_STR, buffer);

    String topic = helper_general::addMacAddress(String(appSettings.mqttTopic));
    topic = helper_general::addTrailingSlash(topic) + FPSTR(_topicWind);
    TLOGDEBUGF_P(PSTR("%sTopic: %s\n"), MQTT_STR, topic.c_str());

    if (mqttClient.publish(topic.c_str(), buffer) == true) {
      TLOGDEBUGF_P(PSTR("%sSuccess sending message.\n"), MQTT_STR);
    } else {
      TLOGDEBUGF_P(PSTR("%sError sending message.\n"), MQTT_STR);
    }
  }
}

void mqttSendWindData() {
  if (mqttConnect()) {
    // char buffer[strlen_P(payload) + 30];
    char buffer[100] = {};
    snprintf_P(buffer, sizeof(buffer), _payload_wind_data_average,
               helper_time::timeToString().c_str(), _wind_speed_average,
                _wind_speed_max, _wind_speed_min, _wind_direction_average);

    TLOGDEBUGF_P(PSTR("%sBuffer: %s\n"), MQTT_STR, buffer);

    String topic = helper_general::addMacAddress(String(appSettings.mqttTopic));
    topic = helper_general::addTrailingSlash(topic) + FPSTR(_topicData);
    TLOGDEBUGF_P(PSTR("%sTopic: %s\n"), MQTT_STR, topic.c_str());

    if (mqttClient.publish(topic.c_str(), buffer) == true) {
      //flashChipLed();
      TLOGDEBUGF_P(PSTR("%sSuccess sending message.\n"), MQTT_STR);
    } else {
      TLOGDEBUGF_P(PSTR("%sError sending message.\n"), MQTT_STR);
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

void isAlive() {
  TLOGDEBUGF_P(PSTR("[WD] Checking alive status... "));
   // if there is no heartbeat for 15 minutes, restart ESP
  if (helper_time::timePassedSince(_last_check_alive_time) > CHECK_ALIVE_INTERVAL) {
    TLOGDEBUGF_P(PSTR("\n[WD] No HeartBeat for 15 minutes. Restarting...\n"));
    ESP.restart();
    delay(1000);
    return;
  }
  TLOGDEBUGF_P(PSTR("OK.\n"));
}

void switchMosfet(bool value) {
  if (_is_mosfet_on != value) {
    _is_mosfet_on = value;
    digitalWrite(MOSFET_PIN, (_is_mosfet_on) ? MOSFET_ON : MOSFET_OFF);
  }
}

void enableSerial(bool value) {
  static bool is_serial_enabled = false;
  if (is_serial_enabled != value) {
    is_serial_enabled = value;
    swSer.enableRx(value);
  }
}

void setup() {
  Serial.begin(115200);
  TLOGDEBUGF_P(PSTR("\n\nStarting %s\n"), APP_NAME);
  Serial.setDebugOutput(true);

  // wifi connection/reconnection will be managed by jw
  WiFi.persistent(false);
  WiFi.disconnect(true);
  ESP.eraseConfig();
  WiFi.setAutoConnect(false);
  WiFi.setAutoReconnect(false);
  delay(10);

  // start with mosfet output switched off
  pinMode(MOSFET_PIN, OUTPUT);
  switchMosfet(MOSFET_OFF);

  // set led pins as output
  // removed in later versions. I used this pin for wake up from sleep
  //pinMode(LED_ONBOARD, OUTPUT);
  //digitalWrite(LED_ONBOARD, LED_ONBOARD_OFF);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_BUILTIN_OFF);

  // check every 1 minute if we are alive.
  tickerWatchDog.attach(60, isAlive);

  initialize();
  // app specific functions

  gill.setWindSpeedUnit(WindSpeedUnit::Knots);

  swSer.begin(SWSERIAL_BAUD_RATE, SWSERIAL_8N1, PIN_SERIAL_RX, PIN_SERIAL_TX, false,
              SERIAL_BUFFER_SIZE);
  enableSerial(false);

}

void loop() {

  // when connected first time with wifi send debug info if exist
  if (_is_first_wifi_connection && WiFi.status() == WL_CONNECTED) {
    _is_first_wifi_connection = false;
    if (crashSave.crashLogFileExists()) {
      crashSave.printCrashLog();
      if (crashSave.sendCrashLogToWeb(_crash_post_url, CRASH_POST_PASSWORD) == 200
          || crashSave.getFSFreeSpace() < 512) {
        crashSave.clearCrashLog();
      }
    }
  }

  // switch on mosfet only when mqttClient is connected
  if (mqttClient.connected() && (WiFi.status() == WL_CONNECTED)) {
    enableSerial(true);
    switchMosfet(MOSFET_ON);
  }
  else {
    switchMosfet(MOSFET_OFF);
    enableSerial(false);
  }

  wifiLoop();
  #ifdef OLED1306
  oledLoop();
  #endif
  mqttLoop();
  button1.tick();

  if (serialDataReceived()) {
    flashChipLed();
    SerialDataResult sdr = gill.decodeSerialData(_serial_data);
    switch (sdr) {
      case SerialDataResult::Ok:
        mqttSendCurrentWindData(gill.getSpeed(), gill.getDirection());
        addDeviceValues(gill.getSpeed(), gill.getDirection());
        break;
      case SerialDataResult::NoControlChars:  // if there is no control char, it's a message
                              // from Gill. Sent it to mqtt status.
        parseDeviceData(_serial_data);
        break;
      default:
        TLOGDEBUGF_P(PSTR("[PARSE] Error %d parsing wind data.\n"), sdr);
        break;
    }
    _serial_has_new_data = false;
    // DEBUG_PRINT_F(PSTR("FreeHeap: %d\n"), ESP.getFreeHeap());
  }

  ledsLoop();

  static uint32_t timeToSendData = appSettings.mqttTopicDataInterval * 1000;
  if (helper_time::timeReached(timeToSendData)) {
    helper_time::setNextTimeInterval(timeToSendData, appSettings.mqttTopicDataInterval * 1000);
    sendNow();
  }

  // All ok. Mark alive.
  _last_check_alive_time = millis();

}