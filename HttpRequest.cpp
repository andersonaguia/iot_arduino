#include "HttpRequest.h"
#include <HTTPClient.h>
#include <Arduino.h>

String getRequest(String url) {
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("CODE: ");
  Serial.println(httpCode);

  String payload = http.getString();

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Resposta da API:");
  } else {
    Serial.print("Erro na solicitação. Código de status: ");
    Serial.println(httpCode);
  }

  http.end();

  return payload;
}

String postRequest(String url, String jsonPayload) {
  HTTPClient http;

  http.begin(url);

  http.addHeader("Content-Type", "application/json");

  Serial.println(jsonPayload);
  int httpCode = http.POST(jsonPayload);
  Serial.print("URL: ");
  Serial.println(url);
  Serial.print("CODE: ");
  Serial.println(httpCode);

  String payload = http.getString();

  if (httpCode == HTTP_CODE_CREATED) {    
    Serial.println("Resposta da API:");
  } else {
    Serial.print("Erro na solicitação. Código de status: ");
    Serial.println(httpCode);
  }

  http.end();

  return payload;
}