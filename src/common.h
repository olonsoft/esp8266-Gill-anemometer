#pragma once

#include <arduino.h>
#include <ArduinoJson.h>
#include <FOTA_ESP.h>
#include <FS.h>
#include <WiFiManager.h>  //https://github.com/tzapu/WiFiManager
#include <helper_general.h>
#include <helper_wifi.h>
#include <helper_time.h>
#include <TZ.h>           // for TimeZone define

// needed for justwifi
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <JustWifi.h>
#include <JustWifiUtils.h>
#include <onebutton.h>

struct AppSettings {
  // general
  char     ssid[31];
  char     password[63];
  char     mqttBroker[60];
  uint16_t mqttPort;
  char     mqttUser[20];
  char     mqttPass[63];
  char     mqttTopic[60];               // I use 3 topics /data /stat /cmd
  uint16_t mqttTopicDataInterval;       
  uint16_t mqttTopicStatusInterval;
  char     firmwareUpdateServer[80];
  uint16_t firmwareUpdateInterval;
};

AppSettings appSettings = {
    DEFAULT_WIFI1_SSID,                 // wifi to connect
    DEFAULT_WIFI1_PSW,                  // wifi password
    MQTT_BROKER,                        // mqtt broker
    MQTT_PORT,                          // mqtt port
    MQTT_USER,                          // mqtt user
    MQTT_PSW,                           // mqtt password
    MQTT_TOPIC,                         // mqtt topic
    UPDATE_INTERVAL_DATA,               // data topic update interval
    UPDATE_INTERVAL_STATUS,             // status topic update interval
    FIRMWARE_URL,                       // firmware url
    UPDATE_INTERVAL_CHECK_FW};          // firmware update check interval

String _crash_post_url = CRASH_POST_URL;

#define SEND_DATA 1

#include <EEPROM.h>
#define EEPROM_SIZE 4096  // EEPROM size in bytes
// #define SPI_FLASH_SEC_SIZE      4096

#define MQTT_ENABLED
#define USE_PUBSUBCLIENT

#ifdef  MQTT_ENABLED

#ifdef  USE_PUBSUBCLIENT
#include <PubSubClient.h>
#else
#include <MQTT.h>
#endif

uint16_t   _mqtt_connections;
bool       _mqtt_connected = false;
const char _topicData[]    PROGMEM = {"data"};
const char _topicStatus[]  PROGMEM = {"stat"};
const char _topicCommand[] PROGMEM = {"cmd"};
const char _topicWind[]    PROGMEM = {"wind"};

const char _statusPayload[] PROGMEM =
    "{\"devi\":\"%s\","
    "\"vers\":\"%s\","
    "\"ssid\":\"%s\","
    "\"pass\":\"%s\","
    "\"bssi\":\"%s\","
    "\"rssi\":%d,"
    "\"time\":\"%s\","
    "\"uptm\":\"%s\","
    "\"memo\":%d,"
    "\"rstr\":\"%s\","
    "\"mbro\":\"%s\","
    "\"mprt\":%d,"
    "\"musr\":\"%s\","
    "\"mpsw\":\"%s\","
    "\"mcnn\":%d,"
    "\"volt\":%.1f"
    "}";


WiFiClient wifiClient;

#ifdef USE_PUBSUBCLIENT
PubSubClient mqttClient(wifiClient);
#else
MQTTClient mqttClient(512);
#endif

#endif  // MQTT_ENABLED

String _status_text = "Starting";

#define OLED1306

#ifdef OLED1306
#include <Wire.h>         // Only needed for Arduino 1.6.5 and earlier

#include "SSD1306Wire.h"  // legacy include: `#include "SSD1306.h"`
// #include <brzo_i2c.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Brzo.h"
// Initialize the OLED display using Wire library
SSD1306Wire display(0x3c, D2, D1);

int screenW               = 128;
int screenYH              = 16;   // top yellow part is 16 px height
int screenH               = 64;
bool screenOn             = true;
uint32_t screenOnTime     = 0;
#endif

