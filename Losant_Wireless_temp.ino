/**
 * Example for sending temperature and humidity
 * hardware 
 * https://store.ncd.io/product/industrial-long-range-wireless-temperature-humidity-sensor/
 * https://store.ncd.io/product/esp32-iot-wifi-ble-module-with-integrated-usb/
 * https://store.ncd.io/product/i2c-shield-for-particle-electron-or-particle-photon-with-outward-facing-5v-i2c-port/
 */
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Losant.h>
#include <Wire.h>
#include <HardwareSerial.h>
HardwareSerial Serial1(1); // use uart2

uint8_t data[29];
int i;
float cTemp;
float fTemp;
const char* ssid = "XXX";
const char* password = "XXX";
const char* LOSANT_DEVICE_ID = "XXX";
const char* LOSANT_ACCESS_KEY = "XXX";
const char* LOSANT_ACCESS_SECRET = "XXX";
WiFiClientSecure wifiClient;

LosantDevice device(LOSANT_DEVICE_ID);

void connect() {

  // Connect to Wifi.
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  delay(500); 
  unsigned long wifiConnectStart = millis();

  while (WiFi.status() != WL_CONNECTED) {
    // Check to see if
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Failed to connect to WIFI. Please verify credentials: ");
      Serial.println();
      Serial.print("SSID: ");
      Serial.println(ssid);
      Serial.print("Password: ");
      Serial.println(password);
      Serial.println();
    }

    delay(500);
    Serial.println("...");
    // Only try for 5 seconds.
    if(millis() - wifiConnectStart > 5000) {
      Serial.println("Failed to connect to WiFi");
      Serial.println("Please attempt to send updated configuration parameters.");
      return;
    }
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.print("Authenticating Device...");
  HTTPClient http;
  http.begin("http://api.losant.com/auth/device");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceId"] = LOSANT_DEVICE_ID;
  root["key"] = LOSANT_ACCESS_KEY;
  root["secret"] = LOSANT_ACCESS_SECRET;
  String buffer;
  root.printTo(buffer);

  int httpCode = http.POST(buffer);

  if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
          Serial.println("This device is authorized!");
      } else {
        Serial.println("Failed to authorize device to Losant.");
        if(httpCode == 400) {
          Serial.println("Validation error: The device ID, access key, or access secret is not in the proper format.");
        } else if(httpCode == 401) {
          Serial.println("Invalid credentials to Losant: Please double-check the device ID, access key, and access secret.");
        } else {
           Serial.println("Unknown response from API");
        }
      }
    } else {
        Serial.println("Failed to connect to Losant API.");

   }

  http.end();

  // Connect to Losant.
  Serial.println();
  Serial.print("Connecting to Losant...");

  device.connectSecure(wifiClient, LOSANT_ACCESS_KEY, LOSANT_ACCESS_SECRET);

  while(!device.connected()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("Connected!");
  Serial.println();
  Serial.println("This device is now ready for use!");
}

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, 16, 17); // pins 16 rx2, 17 tx2, 19200 bps, 8 bits no parity 1 stop bit
  Serial.setTimeout(2000);
  connect();
}
void report(double humidity, double temp, double tempF, double battery) {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["humidity"] = humidity_sht25;
  root["temp"] = temp;//cTemp;
  root["tempF"] = tempF;
  root["battery"] = battery;
  device.sendState(root);
  Serial.println("   Reported!");
}

int timeSinceLastRead = 0;
void loop() {
   bool toReconnect = false;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Disconnected from WiFi");
    toReconnect = true;
  }

  if (!device.connected()) {
    Serial.println("Disconnected from MQTT");
    Serial.println(device.mqttClient.state());
    toReconnect = true;
  }

  if (toReconnect) {
    connect();
  }

  device.loop();

//////////////Wireless///////////////
if (Serial1.available())
  {
    data[0] = Serial1.read();
  //  delay(k);
   if(data[0]==0x7E)
    {
    while (!Serial1.available());
    for ( i = 1; i< 29; i++)
      {
      data[i] = Serial1.read();
      delay(1);
      }
    if(data[15]==0x7F)  /////// to check if the recive data is correct
      {
  if(data[22]==1)  //////// make sure the sensor type is correct
         {  
  humidity_sht25 = ((((data[24]) * 256) + data[25]) /100.0);
  int16_t cTempint = (((uint16_t)(data[26])<<8)| data[27]);
   cTemp = (float)cTempint /100.0;
   fTemp = cTemp * 1.8 + 32;
  float battery = ((data[18] * 256) + data[19]);
   battery = 0.00322 * battery; /////////// battery voltage
   Serial.print("Humidity % ");
   Serial.print (humidity_sht25);
   Serial.print("   Temperature in C ");
   Serial.print( cTemp);
   Serial.print("   Battery Voltage ");
   Serial.print(battery);
  report(humidity_sht25,cTemp,fTemp,battery);
  
         }
      }
    }
  }
}
