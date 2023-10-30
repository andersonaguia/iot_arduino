#include "ControllerData.h"

ControllerData::ControllerData() {}

void ControllerData::setName(String name) {
  this->name = name;
}

void ControllerData::setModel(String model) {
  this->model = model;
}

void ControllerData::setConnectionType(String connectionType) {
  this->connectionType = connectionType;
}

void ControllerData::setIpAddress(String ipAddress) {
  this->ipAddress = ipAddress;
}

void ControllerData::setMacAddress(String macAddress) {
  this->macAddress = macAddress;
}

void ControllerData::setLocation(String location) {
  this->location = location;
}

void ControllerData::setId(int id) {
  this->id = id;
}

String ControllerData::getName() {
  return name;
}

String ControllerData::getModel() {
  return model;
}

String ControllerData::getConnectionType() {
  return connectionType;
}

String ControllerData::getIpAddress() {
  return ipAddress;
}

String ControllerData::getMacAddress() {
  return macAddress;
}

String ControllerData::getLocation() {
  return location;
}

int ControllerData::getId() {
  return id;
}
