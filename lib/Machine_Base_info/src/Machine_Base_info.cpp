#include "Machine_Base_info.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>


DynamicJsonDocument Machine_Info::GetDeviceInfo()
{
  DynamicJsonDocument json_doc(10000);
  JsonVariant json_obj = json_doc.to<JsonVariant>();
  json_doc["device_no"].set(device_no);
  json_doc["FIRMWARE_VERSION"].set(FIRMWARE_VERSION);
  json_doc["mode"].set(mode);
  return json_doc;
};

String Machine_Info::GetDeviceInfoString()
{
  void* json_output = malloc(10000);
  DynamicJsonDocument json_doc = GetDeviceInfo();
  serializeJsonPretty(json_doc, json_output, 10000);
  String returnString = String((char*)json_output);
  free(json_output);
  json_doc.clear();
  return returnString;
};

