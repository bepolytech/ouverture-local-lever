/*
 * MIT License
 * by Lucas Placentino
 * for the Bureau Etudiant de Polytechnique (BEP) ULB
 *
 * Open status lever code for esp8266
 * 
 * Get lever state (and temp&hum data) every X seconds, then send data to api server
 * (then the Wordpress site will get the status from the api server)
 * 
 */
 
#include <ESP8266WiFi.h>
//TODO check diff between ESP8266WiFi and WiFiUdp ?
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <AHTxx.h>

/*
//#include <ArduinoJWT.h>
// OR ?
//#include <CustomJWT.h>
//char secret_key[] = "tester";
//char string[] = "{\"temp\":22.5,\"speed\":25.1}";
//CustomJWT jwt2(secret_key, 256);
// Secret code
//ArduinoJWT jwt1 = ArduinoJWT("secret");
*/

const char *ssid = "<SSID>"; // wifi ssid
const char *password = "<PASSWORD>"; // wifi password
#define NTP_SERVER "pool.ntp.org"; // europe.pool.ntp.org ?
#define NTP_OFFSET 3600; // UTC+1 (Brussels time CET) = 3600 seconds offset
#define API_SERVER "https://api.example.org"; // api server url
const char api_key[] = "test"; //? const char *api_key = ... ?
#define leverPin 1; // pin on which the lever is wired // TODO: check wemos D1 mini pins
#define REFRESH_TIME 30000; //ms between each refresh and api call (30sec)
  
// Temp sensor
AHTxx aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR); //sensor address, sensor type (we use an aht21) TODO: check address? (from example)
float ahtValue; 

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET);
// NTP epoch time
unsigned long epochTime;

// global vars
unsigned int hum;
int temp;
int door;

void setup() {
  // setup wifi, NTP, API, etc

  // start serial bus
  Serial.begin(115200);

  // start wifi
  initWifi();
  
  // initially door is UNKNOWN (=2)
  door = 2;
  
  // set lever pin to input (pulled high)
  pinMode(leverPin,INPUT_PULLUP);
  
  // check if aht sensor is working
  while (aht20.begin() != true) { //for ESP-01 use aht20.begin(0, 2);
    Serial.println(F("AHT2x not connected or fail to load calibration coefficient")); //(F()) save string to flash & keeps dynamic memory free
    delay(3000); // 3sec
  }
  Serial.println(F("AHT20 OK"));

  /* JWT encoding not used
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
  */

  // start NTP
  timeClient.begin();
}

void loop() {
  // check every X seconds if the lever is closed, and send status with current time
  Serial.println(F("Checking lever status"));
  // updates NTP
  timeClient.update();
  
  // check if lever is closed (door open)
  if ( digitalRead(leverPin) == LOW ) {
    door = 1; // door is open (lever activated, because pulled high)
  } else {
    door = 0;
  }
  
  // update unix time
  epochTime = getEpochTime();

  // update human readable time
  String update_time;
  update_time = statusTime();
  
  // getting temp and hum data
  ahtValue = aht20.readTemperature(); //read 6-bytes via I2C, takes 80 milliseconds
  Serial.print(F("Temperature: "));
  if (ahtValue != AHTXX_ERROR) { //AHTXX_ERROR = 255, library returns 255 if error occurs
    Serial.print(ahtValue);
    Serial.println(F(" +-0.3C"));
    temp = round(ahtValue); //because temp must be an int
  } else { // if error occurs
    printAhtStatus(); //print temperature command status
  }
  ahtValue = aht20.readHumidity(AHTXX_USE_READ_DATA); //use 6-bytes from temperature reading, takes zero milliseconds!!!
  Serial.print(F("Humidity...: "));
  if (ahtValue != AHTXX_ERROR) { //AHTXX_ERROR = 255, library returns 255 if error occurs 
    Serial.print(ahtValue);
    Serial.println(F(" +-2%"));
    hum = round(ahtValue); //because hum must be an int
  } else { // if error occurs
    printAhtStatus(); //print temperature command status not humidity!!! RH measurement use same 6-bytes from T measurement
  }
  
  // send status as json to api server
  result = sendStatus(&update_time);
  // checks is error occured on api PUT
  if (result == 0) {
    Serial.print(F("sendStatus success"));
  } else {
    Serial.print(F("sendStatus error"));
  }

  delay(REFRESH_TIME) // see #defines
}

void initWifi() {
  // init wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println(F("Connecting to WiFi"))
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print(F("Connected to the WiFi network: "));
  Serial.println(ssid);
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());
}

unsigned long getEpochTime() { //! OFFSET IS APPLIED TO EPOCH TIME AS WELL
  // get epoch time from NTP
  unsigned long now = timeClient.getEpochTime();
  Serial.print(F("Epoch time: "));
  Serial.println(now);
  return now;
}

string statusTime() {
  // get formatted string time (human readable) from NTP
  char now[] = timeClient.getFormattedTime();
  Serial.print(F("Formatted time: "));
  Serial.println(now);
  return now;
}

int sendStatus(String* time_human) {
  // send data to api server
  DynamicJsonDocument json(200);

  // add data to json packet
  json["door_state"] = door;
  json["temperature"] = temp;
  json["humidity"] = hum;
  json["update_time"] = &time_human;
  json["update_time_unix"] = time_epoch;
  //json["presence"] = presence; // from PIR sensor ? nahh

  String jsonData;
  serializeJson(json, jsonData);

  HTTPClient http;
  http.begin(API_SERVER);
  http.addHeader("Content-Type", "application/json");
  // apply API key as a header named "api_token"
  http.addHeader("api_token", API_KEY);
  Serial.print(jsonData);
  int httpResponseCode = http.PUT(jsonData);
  int res;
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println(httpResponseCode);
    Serial.println(response);
    res = 0;
  } else {
    Serial.println(F("Error on HTTP request"));
    Serial.println(httpResponseCode);
    res = 1;
  }
  http.end();
  // return successful api call or not
  return res;
}

// print temp sensor error if one occurs
void printAhtStatus() {
  switch ( aht20.getStatus() ) {
    case AHTXX_NO_ERROR:
      Serial.println(F("no error"));
      break;

    case AHTXX_BUSY_ERROR:
      Serial.println(F("sensor busy, increase polling time"));
      break;

    case AHTXX_ACK_ERROR:
      Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_DATA_ERROR:
      Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_CRC8_ERROR:
      Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      break;

    default:
      Serial.println(F("unknown status"));    
      break;
  }
}
