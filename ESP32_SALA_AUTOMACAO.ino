#include <WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_err.h>
#include <esp_log.h>
#include "HttpRequest.h"

esp_adc_cal_characteristics_t adc_cal;  //Structure that contains information for calibration

bool esp32 = true;  //Change to false for Arduino

int controllerId = 0;
int thermistorId = 0;
int count = 0;
int timeToReadAgain = 60000;  //Time for read data in miliseconds
int attempts = 10;            //Number of attempts to connect to the server
float lastTemperature = 0.0;

int thermistorPin;

double adcMax, Vs;

double R1 = 5120.0;    // Voltage divider resistor 4K7
double Beta = 3100.0;  //Beta value for NTC 10K Full Gauge SB41
double To = 298.15;    // Initial temperature in Kelvin (25°C)
double Ro = 10000.0;   // NTC resistance

const char *ssid = "your_wireless_ssid";
const char *password = "your_wireless_password";
IPAddress ip(172, 31, 210, 183);
IPAddress gateway(172, 31, 210, 1);
IPAddress subnet(255, 255, 255, 0);
String macAddress = "";

String urlBase = "http://172.31.210.181:3001";//Host address and port

const size_t bufferSize = 1024;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println("Initializing...");

  thermistorPin = 34;
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);                                                              //pin 34 esp32 WROOM
  esp_adc_cal_value_t adc_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, &adc_cal);  //Start calibration
  adcMax = 4095.0;                                                                                                        // ADC 12-bit (0-4095)
  Vs = 3.3;                                                                                                               // Voltage level

  delay(5000);

  Serial.println("Connecting to the network " + String(ssid) + "...");

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  WiFi.config(ip, gateway, subnet);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // ArduinoOTA.setPort(3232);

  ArduinoOTA.setHostname("ESP32 - AUTOMACAO");

  ArduinoOTA.setPassword("admin");

  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart(startOTA);

  ArduinoOTA.onEnd(endOTA);

  ArduinoOTA.onProgress(progressOTA);

  ArduinoOTA.onError(errorOTA);

  ArduinoOTA.begin();

  Serial.println("Connected!");
  delay(1000);
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  delay(1000);

  macAddress = WiFi.macAddress();

  Serial.print("MAC Address: ");
  Serial.println(macAddress);

  delay(1000);

  Serial.println("Getting data from the controller...");

  getControllerData();

  Serial.println("Ready!");
  delay(2000);
  getTemperature(thermistorPin);
}

void loop() {
  ArduinoOTA.handle();

  Serial.println("Obtendo dados do sensor...");

  float actualTemperature = getTemperature(thermistorPin);

  printTemperature(actualTemperature);

  if (actualTemperature != lastTemperature) {
    lastTemperature = actualTemperature;
    saveThermistorData(thermistorId, actualTemperature);
  }

  delay(timeToReadAgain);
}

void saveThermistorData(int thermistorId, float t) {
  String path = "/thermistordata/add";

  StaticJsonDocument<200> doc;
  doc["thermistorId"] = thermistorId;
  doc["value"] = t;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  String response = postRequest(urlBase + path, jsonPayload);

  int statusCode = extractField<int>(response, "statusCode", true);

  if (statusCode == 201) {
    count = 0;
  } else {
    count++;
    restartSystem();
  }
}

void getControllerData() {
  bool tryAgain = true;
  while (tryAgain) {
    if (controllerId <= 0) {
      Serial.println("Trying to get the controller id...");
      getControllerId();
    } else {
      Serial.println("Trying to get the thermistor id...");
      getControllerDevices();
      if (thermistorId > 0) {
        tryAgain = false;
      }
    }
    delay(5000);
  }
}

void getControllerId() {
  String url = urlBase;
  url += "/controllers/findbyip?ipAddress=";
  url += myIpToString();
  String response = getRequest(url);

  int statusCode = extractField<int>(response, "statusCode", true);

  Serial.print("Status Code: ");
  Serial.println(statusCode);
  if (statusCode == 200) {
    controllerId = extractField<int>(response, "id", false);
    count = 0;
  } else {
    count++;
    restartSystem();
  }
}

void getControllerDevices() {
  String url = "";
  url += urlBase;
  url += "/thermistors/findbycontrollerport/?controllerId=" + String(controllerId) + "&controllerPort=" + String(thermistorPin);

  String response = getRequest(url);

  int statusCode = extractField<int>(response, "statusCode", true);

  if (statusCode == 200) {
    thermistorId = extractField<int>(response, "id", false);
    count = 0;
  } else {
    count++;
    restartSystem();
  }
}

String myIpToString() {
  String stringIP = "";

  for (int i = 0; i < 4; i++) {
    if (i < 3) {
      stringIP += String(ip[i]);
      stringIP += ".";
    } else {
      stringIP += String(ip[i]);
    }
  }

  return stringIP;
}

void restartSystem() {
  if (count >= attempts) {
    Serial.println("Unable to communicate with the server");
    delay(2000);
    Serial.print("Rebooting");
    for (int i = 0; i <= 2; i++) {
      delay(1000);
      Serial.print(".");
    }
    ESP.restart();
  }
}

template<typename T>
T extractField(const String &jsonString, const String &fieldName, bool statusCode) {
  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("Erro ao fazer o parsing do JSON: ");
    Serial.println(error.c_str());
    return T(-1);
  }

  if (statusCode) {
    return doc["body"][fieldName].as<T>();
  } else {
    if (doc.containsKey("body") && doc["body"].containsKey("data") && doc["body"]["data"].containsKey(fieldName)) {
      return doc["body"]["data"][fieldName].as<T>();
    }
  }
  return T(-1);
}

void printTemperature(float t) {
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.println(" °C");
}

float getTemperature(int thermistorPin) {
  uint32_t AD = 0;
  for (int i = 0; i < 100; i++) {
    AD += adc1_get_raw(ADC1_CHANNEL_6);  //Get RAW value from ADC
    ets_delay_us(30);
  }
  AD /= 100;

  double Vout, Rt = 0;
  double T, Tc, Tf = 0;

  double adc = 0;

  adc = analogRead(thermistorPin);
  adc = AD;

  Vout = adc * Vs / adcMax;
  Rt = R1 * Vout / (Vs - Vout);

  T = 1 / (1 / To + log(Rt / Ro) / Beta);  // Kelvin
  Tc = T - 273.15;                         // Celsius
  Tf = Tc * 9 / 5 + 32;                    // Fahrenheit

  return Tc;
}

void startOTA() {
  String type;

  if (ArduinoOTA.getCommand() == U_FLASH)
    type = "flash";
  else
    type = "filesystem";  // U_SPIFFS

  Serial.println("Start updating " + type);
}


void endOTA() {
  Serial.println("\nEnd");
}

void progressOTA(unsigned int progress, unsigned int total) {
  Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
}

void errorOTA(ota_error_t error) {
  Serial.printf("Error[%u]: ", error);

  if (error == OTA_AUTH_ERROR)
    Serial.println("Auth Failed");
  else if (error == OTA_BEGIN_ERROR)
    Serial.println("Begin Failed");
  else if (error == OTA_CONNECT_ERROR)
    Serial.println("Connect Failed");
  else if (error == OTA_RECEIVE_ERROR)
    Serial.println("Receive Failed");
  else if (error == OTA_END_ERROR)
    Serial.println("End Failed");
}
