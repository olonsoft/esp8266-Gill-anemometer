#define APP_NAME    "Anemometer"
#define APP_VERSION "1.1.8"  // Updated 2021-06-03
#define APP_AUTHOR  "dimitris19@gmail.com"
#define APP_WEBSITE "https://github.com/olonsoft/esp8266-Gill-anemometer"


// ======= Software Serial =======
#define SWSERIAL_BAUD_RATE 9600
#define SERIAL_BUFFER_SIZE 256


// ======== pin definitions ========
// LED_BUILTIN          D4    // gpio2. Already defined in pins_arduino.h
#define LED_BUILTIN_ON  LOW
#define LED_BUILTIN_OFF HIGH

// -> note: I connected GPIO16 to Reset so the board can sleep in needed.
//#define LED_ONBOARD     D0    // gpio16 pcb led
//#define LED_ONBOARD_ON  LOW
//#define LED_ONBOARD_OFF HIGH

#define MOSFET_PIN      D7    // gpio13
#define MOSFET_ON       HIGH
#define MOSFET_OFF      LOW

#define PIN_SERIAL_RX   D5    // gpio14
#define PIN_SERIAL_TX   D6    // gpio12

// oled i2c SCL pin     D1    // gpio5
// oled i2c SDA pin     D2    // gpio4 
#define BUTTON1         D3    // gpio0