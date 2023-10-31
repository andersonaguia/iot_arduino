#include "HttpRequest.h"
#include <HTTPClient.h>
#include <Arduino.h>

String getRequest(String url) {
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  String payload = http.getString();

  http.end();

  return payload;
}

String postRequest(String url, String jsonPayload) {
  HTTPClient http;

  http.begin(url);

  http.addHeader("Content-Type", "application/json");

  Serial.println(jsonPayload);
  int httpCode = http.POST(jsonPayload);

  String payload = http.getString();

  http.end();

  return payload;
}