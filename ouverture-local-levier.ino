/*
 * MIT License
 * by Lucas Placentino
 *
 * Open status lever code for esp8266
 * 
 * Get lever state every X seconds/minutes, send data to api server
 * (then the Wordpress site will get the status from the api server)
 * 
 */

#define WIDI_SSID "example";
#define WIFI_PSWD "example";
#define NTP_SERVER "pool.ntp.org";
#define NTP_OFFSET "000"; // GMT +1 Brussels time
#define API_SERVER "example";
#define API_CRED "examplecredentials";


void setup() {
  // setup wifi, NTP, API, etc
}

void loop() {
  // check every X seconds/minutes if the lever is closed, and send status with current time
}

string statusTime() {
  // get formatted string time from NTP
}

int sendStatus() {
  // send data to api server
}
