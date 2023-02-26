/*
 * MIT License
 * by Lucas Placentino
 * for the Bureau Etudiant de Polytechnique (BEP) ASBL - ULB
 *
 * Open status lever code for esp8266
 * 
 * Get lever state (and temp&hum data) every X seconds, then send data to api server
 * (then the Wordpress site will get the status from the api server and show it to users)
 * 
 */
 
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h> //! TODO
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <AHTxx.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <SPI.h> // required here (even if we're only using I2C)
#include <Wire.h>

#include "Secrets.h" // TODO: check if this is the right way to do it

// OLED screen
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin
#define SCREEN_ADDRESS 0x3C // I2C address (0X3C or 0x3D ?)

// params
#define NTP_SERVER "pool.ntp.org" // europe.pool.ntp.org ?
#define NTP_OFFSET 3600 // UTC+1 (Brussels time CET) = 1*3600 seconds offset
#define LEVER_PIN 4 // or D2 (for nodeMCU) // pin on which the lever is wired // TODO: check wemos D1 mini pins ?
//#define LED_PIN 2 // or D4 (for nodeMCU) // built-in LED is GPIO 2 on NodeMCU v3, use LED_BUILTIN ?
#define REFRESH_TIME 5000 //ms between each refresh and api call (30sec)

// defined in Secrets.h :
//#define API_SERVER "http://192.168.137.100:8000/local" // api server url
//const char *ssid = "wifissid"; // wifi ssid
//const char *password = "wifipasswd"; // wifi password
//const char api_key[] = "yourapikey"; // make it a *pointer ?

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

HTTPClient http; //https? TODO
WiFiClient wifi_client;

//WiFiClientSecure *secure_client = new WiFiClientSecure; // TODO


//WiFiClientSecure wifiSecure;
//HttpClient client(wifiSecure, "api.bepolytech.be", 443);

//X509List cert(cert_DigiCert_Global_Root_CA); //nah

// screen init
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  // setup wifi, NTP, API, etc

  // start serial bus
  Serial.begin(9600);

  // initialize the OLED object
  while (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    delay(5000); // Don't proceed, loop forever
  }
  // Clear and setup the screen buffer.
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // draw white screen
  display.setCursor(0,0);
  display.fillRect(0, 0, 127, 63, WHITE);
  display.display();
  delay(50);

  // Display Setup Text
  display.clearDisplay();
  display.setCursor(0,28);
  display.println(F("Starting up..."));
  display.display();
  delay(500);

  // start wifi
  initWifi();

  //config ntp for x.509 validation (for https)
  //configTime(NTP_OFFSET, 0, "be.pool.ntp.org", "europe.pool.ntp.org", "pool.ntp.org");
  /*
  if(secure_client) { // TODO
    // set secure client without certificate
    secure_client->setInsecure();

    ////HTTPClient https;
  */


  // initially door is UNKNOWN (=2)
  door = 2;
  
  // set lever pin to input (pulled high)
  pinMode(LEVER_PIN,INPUT_PULLUP);

  //pinMode(LED_PIN,OUTPUT);
  pinMode(LED_BUILTIN,OUTPUT);
  
  // check if aht sensor is working
  while (aht20.begin() != true) { //for ESP-01 use aht20.begin(0, 2);
    Serial.println(F("AHT2x not connected or fail to load calibration coefficient")); //(F()) save string to flash & keeps dynamic memory free
    delay(3000); // 3sec
  }
  Serial.println(F("AHT20 OK"));

  // start NTP
  timeClient.begin();

  // clear display
  display.clearDisplay();
}

void loop() {
  // check every X seconds if the lever is closed, and send status with current time
  Serial.println(F("Checking lever status"));
  // updates NTP
  timeClient.update();
  
  // check if lever is closed (door open)
  if ( digitalRead(LEVER_PIN) == LOW ) {
    door = 1; // door is open (lever activated, because pulled high)
    //digitalWrite(LED_PIN, LOW); // LED ON, because active low
    digitalWrite(LED_BUILTIN, LOW); // LED ON, because active low
    Serial.println(F("Door is open"));
  } else {
    door = 0;
    //digitalWrite(LED_PIN, HIGH); // LED OFF, because active low
    digitalWrite(LED_BUILTIN, HIGH); // LED OFF, because active low
    Serial.println(F("Door is closed"));
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
  
  String info = "No info";

  // send status as json to api server
  int result;
  result = sendStatus(update_time, info);
  // checks is error occured on api PUT
  if ( result == 0) {
    Serial.println(F("sendStatus success"));
  } else {
    Serial.println(F("sendStatus error"));
  }

  //display current status on OLED screen
  displayStatus(result, update_time);

  delay(REFRESH_TIME); // see #defines
}

void initWifi() {
  display.clearDisplay();
  display.setCursor(0,28);
  display.println(F("Connecting to WiFi"));
  display.display();
  delay(100);
  
  // init wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println(F("Connecting to WiFi"));
  display.println("");
  display.display(); //?
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    display.print(F("."));
    display.display();
  }
  Serial.print(F("Connected to the WiFi network: "));
  Serial.println(WIFI_SSID);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println(F("Connected to WiFi:"));
  display.println(WIFI_SSID);
  Serial.print(F("IP: "));
  Serial.println(WiFi.localIP());
  display.println(F("IP:"));
  display.println(WiFi.localIP());
  display.display();
  delay(3000);
}

