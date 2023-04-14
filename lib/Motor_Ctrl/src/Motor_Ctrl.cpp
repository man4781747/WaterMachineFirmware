#include "Motor_Ctrl.h"

#include <vector>
#include <esp_system.h>

#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>


#define FREQUENCY 50;

TwoWire myWire(0);

void Single_Motor::ActiveMotor(int channelIndex_,String motorName_ = "") {
  channelIndex=channelIndex_;
  if (motorName == String("")) {
    uint64_t randomValue = ((uint64_t)esp_random()) << 32 | esp_random();
    char uuidString[32];
    sprintf(uuidString, "%016llx", randomValue);
    motorName = "Motor-"+String(uuidString);
  } else {
    motorName = motorName_;
  }
};


// void Motor_Ctrl::INIT()
// {
//   pwm = Adafruit_PWMServoDriver();
// }
Motor_Ctrl::Motor_Ctrl()
{
}

void Motor_Ctrl::INIT_Motors()
{
  myWire.begin(13, 14);
  pwm = Adafruit_PWMServoDriver(0x40, myWire);
  // pwm = Adafruit_PWMServoDriver();
  pwm.begin();
  pwm.setPWMFreq(50);
}

void Motor_Ctrl::AddNewMotor(int channelIndex_, String motorName_, String descrption)
{
  motorsArray[channelIndex_] = Single_Motor();
  motorsArray[channelIndex_].ActiveMotor(channelIndex_, motorName_);
}

void Motor_Ctrl::SetMotorTo(int channelIndex_, int angle)
{
  ESP_LOGI("Motor","Motor %02d to: %03d", channelIndex_, angle);
  int pulse_wide = map(angle, 0, 180, 600, 2400);
  int pulse_width = int((float)pulse_wide / 1000000.*50.*4096.);
  ESP_LOGI("Motor","pulse_width %d", pulse_width);
  pwm.setPWM(channelIndex_, 0, pulse_width);
  
}