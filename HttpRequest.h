#ifndef HttpRequest_h
#define HttpRequest_h

#include <Arduino.h>

String getRequest(String url);
String postRequest(String url, String jsonPayload);

#endif