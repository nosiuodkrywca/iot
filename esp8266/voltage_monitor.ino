#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>
#include <Wire.h>

#define sgn(x) ((x) < 0 ? -1 : ((X) > 0 ? 1 : 0))

WiFiManager wm;

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80

unsigned long currT = 0;
unsigned long prevT = 0;
unsigned long diffT = 0;

// this is the integer offset on raw sensor data, eg. background noise
int zeroOffset = -1;

// this is a static voltage offset value applied to the final result
float voltOffset = -0.05;

// precision of the sensor, 1024 for 10-bit
int precision = 1024;

// accepted maximum voltage of the board
float inputRange = 3.2;

// resistance of the first resistor (Vin)
int resI = 4700;

// resistance of the second resistor (GND)
int resO = 330;

// voltage divider ratio
float divRatio = (resI + resO) / resO;

// capacity of the battery bank (in Ah)
int capacity = 320;

// voltage of a fully charged battery
float fullyCharged = 12.8;

// voltage of an empty battery
float totallyEmpty = 10.8;

// voltage drop of battery bank under 1c load (in %)
int cdrop = 4;


void setup() {
  Serial.begin(9600);
  
  //wm.resetSettings();                     // uncomment to reset network settings
  wm.autoConnect();

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");


  ArduinoOTA.onStart([]() {
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA end");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("OTA auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("OTA begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("OTA connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("OTA receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("OTA end Failed");
  });
  ArduinoOTA.begin();


  server.on("/", handleRoot);               // Call the 'handleRoot' function when a client requests URI "/"
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  server.begin();                           // Actually start the server
  Serial.println("HTTP server started");

  server.enableCORS(true);

}



void loop(void){
  ArduinoOTA.handle();
  server.handleClient();                    // Listen for HTTP requests from clients
}

void handleRoot() {
  float voltage = measure();
  server.send(200, "text/plain", String(voltage));   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

float measure() {

  float res[10] = {};
  int sensorValue;
  float voltage;

  int i = 0;

  while(i < 10) {

    currT = millis();
    diffT = currT - prevT;

    if (diffT>20) {
      Serial.print("measurement no #");
      Serial.print(i);
      Serial.print(": ");
      
      sensorValue = analogRead(A0);
      voltage = (sensorValue + zeroOffset) * (1.0/precision) * divRatio * inputRange + voltOffset;
      
      res[i] = voltage;
      Serial.print(voltage);
      Serial.println(" V");

      prevT = currT;      
      i += 1;
    }


  }

  return mode(res);
  
}

float mode(float arr[]) {

  //int arr[] = { 1, 5, 8, 9, 6, 7, 3, 4, 2, 0 };
    //int n = sizeof(arr) / sizeof(arr[0]);
    int n = 10;
    /*Here we take two parameters, the beginning of the
    array and the length n upto which we want the array to
    be sorted*/
    std::sort(arr, arr + n);

  //int[10] sortedArray; // this array must be sorted!!

  int modeCt = 0;
  float modeV = -1;

  int ct = 0;
  float v = -1.;

  for(int i = 0; i< sizeof(arr); i++) {
    if(arr[i] != v) {
      v = arr[i];
      ct = 0;
    }

    ct ++;

    if(ct > modeCt) {
      modeCt = ct;
      modeV = v;
    }
  }
  Serial.print("mode: ");
  Serial.println(modeV);
  return modeV;
}