#include "Machine_Base_info.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <sstream>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <regex>


DynamicJsonDocument Machine_Info::GetDeviceInfo()
{
  DynamicJsonDocument json_doc(10000);
  json_doc["device_no"].set(MachineInfo.device_no);
  json_doc["FIRMWARE_VERSION"].set(MachineInfo.FIRMWARE_VERSION);
  json_doc["mode"].set(MachineInfo.mode);
  json_doc["time_interval"].set(*MachineInfo.time_interval);
  return json_doc;
};

String Machine_Info::GetDeviceInfoString()
{
  String returnString;
  DynamicJsonDocument json_doc = GetDeviceInfo();
  serializeJsonPretty(json_doc, returnString);
  json_doc.clear();
  return returnString;
};


void Machine_Info::updateTimeInterval(String NewTimeIntervals)
{
  std::string str(NewTimeIntervals.c_str());
  std::regex pattern("\\d\\d:\\d\\d");
  std::smatch matches;
  while (std::regex_search(str, matches, pattern)) {
    for (auto match : matches) {
      

    }
    str = matches.suffix().str();
  }
}
