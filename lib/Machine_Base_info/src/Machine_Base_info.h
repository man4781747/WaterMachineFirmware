#ifndef MACHINE_INFO_H
#define MACHINE_INFO_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

struct MachineInfo_t {
  DynamicJsonDocument* time_interval;
  String device_no = "";
  String FIRMWARE_VERSION = "2.0.0";
  String mode = "Mode_Slave";
};

class Machine_Info
{
  public:
    MachineInfo_t MachineInfo;
    Machine_Info(void){
      MachineInfo.time_interval = new DynamicJsonDocument(1024);
    };
    
    DynamicJsonDocument GetDeviceInfo();
    String GetDeviceInfoString();
    
    void LoadInfoByJSONItem(DynamicJsonDocument& inputJSON){
      JsonObject time_intervalData = inputJSON["time_interval"].as<JsonObject>();
      String timeIntervalDataTemp;
      serializeJsonPretty(time_intervalData, timeIntervalDataTemp);
      DeserializationError error = deserializeJson(*MachineInfo.time_interval, timeIntervalDataTemp);
      if (error) {
        ESP_LOGE("Machine_Base_info", "deserializeJson() failed: %s", error.f_str());
      }
      String device_no_ = inputJSON["device_no"];
      MachineInfo.device_no = device_no_;
      // String FIRMWARE_VERSION_ = inputJSON["FIRMWARE_VERSION"];
      MachineInfo.FIRMWARE_VERSION = "V2.0.0";
    };

    void updateTimeInterval(String content);

  private:
};

class Motor_Info
{
  public:
    Motor_Info(void){};

  private:
};


#endif