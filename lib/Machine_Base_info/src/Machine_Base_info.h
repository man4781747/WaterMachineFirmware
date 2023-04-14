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

    String GetInfoJSONString(){
      StaticJsonDocument<200> doc;
      doc["device_no"] = device_no;
      doc["FIRMWARE_VERSION"] = FIRMWARE_VERSION;
      String output;
      serializeJsonPretty(doc, output);
      return output;
    };

    StaticJsonDocument<1024> GetDeviceInfo();


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
    Servo servo_m1;
    Servo servo_m2;

  private:
};


#endif