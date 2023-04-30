/*
 * PARAMS
 */

// OLED screen
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin, -1=no reset pin
#define SCREEN_ADDRESS 0x3C // I2C address (0X3C or 0x3D ?) // use I2CScan func to find

// params
#define NTP_SERVER "pool.ntp.org" // europe.pool.ntp.org ?

//deprecated:
//#define NTP_OFFSET 3600 // UTC+1 (Brussels time CET) = 1*3600 seconds offset

#define LEVER_PIN 14 // or D5 (for nodeMCU) // pin on which the lever is wired // TODO: check wemos D1 mini pins ?
//#define LED_PIN 2 // or D4 (for nodeMCU) // built-in LED is GPIO 2 on NodeMCU v3, use LED_BUILTIN ?
#define BTN_PIN 12 // or D6 (for NodeMCU) // TODO
#define PIR_PIN 10 // or SD3 (fore NodeMCU) // TODO
#define REFRESH_TIME 5000 //ms between each refresh and api call (30sec, 5sec for testing)
#define PIR_DELAY 3000 //minimum time between PIR detections

// defined in Secrets.h :
//#define API_SERVER "http://192.168.137.100:8000/local" // api server url
//const char *ssid = "wifissid"; // wifi ssid
//const char *password = "wifipasswd"; // wifi password
//const char api_key[] = "yourapikey"; // make it a *pointer ?