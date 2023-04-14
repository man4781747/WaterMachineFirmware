#include "Machine_Base_info.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>



StaticJsonDocument<1024> Machine_Info::GetDeviceInfo()
{
  StaticJsonDocument<1024> json_doc;
  JsonVariant json_obj = json_doc.to<JsonVariant>();
  json_doc["device_no"].set(device_no);
  json_doc["FIRMWARE_VERSION"].set(FIRMWARE_VERSION);
  return json_doc;
};