// ============================== time library ================================

const char *ntpServer = NTP_SERVER;
// timezones: https://remotemonitoringsystems.ca/time-zone-abbreviations.php
#define MY_TIMEZONE TZ_Europe_Athens

// const char *TZ_INFO = "EET-2EEST-3,M3.5.0/03:00:00,M10.5.0/04:00:00";  


// =========================== onebutton ======================================

OneButton button1(BUTTON1, true);
uint32_t _longpress_start_time = 0;

//=============================================================================

String URLEncode2(const char *msg) {
  const char *hex = "0123456789abcdef";
  String encodedMsg = "";

  while (*msg != '\0') {
    if (('a' <= *msg && *msg <= 'z') || ('A' <= *msg && *msg <= 'Z') ||
        ('0' <= *msg && *msg <= '9')) {
      encodedMsg += *msg;
    } else {
      encodedMsg += '%';
      encodedMsg += hex[*msg >> 4];
      encodedMsg += hex[*msg & 15];
    }
    msg++;
  }
  return encodedMsg;
}

String Value2String(int y) { return !isnan(y) ? String(y) : ""; }

String Value2String(float y) { return !isnan(y) ? String(y) : ""; }

void debugConfig() {
  TLOGDEBUGF_P(
      PSTR("\n[CONFIG] ssid: '%s'\n\t pass: '%s'\n\tBroker: %s\n\tPort: "
           "%d\n\tUser: %s\n\tPass: %s\n\tTopic: %s\n\tmqttInterval: "
           "%d\n\tstatusInterval: %d\n\tOTA server: %s\n\tOTA interval: %d\n"),
      appSettings.ssid, appSettings.password, appSettings.mqttBroker,
      appSettings.mqttPort, appSettings.mqttUser, appSettings.mqttPass,
      appSettings.mqttTopic, appSettings.mqttTopicDataInterval,
      appSettings.mqttTopicStatusInterval, appSettings.firmwareUpdateServer,
      appSettings.firmwareUpdateInterval);
}

// ================================== ESP_FS ==================================

bool formatFS() {
  TLOGDEBUGF_P(PSTR("[ESP_FS] Formatting...\n"));
  if (ESP_FS.format()) {
    TLOGDEBUGF_P(PSTR("[ESP_FS] File system formatted !!!\n"));
    return true;
  } else {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Error formating file system.\n"));
    return false;
  }  
}

bool saveConfig() {  
  if (!helper_general::beginFileSystem()) return false; // ? I had to mount again FS. Is it a bug?

  DynamicJsonDocument doc(1024);

  doc[F("ssid")] = appSettings.ssid;
  doc[F("pass")] = appSettings.password;
  doc[F("mbro")] = appSettings.mqttBroker;
  doc[F("mpor")] = appSettings.mqttPort;
  doc[F("musr")] = appSettings.mqttUser;
  doc[F("mpsw")] = appSettings.mqttPass;
  doc[F("mtop")] = appSettings.mqttTopic;
  doc[F("mdin")] = appSettings.mqttTopicDataInterval;
  doc[F("msin")] = appSettings.mqttTopicStatusInterval;
  doc[F("usrv")] = appSettings.firmwareUpdateServer;
  doc[F("uint")] = appSettings.firmwareUpdateInterval;

  serializeJsonPretty(doc, Serial);

  File configFile = ESP_FS.open(String(CONFIGFILE), "w");

  if (!configFile) {
    TLOGDEBUGF_P(
        PSTR("[ESP_FS] Failed to open config file [%s] for writing.\n"),
        CONFIGFILE);
    return false;
  }

  debugConfig();

  if (serializeJson(doc, configFile) > 0) {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Configuration saved to File System.\n"));
  } else {
    TLOGDEBUGF_P(
        PSTR("[ESP_FS] Error saving configuration to File System.\n"));
    return false;
  }

  configFile.close();
  return true;
}