unsigned long getEpochTime() { //! OFFSET IS APPLIED TO EPOCH TIME AS WELL
  // get epoch time from NTP
  unsigned long now = timeClient.getEpochTime();
  Serial.print(F("Epoch time: "));
  Serial.println(now);
  return now;
}

String statusTime() {
  // get formatted string time (human readable) from NTP
  String now = timeClient.getFormattedTime();
  Serial.print(F("Formatted time: "));
  Serial.println(now);
  return now;
}

int sendStatus(String time_human, String info) {
  // send data to api server
  //DynamicJsonDocument json_doc(200);
  //StaticJsonDocument<192> json_doc; // prefer static ? 196? fron ArduinoJson Assistant, or 200 ?

  // add data to json packet
  //json_doc["door_state"] = door;
  //json_doc["temperature"] = temp;
  //json_doc["humidity"] = hum;
  //json_doc["update_time"] = time_human;
  //json_doc["update_time_unix"] = epochTime;
  ////json_doc["presence"] = presence; // from PIR sensor ? nahh

  String jsonData;
  //serializeJson(json_doc, jsonData);

  // or just don't use ArduinoJson
  jsonData = "{\"door_state\":\"" + String(door) + "\",\"update_time\":\"" + String(time_human) + "\",\"update_time_unix\":\"" + String(epochTime) + "\",\"info\":\"" + String(info) + "\",\"temperature\":\"" + String(temp) + "\",\"humidity\":\"" + String(hum) + "\"}";
  Serial.println(jsonData);
  
  http.begin(wifi_client, API_SERVER);
  //http.begin(*secure_client, API_SERVER); //https TODO
  http.addHeader("Content-Type", "application/json");
  // apply API key as a header named "api_token"
  http.addHeader("api_token", API_KEY);
  Serial.println(F("Sending JSON data"));
  int httpResponseCode = http.PUT(jsonData); // send data and get response
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

void displayStatus(int result_api, String update_time) { // display has 6 lines for basic text println
  display.clearDisplay();
  display.setCursor(0,0);

  // door
  display.print("Door ");
  switch (door) {
    case 0:
      display.println(F("CLOSED"));
      break;
    case 1:
      display.println(F("OPEN"));
      break;
    case 2:
      display.println(F("UNKOWN"));
      break;
    default:
      display.println(F("?ERROR?"));
      break;
  }

  // temp and hum
  display.print(F("Temp: "));
  if (temp < 0) { // because display.print accepts 32-bit unsigned int
    display.print("-");
    display.print(int(-temp)); //? works?
  } else {
    display.print(int(temp));
  }
  display.print(F("°C, "));
  display.print(F("Hum: "));
  display.print(int(hum));
  display.println(F("%"));

  // wifi status
  display.print(F("WiFi "));
  switch ( WiFi.status() ) {
    case WL_CONNECTED:
      display.print(F("CONNECTED "));
      display.println(WiFi.localIP());
      break;
    default:
      display.println(F("!disconnected!"));
      break;
  }

  // last update time
  display.print(F("Last update: "));
  display.println(update_time);

  // api call result
  display.print(F("API call "));
  switch (result_api) {
    case 0:
      display.println(F("success"));
      break;
    case 1:
      display.println(F("FAILED"));
      break;
    default:
      display.println(F("UNKNOWN?"));
      break;
  }
  
  // show on screen
  display.display();
}

// print temp sensor error if one occurs
void printAhtStatus() {
  //? display.clearDisplay(); // needed ?
  switch ( aht20.getStatus() ) {
    case AHTXX_NO_ERROR:
      Serial.println(F("no error"));
      //display.clearDisplay();
      //display.setCursor(0,0);
      //display.println(F("no aht error"));
      //display.display();
      //delay(200);
      break;

    case AHTXX_BUSY_ERROR:
      Serial.println(F("sensor busy, increase polling time"));
      display.setCursor(0,0);
      display.println(F("sensor busy, increase polling time"));
      display.display();
      delay(500);
      break;

    case AHTXX_ACK_ERROR:
      Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      display.setCursor(0,0);
      display.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      display.display();
      delay(500);
      break;

    case AHTXX_DATA_ERROR:
      Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      display.setCursor(0,0);
      display.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      display.display();
      delay(500);
      break;

    case AHTXX_CRC8_ERROR:
      Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      display.setCursor(0,0);
      display.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      display.display();
      delay(500);
      break;

    default:
      Serial.println(F("unknown status"));    
      //display.setCursor(0,0);
      //display.println(F("unknown status"));
      //display.display();
      //delay(500);
      break;
  }
}

/* --- TEST --- */ // to wake up screen on PIR movement
//void sleepDisplay(Adafruit_SSD1305 *display) {
void sleepDisplay() {
  display.clearDisplay();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
}
//void wakeDisplay(Adafruit_SSD1305 *display) {
void wakeDisplay() {
  //? needs hard reset (to RST pin of display) before asking to turn on ?
  display.ssd1306_command(SSD1306_DISPLAYON);
  display.clearDisplay();
}

