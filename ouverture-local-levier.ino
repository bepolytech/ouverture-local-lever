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

#include <ArduinoJWT.h>
// OR ?
#include <CustomJWT.h>
char secret_key[] = "tester";
char string[] = "{\"temp\":22.5,\"speed\":25.1}";
CustomJWT jwt2(secret_key, 256);

#define WIDI_SSID "example";
#define WIFI_PSWD "example";
#define NTP_SERVER "pool.ntp.org";
#define NTP_OFFSET "000"; // GMT +1 Brussels time
#define API_SERVER "example";
//#define API_CRED "examplecredentials";

// Secret code
ArduinoJWT jwt1 = ArduinoJWT("secret");

void setup() {
  // setup wifi, NTP, API, etc
  
  
  Serial.begin(115200);
  
  // --- JWT encode --- :

  // Convert JSON payload to verified JWT
  char input1[] = "{\"name\": \"John Doe\",\"email\": \"john.doe@example.com\",\"iat\": 1630182518}";
  int input1Len = jwt1.getJWTLength(input1);
  char output1[input1Len];
  jwt1.encodeJWT(input1, output1);
  Serial.println("\nEncoded JWT:");
  Serial.println(output1);
  
  // OR ?
  
  jwt2.allocateJWTMemory();
  jwt2.encodeJWT(string);
  Serial.printf("Header: %s\nHeader Length: %d\n", jwt2.header, jwt2.headerLength);
  Serial.printf("Payload: %s\nPayload Length: %d\n", jwt2.payload, jwt2.payloadLength);
  Serial.printf("Signature: %s\nSignature Length: %d\n", jwt2.signature, jwt2.signatureLength);
  Serial.printf("Final Output: %s\nFinalOutput Length: %d\n", jwt2.out, jwt2.outputLength);
  jwt2.clear();
  
  // ------------------
  
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