bool loadConfig() {
  TLOGDEBUGF_P(PSTR("[ESP_FS] Loading config from file [%s].\n"), CONFIGFILE);
  if (!helper_general::beginFileSystem()) return false;

  if (!ESP_FS.exists(String(CONFIGFILE))) {
    TLOGDEBUGF_P(PSTR("Config file does not exist. Creating default. [%s]\n"),
                  CONFIGFILE);
    return (saveConfig());
  }

  File configFile = ESP_FS.open(String(CONFIGFILE), "r");
  if (!configFile) {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Failed to open config file [%s].\n"),
                  CONFIGFILE);
    return false;
  }

  size_t size = configFile.size();
  TLOGDEBUGF_P(PSTR("[ESP_FS] Config file size: %d\n"),size);

  if (size > 1024) {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Config file size is too large.\n"));
    return false;
  }

  const size_t capacity = ESP.getFreeHeap() - 3096;
  DynamicJsonDocument doc(capacity);
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Failed to parse config file. Error: %s\n"),
                  error.c_str());
    return false;
  }

  configFile.close();

  serializeJsonPretty(doc, Serial);

  // @todo check if values are null before copying
  strlcpy(appSettings.ssid, doc[F("ssid")] | appSettings.ssid,
          sizeof(appSettings.ssid));
  strlcpy(appSettings.password, doc[F("pass")] | appSettings.password,
          sizeof(appSettings.password));
  strlcpy(appSettings.mqttBroker, doc[F("mbro")] | appSettings.mqttBroker,
          sizeof(appSettings.mqttBroker));
  appSettings.mqttPort = doc[F("mpor")] | appSettings.mqttPort;
  strlcpy(appSettings.mqttUser, doc[F("musr")] | appSettings.mqttUser,
          sizeof(appSettings.mqttUser));
  strlcpy(appSettings.mqttPass, doc[F("mpsw")] | appSettings.mqttPass,
          sizeof(appSettings.mqttPass));
  strlcpy(appSettings.mqttTopic, doc[F("mtop")] | appSettings.mqttTopic,
          sizeof(appSettings.mqttTopic));
  appSettings.mqttTopicDataInterval =
      doc[F("mdin")] | appSettings.mqttTopicDataInterval;
  appSettings.mqttTopicStatusInterval =
      doc[F("msin")] | appSettings.mqttTopicStatusInterval;
  strlcpy(appSettings.firmwareUpdateServer,
          doc[F("usrv")] | appSettings.firmwareUpdateServer,
          sizeof(appSettings.firmwareUpdateServer));
  appSettings.firmwareUpdateInterval =
      doc[F("uint")] | appSettings.firmwareUpdateInterval;
  debugConfig();
  return true;
}

bool deleteConfig() {
  
  if (!helper_general::beginFileSystem()) return false; // ? I had to mount again FS. Is it a bug?

  if (!ESP_FS.exists(String(CONFIGFILE))) {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Config file [%s] does not exist.\n"),
                  CONFIGFILE);
    return false;
  }

  TLOGDEBUGF_P(PSTR("[ESP_FS] Deleting config file [%s].\n"), CONFIGFILE);

  if (ESP_FS.remove(String(CONFIGFILE))) {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Config File deleted.\n"));
    return true;
  } else {
    TLOGDEBUGF_P(PSTR("[ESP_FS] Can not delete Config File.\n"));
    return false;
  }
}

