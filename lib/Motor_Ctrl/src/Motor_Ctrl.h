#ifndef MOTOR_CTRL_H
#define MOTOR_CTRL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>

class Single_Motor
{
  public:
    Single_Motor(void){};
    void ActiveMotor(int channelIndex_, String motorName);

    int channelIndex = -1;
    String motorName = "";
    String motorDescription = "";
};


class Motor_Ctrl
{
  public:
    Motor_Ctrl(void);
    void INIT_Motors();
    void AddNewMotor(int channelIndex_, String motorName="", String descrption="");
    void SetMotorTo(int channelIndex_, int angle);
    u_int32_t motorsStatusCode = 0;
    Single_Motor motorsArray[16];
  private:
    Adafruit_PWMServoDriver pwm;
};


#endif