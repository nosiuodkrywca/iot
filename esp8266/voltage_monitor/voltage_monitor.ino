#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>

#define sgn(x) ((x) < 0 ? -1 : ((X) > 0 ? 1 : 0))

WiFiManager wm;                 // create a WiFiManager object

ESP8266WebServer server(80);    // create a web server object that listens for HTTP request on port 80

// this is the integer offset on raw sensor data, eg. background noise
const int zeroOffset = -1;

// this is a static voltage offset value applied to the final result
const float voltOffset = -0.05;

// precision of the sensor, 1024 for 10-bit
const int precision = 1024;

// accepted maximum voltage of the board
const float inputRange = 3.2;

// resistance of the first resistor (Vin)
const int resI = 4700;

// resistance of the second resistor (GND)
const int resO = 330;

// voltage divider ratio
const float divRatio = (resI + resO) / resO;

// capacity of the battery bank (in Ah)
const int capacity = 320;

// voltage of a fully charged battery
const float fullyCharged = 12.8;

// voltage of an empty battery
const float totallyEmpty = 10.8;

// voltage drop of battery bank under 1c load (in %)
const int cdrop = 4;

/* SETUP */
void setup() {
  Serial.begin(9600);
  
  //wm.resetSettings();                     // uncomment to reset network settings
  wm.autoConnect();

  // port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // no authentication by default
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

/* LOOP */
void loop(void){
  ArduinoOTA.handle();
  server.handleClient();                    // listen for HTTP requests from clients
}

void handleRoot() {
  float voltage = measure();
  server.send(200, "text/plain", String(voltage));   // Send HTTP status 200 (Ok) and send some text to the browser/client
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

const int no_of_measurements = 10;
const int time_diff = 20;

float measure() {
  // for time delay between measurements
  unsigned long currT = 0;
  unsigned long prevT = 0;
  unsigned long diffT = 0;

  float res[no_of_measurements] = {};
  int sensorValue;
  float voltage;

  int i = 0;

  while(i < no_of_measurements) {

    currT = millis();
    diffT = currT - prevT;

    if (diffT > time_diff) {
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
  int n = sizeof(arr) / sizeof(arr[0]);
  std::sort(arr, arr + n);

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