/*  todo: have to check those functions
//
https://github.com/G6EJD/Using-ESP8266-EEPROM/blob/master/ESP8266_Reading_and_Writing_EEPROM.ino/
//
https://arduino.stackexchange.com/questions/25945/how-to-read-and-write-eeprom-in-esp8266

// Loads string from EEPROM. Returns length of string read.
int loadFlashString(uint16_t startAt, char* buffer, size_t maxLen)
{
    EEPROM.begin(EEPROM_SIZE);
    uint16_t i = 0;
    {
        buffer[i] = (char)EEPROM.read(startAt + i);
        Serial.println(buffer[i]);
    } while(i < maxLen && buffer[i++] != 0);
    Serial.println(buffer);
    EEPROM.end();
    if (i >= maxLen) {
        return -1;
    }
    return i;
}

// Saves string to EEPROM. Returns index position after end of string.
int saveFlashString(uint16_t startAt, const String& id)
{
    uint16_t len = id.length();
    if (len == 0) return -1;
    EEPROM.begin(EEPROM_SIZE);
    for (uint16_t i = 0; i <= len; i++)
    {
        EEPROM.write(i + startAt, (uint8_t) id.c_str()[i]);
    }
    EEPROM.write(startAt + len, 0);
    EEPROM.commit();
    EEPROM.end();
    return startAt + len;
}

int saveFlashWiFi(const String& ssid, const String& pass) {
    uint16_t lenSSID = ssid.length();
    uint16_t lenPass = pass.length();
    if (lenSSID > 31 || lenPass > 63) return -1; // invalid ssid or password 
    EEPROM.begin(EEPROM_SIZE);

    // ssid starts at 0. Max is 31 + #0 (31 is the max because 0 is first)
    uint16_t i;
    for (i = 0; i <= lenSSID; i++)
    {
        EEPROM.write(i, (uint8_t) ssid.c_str()[i]);
        Serial.println(ssid.c_str()[i]);
    }
    EEPROM.write(lenSSID, 0);

    // pass starts at 32
    uint16_t j;
    for (j = 0; j<=lenPass; j++) {
        EEPROM.write(32+j, (uint8_t) pass.c_str()[j]);
        Serial.println(ssid.c_str()[j]);
    }
    EEPROM.write(32+lenPass, 0);

    EEPROM.commit();
    EEPROM.end();
    return (i+j);
}

*/

// FOTA section
void onFOTAMessage(fota_t t, char *msg) { TLOGDEBUG(msg); }

void FOTA_Setup() {
  String server = helper_general::addMacAddress(String(appSettings.firmwareUpdateServer));
  server = helper_general::addTrailingSlash(server);
  FOTAClient.setFOTAParameters(server.c_str(), APP_NAME, 
                               APP_VERSION, APP_VERSION);
  FOTAClient.onMessage(onFOTAMessage);
}

void FOTA_Loop() {
  
  if (WiFi.status() != WL_CONNECTED) return;

  static uint32_t last_check = 0;
  if (helper_time::timeReached(last_check)) {
    helper_time::setNextTimeInterval(last_check, appSettings.firmwareUpdateInterval * 1000);
    FOTAClient.checkAndUpdateFOTA(true);
  }

}
// =============================== just wifi start =============================

void jwSetup() {
  jw.setHostname(
      APP_NAME);  // Set WIFI hostname (otherwise it would be ESP_XXXXXX)

  // Callbacks
  jw.subscribe(infoCallback);
  jw.subscribe(mdnsCallback);
  // AP mode only as fallback
  jw.enableAP(false);
  jw.enableAPFallback(false);

  // Enable STA mode (connecting to a router)
  jw.enableSTA(true);

  // Configure it to scan available networks and connect in order of dBm
  jw.enableScan(true);

  // Clean existing network configuration
  jw.cleanNetworks();

  WiFi.printDiag(Serial);

  // Add internal esp8266 saved wifi network
  jw.addCurrentNetwork(true);
  // Add wifi network saved in settings
  jw.addNetwork(appSettings.ssid, appSettings.password);
  // add additional wifi networks
  jw.addNetwork("Yianna Caravel", "2810824724");
  jw.addNetwork("ikaros3", "ikaros2021");
  jw.addNetwork("Support", "12345678");
  // Add an open network
  jw.addNetwork("open");
}

// ============================================================================

void restartESP() {
  ESP_FS.end();  // unmount file system before restart
  delay(1000);
  // ESP.reset();    // is a hard reset and can leave some of the registers in
  // the old state which can lead to problems, its more or less like the reset
  // button on the PC.
  ESP.restart();  // tells the SDK to reboot, so its a more clean reboot, use
                  // this one if possible.
  delay(5000);
}

