#ifndef MACHINE_INFO_H
#define MACHINE_INFO_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>


class Machine_Info
{
  public:
    Machine_Info(void){};

    String device_no = "";
    String FIRMWARE_VERSION = "2.0.0";
    String mode = "Mode_Slave";
    
    DynamicJsonDocument GetDeviceInfo();
    String GetDeviceInfoString();
    
    void LoadInfoByJSONItem(StaticJsonDocument<200>& inputJSON){
      String device_no_ = inputJSON["device_no"];
      String FIRMWARE_VERSION_ = inputJSON["FIRMWARE_VERSION"];
      device_no = device_no_;
      FIRMWARE_VERSION = FIRMWARE_VERSION_;
    };
  private:
};

class Motor_Info
{
  public:
    Motor_Info(void){};

  private:
};


#endif