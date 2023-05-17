#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>
#include <unordered_map>

enum MotorSwitchStatus : int {
  LeftCenter = 0,
  LeftRight = 90,
  CenterRight = 180
};

class Single_Motor
{
  public:
    Single_Motor(void){};
    void ActiveMotor(int channelIndex_, String motorID="", String motorName="", String descrption="");
    int motorStatus = MotorSwitchStatus::LeftRight;
    int channelIndex = -1;
    String motorID = "";
    String motorName = "";
    String motorDescription = "";
};


enum MotorCtrlSteps {
  Idel,
  Active,
  Running
};

class Motor_Ctrl
{
  public:
    Motor_Ctrl(void);
    void INIT_Motors(TwoWire &Wire_);
    void AddNewMotor(int channelIndex_, String motorID="", String motorName="", String descrption="");
    
    void SetMotorTo(int channelIndex_, int angle);
    void SetMotorTo(String motorID, int angle);
    
    void MotorStatusChange(int channelIndex_);
    void MotorStatusChange(String motorID);

    u_int32_t motorsStatusCode = 0;
    int active = MotorCtrlSteps::Idel;
    std::unordered_map<std::string, Single_Motor> motorsDict;
    Adafruit_PWMServoDriver *pwm_1;
    Adafruit_PWMServoDriver *pwm_2;
  private:
};

enum PeristalticMotorStatus : int {
  FORWARD = 0b10,
  STOP = 0b00,
  REVERSR = 0b01,
};

class C_Peristaltic_Motors_Ctrl
{
  public:
    C_Peristaltic_Motors_Ctrl(void){};
    void INIT_Motors(int SCHP, int STHP, int DATA, int moduleNum);
    int SHCP, STCP, DATA, moduleNum;
    uint8_t *moduleDataList;
    void SetAllMotorStop();
    void RunMotor(uint8_t *moduleDataList);
    void SetMotorStatus(int index, PeristalticMotorStatus status);
    void ShowNowSetting();

  private:
};





#endif