void resetESP() {
  TLOGDEBUGF_P(PSTR("[RESET] Reset to factory defaults.\n"));
  deleteConfig();
  delay(1000);
  restartESP();
}

// ----- button 1 callback functions

// This function will be called when the button1 was pressed 1 time
void click1() {
  Serial.println("Button 1 click.");
#ifdef OLED1306
  screenOn = true;
  screenOnTime = millis();  
  display.displayOn();
#endif
}  // click1

// This function will be called when the button1 was pressed 2 times in a short
// timeframe.
void doubleclick1() {
  Serial.println("Button 1 doubleclick.");
}  // doubleclick1

// This function will be called once, when the button1 is pressed for a long
// time.
void longPressStart1() {
  Serial.println("Button 1 longPress start");
  _longpress_start_time = millis();
}  // longPressStart1

// This function will be called often, while the button1 is pressed for a long
// time.
void longPress1() {
  // Serial.println("Button 1 longPress...");
}  // longPress1

// This function will be called once, when the button1 is released after beeing
// pressed for a long time.
void longPressStop1() {
  Serial.println("Button 1 longPress stop");
  if (helper_time::timePassedSince(_longpress_start_time) > 5000) {
    resetESP();
  }
}  // longPressStop1

// ===================== setupOneButton =============
void oneButtonSetup() {
  // link the button 1 functions.
  button1.attachClick(click1);
  button1.attachDoubleClick(doubleclick1);
  button1.attachLongPressStart(longPressStart1);
  button1.attachLongPressStop(longPressStop1);
  button1.attachDuringLongPress(longPress1);
}

#ifdef OLED1306

void initSSD1306() {
  display.init();
  display.flipScreenVertically();
  display.setContrast(255);  
}

void _drawWifiQuality(int8_t quality) {
  display.setColor(BLACK);
  display.fillRect(83, 0, 44, 15);
  display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_10);
  if (WiFi.isConnected()) {
    display.drawString(119, 0, String(WiFi.RSSI()) + "dBm");
    for (int8_t i = 0; i < 4; i++) {
      for (int8_t j = 0; j < 2 * (i + 1); j++) {
        if (quality > i * 25 || j == 0) {
          display.setPixel(121 + 2 * i, 9 - j);
        }
      }
    }
  } else {
    display.drawString(58, 0, F("-----"));
  }
  //display.drawLine(83, 15, 128, 15);
  display.drawLine(0, 15, 127, 15);
}

void _oledUpdateStatusText() {
 
  display.setColor(BLACK);
  display.fillRect(0, 0, 82, 15);
  display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (_status_text == "") {
    if (millis()%6000 < 3000) {
      _status_text = WiFi.localIP().toString();
    } else {
      _status_text = WiFi.SSID();
    }
  }
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, _status_text);

  //display.drawLine(0, 16, 82, 16);
  static unsigned long last_check = 0;
  if ((last_check > 0) && ((millis() - last_check) < 3000)) return;
  last_check = millis();
  _status_text = "";
}

void _oledLoop() {
  static uint32_t check_every_sec = 0;
  if (helper_time::timeReached(check_every_sec)) {
    helper_time::setNextTimeInterval(check_every_sec, 1000);
    if (screenOn) {
      _oledUpdateStatusText();
      _drawWifiQuality( helper_wifi::wifiGetRssiAsQuality(WiFi.RSSI()) );
      display.display();
    }
  }
}

#endif

// =============================== MQTT ===============================

#ifdef MQTT_ENABLED

void _statusReport();  // used later
bool _mqttSendMessage(const char *message);

