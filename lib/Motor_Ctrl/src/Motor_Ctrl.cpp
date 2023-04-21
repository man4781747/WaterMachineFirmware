#include "Motor_Ctrl.h"

#include <vector>
#include <esp_system.h>

#include <ArduinoJson.h>
#include <Adafruit_PWMServoDriver.h>
#include <TimeLib.h>   

#define FREQUENCY 50;

TwoWire myWire(0);

/**
 * @brief 單一顆馬達初始化
 * 
 * @param channelIndex_ 
 * @param motorName_ 
 */
void Single_Motor::ActiveMotor(int channelIndex_, String motorID_, String motorName_, String descrption) {
  channelIndex=channelIndex_;
  motorID = motorID_;
  motorDescription = descrption;
  if (motorName_ == String("")) {
    uint64_t randomValue = ((uint64_t)esp_random()) << 32 | esp_random();
    char uuidString[32];
    sprintf(uuidString, "%016llx", randomValue);
    motorName = "Motor-"+String(uuidString);
  } else {
    motorName = motorName_;
  }
};


Motor_Ctrl::Motor_Ctrl()
{
}

/**
 * @brief 馬達們控制物件初始化
 * 
 */
void Motor_Ctrl::INIT_Motors()
{
  ESP_LOGI("Motor_Ctrl","INIT_Motors");
  myWire.begin(13, 14);
  pwm_1 = Adafruit_PWMServoDriver(0x40, myWire);
  pwm_1.begin();
  pwm_1.setPWMFreq(50);
  pwm_2 = Adafruit_PWMServoDriver(0x41, myWire);
  pwm_2.begin();
  pwm_2.setPWMFreq(50);
}

/**
 * @brief 新增馬達設定
 * 
 * @param channelIndex_ 
 * @param motorName_ 
 * @param descrption 
 */
void Motor_Ctrl::AddNewMotor(int channelIndex_, String motorID_, String motorName_, String descrption)
{
  ESP_LOGI("Motor_Ctrl","AddNewMotor: %02d, %s, %s", channelIndex_, motorName_.c_str(), descrption.c_str());
  motorsArray[channelIndex_] = Single_Motor();
  motorsArray[channelIndex_].ActiveMotor(channelIndex_, motorID_, motorName_, descrption);
}

/**
 * @brief 設定指定PWM馬達角度
 * 
 * @param channelIndex_ 
 * @param angle 
 */
void Motor_Ctrl::SetMotorTo(int channelIndex_, int angle)
{
  ESP_LOGI("Motor","Motor %02d set: %03d", channelIndex_, angle);
  motorsArray[channelIndex_].motorStatus = angle;
}

/**
 * @brief 馬達狀態正式變更
 * 
 * @param channelIndex_ 
 */
void Motor_Ctrl::MotorStatusChange(int channelIndex_)
{
  ESP_LOGI("Motor","Motor %02d change to: %03d", channelIndex_, motorsArray[channelIndex_].motorStatus);
  int pulse_wide = map(motorsArray[channelIndex_].motorStatus, 0, 180, 600, 2400);
  int pulse_width = int((float)pulse_wide / 1000000.*50.*4096.);
  if (channelIndex_/16 == 1) {
    pwm_2.setPWM(channelIndex_%16, 0, pulse_width);
  } else {
    pwm_1.setPWM(channelIndex_%16, 0, pulse_width);
  }
}


/**
 * @brief 單一顆蠕動馬達初始化
 * 
 * @param motorIndex_ 
 * @param motorName_ 
 */
void C_Single_Peristaltic_Motor::ActiveMotor(int motorIndex_, String motorID_, String motorName_, String descrption) {
  motorIndex=motorIndex_;
  motorID = motorID_;
  motorDescription = descrption;
  if (motorName_ == String("")) {
    uint64_t randomValue = ((uint64_t)esp_random()) << 32 | esp_random();
    char uuidString[32];
    sprintf(uuidString, "%016llx", randomValue);
    motorName = "Motor-"+String(uuidString);
  } else {
    motorName = motorName_;
  }
};


/**
 * @brief 蠕動馬達們控制物件初始化
 * 
 */
void C_Peristaltic_Motors_Ctrl::INIT_Motors()
{
  ESP_LOGI("Motor_Ctrl","INIT_Motors");
}

void C_Peristaltic_Motors_Ctrl::AddNewMotor(int channelIndex_, String motorID, String motorName_, String descrption)
{
  ESP_LOGI("Peristaltic_Motors_Ctrl","AddNewPeristalticMotor: %02d, %s, %s", channelIndex_, motorName_.c_str(), descrption.c_str());
  motorsArray[channelIndex_] = C_Single_Peristaltic_Motor();
  motorsArray[channelIndex_].ActiveMotor(channelIndex_, motorID, motorName_, descrption );
}

void C_Peristaltic_Motors_Ctrl::RunMotor(int channelIndex_, int type, int durationTime)
{
  ESP_LOGI("Peristaltic_Motors_Ctrl","Peristaltic motor %02d set: %d in %d second", channelIndex_, type, durationTime);
  motorsArray[channelIndex_].motorNextStatus = type;
  time_t nowTime = now();
  motorsArray[channelIndex_].motorEndTime = time_t(nowTime + durationTime);
}

