#ifndef ControllerData_h
#define ControllerData_h

#include <Arduino.h>

class ControllerData {
public:
  ControllerData();

  void setName(String name);
  void setModel(String model);
  void setConnectionType(String connectionType);
  void setIpAddress(String ipAddress);
  void setMacAddress(String macAddress);
  void setLocation(String location);
  void setId(int id);

  String getName();
  String getModel();
  String getConnectionType();
  String getIpAddress();
  String getMacAddress();
  String getLocation();
  int getId();

private:
  String name;
  String model;
  String connectionType;
  String ipAddress;
  String macAddress;
  String location;
  int id;
};

#endif
