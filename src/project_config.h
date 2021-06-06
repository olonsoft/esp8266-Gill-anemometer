#pragma once

#define APP_NAME                  "Anemometer"
#define APP_VERSION               "1.1.9"  // Updated 2021-06-04
#define APP_AUTHOR                "dimitris19@gmail.com"
#define APP_WEBSITE               "https://github.com/olonsoft/esp8266-Gill-anemometer"

// ======= Location where debug log is saved =======
#define CRASH_POST_URL            "http://fw.crete.ovh/anemometer/gill01/debug.php"
#define CRASH_POST_PASSWORD       "1919"

// ======= MQTT =======
#define MQTT_TOPIC                "olonsoft/devices/anemometer/gill01/"
#define MQTT_BROKER               "test.mosquitto.org"
#define MQTT_USER                 ""
#define MQTT_PSW                  ""

// ======= FOTA =======
#define FIRMWARE_URL              "http://fw.crete.ovh/anemometer/gill01"

// ======= WIFI =======
#ifndef DEFAULT_WIFI1_SSID
#define DEFAULT_WIFI1_SSID        "wifiSSID1"          // Enter your Wifi network SSID
#endif
#ifndef DEFAULT_WIFI1_PSW
#define DEFAULT_WIFI1_PSW         "wifiPASS1"          // Enter your Wifi network 
#endif

// ======= General options =======
#define CHECK_ALIVE_INTERVAL      900000          // 90 seconds
#define SCREEN_SAVER_TIME         120000          // 120 seconds

// ======= Software Serial =======
#define SWSERIAL_BAUD_RATE        9600
#define SERIAL_BUFFER_SIZE        256

// ====== Configuration file =======
#define CONFIGFILE                "/config.json"

// ======== pin definitions ========
// LED_BUILTIN          D4    // gpio2. Already defined in pins_arduino.h
#define LED_BUILTIN_ON            LOW
#define LED_BUILTIN_OFF           HIGH

// -> note: I connected GPIO16 to Reset so the board can sleep if needed.
//#define LED_ONBOARD             D0    // gpio16 pcb led
//#define LED_ONBOARD_ON          LOW
//#define LED_ONBOARD_OFF         HIGH

#define MOSFET_PIN                D7    // gpio13
#define MOSFET_ON                 HIGH
#define MOSFET_OFF                LOW

#define PIN_SERIAL_RX             D5    // gpio14
#define PIN_SERIAL_TX             D6    // gpio12

// oled i2c SCL pin               D1    // gpio5
// oled i2c SDA pin               D2    // gpio4 
#define BUTTON1                   D3    // gpio0