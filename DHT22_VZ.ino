/***********************************************************************/
/* Sketch to connect a DHT22 to volkszaehler.org middleware via http.  */
/* It is using DeepSleep to save power between the measurements.       */
/*                                                                     */
/* It's possible to to connect to two different AccessPoints.          */
/* Replace your AP_SSID and AP_PASSWORDS.                              */
/*                                                                     */
/* Install "DHT-sensor-library":                                       */
/* Sketch --> Include Library --> Manage Libraries... -->              */
/* filter for "dht" --> choose "DHT sensor libray" --> install         */
/*                                                                     */
/***********************************************************************/
 
#include <ESP8266WiFi.h>
#include <DHT.h>

extern "C" {
#include "user_interface.h"
}

// Debugging (set to false to disable serial Output)
const bool debugON = false;                                      

// AP1 definitions
#define AP1_SSID "yourssid1"
#define AP1_PASSWORD "yourpassword1"

// AP2 definitions
#define AP2_SSID "yourssid2"
#define AP2_PASSWORD "yourpassword2"

//VZ Server definitions
#define VZ_URL  "middleware"                                      // url in front of your "/volkszaehler.org/htdocs/frontend" without "http://"
#define VZ_PORT        80

// VZ Channel uuid definitions
#define uuid_h  "your-uuid-for-huidity"
#define uuid_t  "your-uuid-for-temperature"

DHT dht(2, DHT22);
long interval = 300;                                              // seconds between measurements
int DHTcounter = 0;

void setup() {
  dht.begin();
  Serial.begin(115200);
  wifiConnect();
}

void loop() {
  if (DHTcounter < 10){
    
    float h = dht.readHumidity();                                 // read DHT22 humidity and temperature
    float t = dht.readTemperature();                              // temperature in Celsius; for Fahrenheit use readTemperature(true)
  
    if (isnan(h) || isnan(t)) {                                   // if there is a problem with the results
      DHTcounter++;                                               // increment DHTcounter
      if (debugON) Serial.println("Error reading DHT");
      delay(500);                                                 // wait 500ms (DHT max. frequency is 2Hz) and
      return;                                                     // restart loop()
    }
  
    delay(2000);                                                  // wait 2s because DHT22 sends results of last measurement
  
    h = dht.readHumidity();                                       // read DHT22 humidity and temperature
    t = dht.readTemperature();                                    // temperature in Celsius; for Fahrenheit use readTemperature(true)

    if (debugON) Serial.println(t);                               // debug: print temperature
    if (debugON) Serial.println(h);                               // debug: print humidity
  
    if (isnan(h) || isnan(t)) {                                   // if there is a problem with the results
      DHTcounter++;                                               // increment DHTcounter
      if (debugON) Serial.println("Error reading DHT");
      delay(500);                                                 // wait 500ms (DHT max. frequency is 2Hz) and
      return;                                                     // restart loop()
    }
  
  sendVZ(uuid_h, h);                                              // send humidity to volkszaehler
  sendVZ(uuid_t, t);                                              // send temperature to volkszaehler
  }
  
  unsigned long currentMillis = millis();                         // read milliseconds since waking up
  system_deep_sleep(((interval * 1000) - currentMillis) * 1000);  // go to deep sleep for defined time
  delay(200);                                                     // give some time to activate sleep state
  abort();
}


/******************************************************/
/*  WIFI Connect function                             */
/*                                                    */ 
/*  if connection fails after ten attempts goto deep  */
/*  sleep and try again after the defined time        */
/******************************************************/

void wifiConnect()
{
  int AP1counter = 0;
  int AP2counter = 0;
  
  if (debugON) Serial.print("Connecting to AP1");                   // debug
  WiFi.begin(AP1_SSID, AP1_PASSWORD);                               // try to connect to AP1
  while (WiFi.status() != WL_CONNECTED && AP1counter < 10) {        // after ten unsuccessful attempts
    delay(1000);                                                     
    AP1counter++;
    if (debugON) Serial.print(".");                                 // debug
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();                                              // disconnect Wifi for second AP
 
    if (debugON) Serial.print("Connecting to AP2");                 // debug
    WiFi.begin(AP2_SSID, AP2_PASSWORD);                             // try to connect to AP2
    while (WiFi.status() != WL_CONNECTED && AP2counter < 10) {      // after ten unsuccessful attempts
      delay(1000);                                                     
      AP2counter++;
      if (debugON) Serial.print(".");                               // debug
    }
  }
  
  if (WiFi.status() != WL_CONNECTED){                               // goto deep sleep to try again later to save power
    unsigned long currentMillis = millis();                         // read milliseconds since waking up
    system_deep_sleep(((interval * 1000) - currentMillis) * 1000);  // go to deep sleep for defined time
    delay(200);                                                     // give some time to activate sleep state
    abort();
  }
    if (debugON) Serial.println("");                                // debug
    if (debugON) Serial.println("WiFi connected");                  // debug
}


/******************************************************/
/*  function to send the data to volkszaehler         */
/*  uuid and value are given as parameter             */
/******************************************************/

void sendVZ(String uuid, float value)
{
  WiFiClient client;

  while (!client.connect(VZ_URL, VZ_PORT)) {                        // try to connect to volkszaehler, 
    if (debugON) Serial.println("connection failed");               // if it fails
    wifiConnect();                                                  // connect to WIFI again
  }
  
  if (debugON) Serial.println("VZ connected");

  String url = "";
  url = "/volkszaehler/htdocs/middleware.php/data/" + uuid +        // build url to send to
        ".json?operation=add&value=" + value;  
  
  if (debugON) Serial.print("POST data to URL: ");                  // debug
  if (debugON) Serial.println(url);                                 // debug

  client.print(String("GET ") + url +                               // send data to volkzaehler
               " HTTP/1.1\r\n" +
               "Host: " + String(VZ_URL) + "\r\n" +  
               "Connection: close\r\n" +
               "Accept: */*\r\n" +                   
               "User-Agent: Mozilla/4.0 " +
               "(compatible; esp8266; " +
               "Windows NT 5.1)\r\n" +
               "\r\n");
  
  if (debugON) delay(500);
   
  while (client.available()) {
    String line = client.readStringUntil('\r');
    if (debugON) Serial.print(line);                                // debug: Print response
  }

  if (debugON) Serial.println();                                    // debug
  if (debugON) Serial.println("Connection closed");                 // debug
  client.stop();                                                    // stop http connection
}
