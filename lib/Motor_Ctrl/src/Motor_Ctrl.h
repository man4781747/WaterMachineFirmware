#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include <Arduino.h>
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
    void INIT_Motors();
    void AddNewMotor(int channelIndex_, String motorID="", String motorName="", String descrption="");
    
    void SetMotorTo(int channelIndex_, int angle);
    void SetMotorTo(String motorID, int angle);
    
    void MotorStatusChange(int channelIndex_);
    void MotorStatusChange(String motorID);

    u_int32_t motorsStatusCode = 0;
    int active = MotorCtrlSteps::Idel;
    Single_Motor motorsArray[64];

    std::unordered_map<std::string, Single_Motor> motorsDict;

  private:
    Adafruit_PWMServoDriver pwm_1;
    Adafruit_PWMServoDriver pwm_2;
};

enum PeristalticMotorStatus : int {
  FORWARD = 1,
  STOP = 0,
  REVERSR = -1
};

class C_Single_Peristaltic_Motor
{
  public:
    C_Single_Peristaltic_Motor(void){};
    void ActiveMotor(int motorIndex_, String motorID="", String motorName="", String descrption="");
    int motorNowStatus = PeristalticMotorStatus::STOP;
    int motorNextStatus = PeristalticMotorStatus::STOP;
    int motorIndex = -1;
    String motorID = "";
    String motorName = "";
    String motorDescription = "";
    long motorEndTime = -1;
};

class C_Peristaltic_Motors_Ctrl
{
  public:
    C_Peristaltic_Motors_Ctrl(void){};
    void INIT_Motors();
    void AddNewMotor(int channelIndex_, String motorID_="", String motorName_="", String descrption="");
    void RunMotor(int channelIndex_, int type, int durationTime);
    C_Single_Peristaltic_Motor motorsArray[32];
  private:
};





#endif