void _mqttOnMessage(char *topic, char *payload, unsigned int len) {
  if (len == 0) return;

  char message[len + 1];
  strlcpy(message, (char *)payload, len + 1);
  message[len] = '\0';
  TLOGDEBUGF_P(PSTR("%sTopic: %s \tMessage: %s\n"), MQTT_STR, topic, message);

  char *command = strtok(message, " ");

  if (strcmp("status", command) == 0) {
    // send status report
    _statusReport();
  };

  if (strcmp("reset", command) == 0) {
    _mqttSendMessage("Reseting...");
    delay(2000);
    resetESP();
    delay(5000);
  }

  if (strcmp("format", command) == 0) {
    _mqttSendMessage("Reseting...");
    delay(2000);
    formatFS();
    delay(5000);
  }

  if (strcmp("restart", command) == 0) {
    _mqttSendMessage("Restarting...");
    delay(2000);
    ESP.restart();
    delay(5000);
  }

  if (strcmp("update", command) == 0) {
    _mqttSendMessage("Updating...");
    FOTAClient.checkAndUpdateFOTA(true);
  }

  if (strcmp("broker", command) ==
      0) {  // command is: broker user:password@domain_or_ip:port
    char *userid = strtok(nullptr, ":");
    char *password = strtok(nullptr, "@");
    char *url = strtok(nullptr, ":");
    char *port = strtok(nullptr, "");

    if (userid != nullptr) {
      if (strcmp("*", userid) > 0) {  // if user or password is blank, use *
        appSettings.mqttUser[0] = 0;
      } else {
        strcpy(appSettings.mqttUser, userid);
      }
    }

    if (password != nullptr) {
      if (strcmp("*", password) > 0) {
        appSettings.mqttPass[0] = 0;
      } else {
        strcpy(appSettings.mqttPass, password);
      }
    }
    if (url != nullptr) {
      strcpy(appSettings.mqttBroker, url);
    }
    if (port != nullptr) {
      appSettings.mqttPort = atoi(port);
    }
    debugConfig();
    _mqttSendMessage("Saving new broker.");
    saveConfig();
  }

  if (strcmp("interval", command) == 0) {
    char *value = strtok(nullptr, "");
    if (value != nullptr) {
      appSettings.mqttTopicDataInterval = atoi(value);
      _mqttSendMessage("Saving new interval.");
      saveConfig();
    }
  }
}

#ifdef USE_PUBSUBCLIENT
void mqttCallback(char *topic, byte *payload, unsigned int length) {
  // handle message arrived
  _mqttOnMessage(topic, (char *)payload, length);
}
#else
void mqttCallback(String &topic, String &payload) {
  // handle message arrived
  _mqttOnMessage(const_cast<char *>(topic.c_str()),
                 const_cast<char *>(payload.c_str()), payload.length());
}
#endif

bool mqttConnect() {
  if (WiFi.status() != WL_CONNECTED) {
    TLOGDEBUGF_P(PSTR("%sNo WiFi connection \n"), MQTT_STR);
    return (_mqtt_connected = false);
  }
  TLOGDEBUGF_P(PSTR("%sConnecting to MQTT... \n"), MQTT_STR);
  _mqtt_connected = mqttClient.connected();
  if (!_mqtt_connected) {
    TLOGDEBUGF_P(PSTR("%sConnecting to: \n\t%s\n\tPort: %d\n\tUser: "
                       "%s\n\tpass: %s\n"), MQTT_STR,
                  appSettings.mqttBroker, appSettings.mqttPort,
                  appSettings.mqttUser, appSettings.mqttPass);
    String clientId = (char *)APP_NAME;
    clientId += "-" + helper_general::getChipIdHex();
#ifdef USE_PUBSUBCLIENT
    mqttClient.setServer(appSettings.mqttBroker, appSettings.mqttPort);
    mqttClient.setCallback(mqttCallback);
#else
    mqttClient.begin(appSettings.mqttBroker, appSettings.mqttPort, wifiClient);
    mqttClient.onMessage(mqttCallback);
#endif
    if (mqttClient.connect(clientId.c_str(), appSettings.mqttUser,
                           appSettings.mqttPass)) {
      _mqtt_connections++;
      TLOGDEBUGF_P(PSTR("%sConnected.\n"), MQTT_STR);
      String topic = helper_general::addMacAddress(String(appSettings.mqttTopic));
      topic = helper_general::addTrailingSlash(topic) + FPSTR(_topicCommand);
      TLOGDEBUGF_P(PSTR("%sSubscribing to topic %s\n"), MQTT_STR, topic.c_str());
      mqttClient.subscribe(topic.c_str());
      _mqtt_connected = true;
    } else {
      int err = 0;
#ifdef USE_PUBSUBCLIENT
      err = mqttClient.state();
#else
      err = int(mqttClient.lastError());
#endif
      TLOGDEBUGF_P(PSTR("%sConnection failed] rc = %d\n"), MQTT_STR, err);
    }
  } else {
    TLOGDEBUGF_P(PSTR("%sAlready connected.\n"), MQTT_STR);
  }
  return _mqtt_connected;
}

