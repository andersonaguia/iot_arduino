#include <WiFi.h>
//#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
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

#include "ControllerData.h"

ControllerData controller;

esp_adc_cal_characteristics_t adc_cal;  //Structure that contains information for calibration

bool esp32 = true;  //Change to false for Arduino

int count = 0;
int secondsToSend = 60;

int thermistorPin;
int thermistorId;
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

String urlBase = "http://172.31.210.181:3001";

const size_t bufferSize = 1024;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1000);
  }

  Serial.println("Initializing...");

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

  ArduinoOTA.setPassword("(123456)");

  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart(startOTA);

  ArduinoOTA.onEnd(endOTA);

  ArduinoOTA.onProgress(progressOTA);

  ArduinoOTA.onError(errorOTA);

  ArduinoOTA.begin();

  Serial.println("Ready!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  macAddress = WiFi.macAddress();

  controller.setIpAddress(myIpToString());
  controller.setMacAddress(macAddress);

  Serial.print("Endereço MAC: ");
  Serial.println(macAddress);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);                                                              //pin 34 esp32 WROOM
  esp_adc_cal_value_t adc_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_0, ADC_WIDTH_BIT_12, 1100, &adc_cal);  //Start calibration
  thermistorPin = 34;
  adcMax = 4095.0;  // ADC 12-bit (0-4095)
  Vs = 3.3;         // Voltage level

  delay(2000);

  int count = 0;
  bool tryAgain = true;

  Serial.println("Obtendo dados do controlador...");
  /*
  while (controller.getId() <= 0 && tryAgain) {
    Serial.println("Dados não encontrados. Realizando cadastro do controlador...");
    if (count > 10 && controller.getId() <= 0) {
      count = 0;

      StaticJsonDocument<200> doc;
      doc["name"] = controller.getName();
      doc["model"] = controller.getModel();
      doc["connectionType"] = controller.getConnectionType();
      doc["ipAddress"] = controller.getIpAddress();
      doc["macAddress"] = controller.getMacAddress();
      doc["location"] = controller.getLocation();

      String jsonPayload;
      serializeJson(doc, jsonPayload);

      String path = "/controllers/create";
      postData(urlBase + path, jsonPayload);
    }

    Serial.println(controller.getId());
    delay(2000);
    getDeviceId();
    count++;
  }*/

  while (true) {
    String url = urlBase;
    url += "/controllers/findbyip?ipAddress=";
    url += myIpToString();
    String response = getRequest(url);

    Serial.println(response);
    controller.setId(extractField<int>(response, "id"));
    controller.setName(extractField<String>(response, "name"));
    controller.setModel(extractField<String>(response, "model"));
    controller.setConnectionType(extractField<String>(response, "connectionType"));

    Serial.print("Controller ID: ");
    Serial.println(controller.getId());
    Serial.print("Nome: ");
    Serial.println(controller.getName());
    Serial.print("Modelo: ");
    Serial.println(controller.getModel());
    Serial.print("Conexão: ");
    Serial.println(controller.getConnectionType());
    delay(10000);

    url = "";
    url += urlBase;
    url += "/thermistors/findbycontrollerport/?controllerId=" + String(controller.getId()) + "&controllerPort=" + String(thermistorPin);

    response = "";
    response = getRequest(url);
    thermistorId = extractField<int>(response, "id");

    Serial.print("Thermistor ID: ");
    Serial.println(thermistorId);
    delay(10000);
  }
}

void loop() {
  float temperature = getTemperature();

  Serial.println(controller.getId());

  Serial.println("Obtendo dados do sensor...");

  ArduinoOTA.handle();
  /*
  if (count == secondsToSend) {
    temperature = getTemperature();
    printTemperature(temperature);
    postSensorValue(temperature, controller.getId());
    count = 0;
  }
*/
  count++;

  delay(1000);
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

template<typename T>
T extractField(const String &jsonString, const String &fieldName) {
  DynamicJsonDocument doc(1024);

  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("Erro ao fazer o parsing do JSON: ");
    Serial.println(error.c_str());
    return T(-1);
  }

  if (doc.containsKey("body") && doc["body"].containsKey("data") && doc["body"]["data"].containsKey(fieldName)) {
    return doc["body"]["data"][fieldName].as<T>();
  }

  return T(-1);
}



/*
void getDeviceId() {
  String path = "/controllers/findbyip?ipAddress=";
  getRequest(urlBase + path + controller.getIpAddress());
}

void postSensorValue(float temperature, int deviceId) {
  String path = "/thermistors/addvalue";
  postRequest(urlBase + path, temperature, deviceId);
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

void printTemperature(float t) {
  Serial.print("Temperatura: ");
  Serial.print(t);
  Serial.println(" °C");
}
*/

int extractKeyId(const String &jsonString) {
  DynamicJsonDocument doc(1024);  // Tamanho do buffer JSON

  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("Erro ao fazer o parsing do JSON: ");
    Serial.println(error.c_str());
    return -1;  // Retorna -1 em caso de erro
  }

  int id = doc["body"]["data"]["id"];

  return id;
}

float getTemperature() {
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
