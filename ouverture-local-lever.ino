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
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

//#include <ArduinoJWT.h>
// OR ?
//#include <CustomJWT.h>
char secret_key[] = "tester";
//char string[] = "{\"temp\":22.5,\"speed\":25.1}";
//CustomJWT jwt2(secret_key, 256);

const char *ssid = "<SSID>";
const char *password = "<PASSWORD>";
#define NTP_SERVER "pool.ntp.org"; // europe.pool.ntp.org ?
#define NTP_OFFSET 3600; // UTC+1 (Brussels time CET) = 3600 seconds offset
#define API_SERVER "example";
const char api_key[] = "test"; //? const char *api_key = ... ?

// Secret code
//ArduinoJWT jwt1 = ArduinoJWT("secret");

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET);
// NTP epoch time
unsigned long epochTime;

void setup() {
  // setup wifi, NTP, API, etc
  Serial.begin(115200);
  initWifi();

  //// --- JWT encode --- :
//
  //// Convert JSON payload to verified JWT
  //char input1[] = "{\"name\": \"John Doe\",\"email\": \"john.doe@example.com\",\"iat\": 1630182518}";
  //int input1Len = jwt1.getJWTLength(input1);
  //char output1[input1Len];
  //jwt1.encodeJWT(input1, output1);
  //Serial.println("\nEncoded JWT:");
  //Serial.println(output1);
  //
  //// OR ?
  //
  //jwt2.allocateJWTMemory();
  //jwt2.encodeJWT(string);
  //Serial.printf("Header: %s\nHeader Length: %d\n", jwt2.header, jwt2.headerLength);
  //Serial.printf("Payload: %s\nPayload Length: %d\n", jwt2.payload, jwt2.payloadLength);
  //Serial.printf("Signature: %s\nSignature Length: %d\n", jwt2.signature, jwt2.signatureLength);
  //Serial.printf("Final Output: %s\nFinalOutput Length: %d\n", jwt2.out, jwt2.outputLength);
  //jwt2.clear();
  //
  //// ------------------

  timeClient.begin();
}

void loop() {
  // check every X seconds/minutes if the lever is closed, and send status with current time
  Serial.println("Checking lever status");
  timeClient.update();

  if 

  delay(20000) // 20 seconds
}

void initWifi() {
  // init wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi")
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to the WiFi network, IP:");
  Serial.println(WiFi.localIP());
}

unsigned long getEpochTime() { //! OFFSET IS APPLIED TO EPOCH TIME AS WELL
  // get epoch time from NTP
  unsigned long now = timeClient.getEpochTime();
  Serial.print("Epoch time: ");
  Serial.println(now);
  return now;
}

string statusTime() {
  // get formatted string time from NTP
  char now[] = timeClient.getFormattedTime();
  Serial.print("Formatted time: ");
  Serial.println(now);
  return now;
}

int sendStatus() {
  // send data to api server

}