bool _mqttSendMessage(const char *message) {
  bool result = false;
  if (mqttConnect()) {
    _status_text = "Sending...";
    String topic;
    topic = helper_general::addMacAddress(String(appSettings.mqttTopic));
    topic = helper_general::addTrailingSlash(topic) + FPSTR(_topicStatus);
    LOGDEBUGLN(topic.c_str());
    result = (mqttClient.publish(topic.c_str(), message) == true);
    mqttClient.loop();
  }
  if (result) {
    TLOGDEBUGF_P(PSTR("%sSuccess sending message.\n"), MQTT_STR);
  } else {
    TLOGDEBUGF_P(PSTR("%sError sending message.\n"), MQTT_STR);
  }
  return result;
}

void _statusReport() {
  int v = analogRead(A0);
  float volt = map(v, 0, 1024, 0, 135) / 10.0f;
  char buffer[400] = {};
  snprintf_P(buffer, sizeof(buffer), _statusPayload, (char *)APP_NAME,
             (char *)APP_VERSION, WiFi.SSID().c_str(),
             "*****",  // WiFi.psk().c_str(),
             WiFi.BSSIDstr().c_str(), WiFi.RSSI(),
             helper_time::timeToString().c_str(), 
             helper_time::getUpTimeString().c_str(),
             ESP.getFreeHeap(),
             String(ESP.getResetReason()).c_str(),
             String(appSettings.mqttBroker).c_str(), 
             appSettings.mqttPort,
             String(appSettings.mqttUser).c_str(),
             "*****",  // String(appSettings.mqttPass).c_str(),
             _mqtt_connections,
             volt);

  LOGDEBUGF("\n[MQTT BUFFER] %s\n", buffer);

  _mqttSendMessage(buffer);
}

void _statusReportLoop() {
  
  if (WiFi.status() != WL_CONNECTED) return;
  if (appSettings.mqttTopicStatusInterval == 0) return;

  static uint32_t last_status_check = 0;
  if (helper_time::timeReached(last_status_check)) {
    helper_time::setNextTimeInterval(last_status_check, appSettings.mqttTopicStatusInterval * 1000);
    _statusReport();
  }
    
}

void _mqttLoop() {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!mqttClient.loop()) {
    static uint32_t last_mqtt_check = 0;

    if (helper_time::timeReached(last_mqtt_check)) {
      helper_time::setNextTimeInterval(last_mqtt_check, 10000);
      TLOGDEBUGF_P(PSTR("%s(mqttloop) Not connected. Reconnecting.\n"), MQTT_STR);
      mqttConnect();
    }

  } else {
    _statusReportLoop();
  }
}

#endif

void initialize() {  
  _crash_post_url = helper_general::addMacAddress(_crash_post_url);  

  // check file system
  TLOGDEBUGF_P(PSTR("[DEBUG] Checking file system.\n"));
  helper_general::beginFileSystem();
  TLOGDEBUGF_P(PSTR("[DEBUG] Print default configuration:\n"));
  debugConfig();

  // loading configuration
  loadConfig();

#ifdef OLED1306
  initSSD1306();
#endif

  TLOGDEBUGF_P("[DEBUG] Printing info:\n"); 
  TLOGDEBUGLN(helper_general::getSystemInfoJson());

  TLOGDEBUGF_P("[DEBUG] Setup oneButton.\n");
  oneButtonSetup();

  // setup wifi
  TLOGDEBUGF_P("[DEBUG] Setup wifi.\n");
  jwSetup();

  // setup OTA updates
  TLOGDEBUGF_P("[DEBUG] Setup FOTA updater.\n");
  FOTA_Setup();

  // start time client
  // ! move it after wifi connection.
  TLOGDEBUGF_P("[DEBUG] Starting ntp client.\n");
  configTime(MY_TIMEZONE, ntpServer);  // updated for > 2.7.0
  //configTime(0, 0, ntpServer);
  //setenv("TZ", TZ_INFO, 1);
  //tzset();  // save the TZ variable
  
  // Give now a chance to the settimeofday callback,
  // because it is *always* deferred to the next yield()/loop()-call.
  yield();
}

void startWiFiManager() {
  TLOGDEBUGF_P("Starting wifi manager.\n");
  char buf[6];
  buf[0] = 0;
  WiFiManagerParameter custom_mqtt_broker("server", "mqtt server",
                                          appSettings.mqttBroker, 60);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port",
                                        itoa(appSettings.mqttPort, buf, 10), 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user",
                                        appSettings.mqttUser, 60);
  WiFiManagerParameter custom_mqtt_password("password", "mqtt password",
                                            appSettings.mqttPass, 60);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt topic",
                                         appSettings.mqttTopic, 60);
  buf[0] = '\0';
  WiFiManagerParameter custom_mqtt_update("update", "update in secs",
      itoa(appSettings.mqttTopicDataInterval, buf, 10), 6);
  WiFiManager wifiManager;
  // wifiManager.resetSettings(); //reset settings - for testing
  // add all your parameters here
  wifiManager.addParameter(&custom_mqtt_broker);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_password);
  wifiManager.addParameter(&custom_mqtt_topic);
  wifiManager.addParameter(&custom_mqtt_update);

  wifiManager.setTimeout(120);  //(in seconds) sets timeout until configuration
                                //portal gets turned off

  // it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration

  // WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to
  // at least 1.5.1 WiFi.mode(WIFI_STA);

  if (!wifiManager.startConfigPortal((char *)APP_NAME)) {
    TLOGDEBUGF_P("failed to connect and hit timeout\n");
    restartESP();
  }
  // if you get here you have connected to the WiFi
  TLOGDEBUGF_P("connected...yeey :)\n");
  WiFi.printDiag(Serial);
  // save SSID and password to flash
  strcpy(appSettings.ssid, WiFi.SSID().c_str());
  strcpy(appSettings.password, WiFi.psk().c_str());
  // save mqtt settings to flash
  strcpy(appSettings.mqttBroker, custom_mqtt_broker.getValue());
  appSettings.mqttPort = atoi(custom_mqtt_port.getValue());
  strcpy(appSettings.mqttUser, custom_mqtt_user.getValue());
  strcpy(appSettings.mqttPass, custom_mqtt_password.getValue());
  strcpy(appSettings.mqttTopic, custom_mqtt_topic.getValue());
  appSettings.mqttTopicDataInterval = atoi(custom_mqtt_update.getValue());
  debugConfig();
  saveConfig();  // save configuration and restart esp
  restartESP();
}

void wifiLoop() {
  if (!captivePortal) {
    jw.loop();
    if (jw.connected()) {
      FOTA_Loop();  // check for firmware updates
    }
  } else {
    startWiFiManager();
  }
}

void appLoop() {
#ifdef OLED1306
  _oledLoop();
#endif

#ifdef MQTT_ENABLED
  // check for messages every second
  static uint32_t  check_every_sec = 0;
  if (helper_time::timeReached(check_every_sec)) {
    helper_time::setNextTimeInterval(check_every_sec, 1000);
    _mqttLoop(); 
  }
#endif
}
