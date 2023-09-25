#include "Motor_Ctrl.h"

#include <vector>
#include <unordered_map>
#include <esp_system.h>

#include <ArduinoJson.h>
#include <Wire.h>
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
void Motor_Ctrl::INIT_Motors(TwoWire &Wire_)
{
  ESP_LOGI("Motor_Ctrl","INIT_Motors");
  pwm_1 = new Adafruit_PWMServoDriver(0x40, Wire_);
  pwm_1->begin();
  pwm_1->setPWMFreq(50);
  pwm_2 = new Adafruit_PWMServoDriver(0x41, Wire_);
  pwm_2->begin();
  pwm_2->setPWMFreq(50);
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
  motorsDict[std::string(motorID_.c_str())] = Single_Motor();
  motorsDict[std::string(motorID_.c_str())].ActiveMotor(channelIndex_, motorID_, motorName_, descrption);
}

void Motor_Ctrl::SetMotorTo(int channelIndex_, int angle)
{
  // ESP_LOGI("Motor","Motor %02d change to: %03d", channelIndex_, angle);
  int pulse_wide = map(angle, 0, 180, 500, 2500);
  int pulse_width = int((float)pulse_wide / 1000000.*50.*4096.);
  if (channelIndex_/16 == 1) {
    pwm_2->setPWM(channelIndex_%16, 0, pulse_width);
    pwm_2->setPWM(channelIndex_%16, 0, pulse_width);
  } else {
    pwm_1->setPWM(channelIndex_%16, 0, pulse_width);
    pwm_1->setPWM(channelIndex_%16, 0, pulse_width);
  }
}

/**
 * @brief 設定指定PWM馬達角度
 * 
 * @param motorID 
 * @param angle 
 */
void Motor_Ctrl::SetMotorTo(String motorID, int angle)
{
  // int channelIndex_
  // ESP_LOGI("Motor","Motor %02d change to: %03d", channelIndex_, angle);
  // int pulse_wide = map(angle, 0, 180, 600, 2400);
  // int pulse_width = int((float)pulse_wide / 1000000.*50.*4096.);
  // if (channelIndex_/16 == 1) {
  //   pwm_2->setPWM(channelIndex_%16, 0, pulse_width);
  // } else {
  //   pwm_1->setPWM(channelIndex_%16, 0, pulse_width);
  // }

  // ESP_LOGI("Motor","Motor %s set: %03d", motorID.c_str(), angle);
  
  // if (motorsDict.count(std::string(motorID.c_str()))) {
  //   motorsDict[std::string(motorID.c_str())].motorStatus = angle;
  // } else {
  //   ESP_LOGE("Motor","Can't find motor: %s setting", motorID.c_str());
  // }
}

void Motor_Ctrl::ResetPCA9685()
{
  pwm_1->reset();
  pwm_1->setPWMFreq(50);
  pwm_2->reset();
  pwm_2->setPWMFreq(50);
}

/**
 * @brief 馬達狀態正式變更
 * 
 * @param motorID 
 */
void Motor_Ctrl::MotorStatusChange(String motorID)
{
  int channelIndex_ = motorsDict[std::string(motorID.c_str())].channelIndex;
  ESP_LOGI("Motor","Motor %s change to: %03d", motorID.c_str(), motorsDict[std::string(motorID.c_str())].motorStatus);
  int pulse_wide = map(motorsDict[std::string(motorID.c_str())].motorStatus, 0, 180, 500, 2500);
  int pulse_width = int((float)pulse_wide / 1000000.*50.*4096.);
  if (channelIndex_/16 == 1) {
    pwm_2->setPWM(channelIndex_%16, 0, pulse_width);
  } else {
    pwm_1->setPWM(channelIndex_%16, 0, pulse_width);
  }
}


SemaphoreHandle_t Peristaltic_Motors_xMutex = xSemaphoreCreateMutex();
/**
 * @brief 蠕動馬達們控制物件初始化
 * 
 */
void C_Peristaltic_Motors_Ctrl::INIT_Motors(int SHCP_, int STCP_, int DATA_, int moduleNum_)
{
  ESP_LOGI("Motor_Ctrl","INIT_Motors");
  SHCP = SHCP_;
  STCP = STCP_;
  DATA = DATA_;
  moduleNum = moduleNum_;
  pinMode(SHCP, OUTPUT);
  pinMode(STCP, OUTPUT);  
  pinMode(DATA, OUTPUT);
  moduleDataList = new uint8_t[moduleNum];
  if (xSemaphoreTake(Peristaltic_Motors_xMutex, portMAX_DELAY) == pdTRUE) {
    memset(moduleDataList, 0, sizeof(moduleDataList));
    xSemaphoreGive(Peristaltic_Motors_xMutex);
  }
  RunMotor(moduleDataList);
}

void C_Peristaltic_Motors_Ctrl::RunMotor(uint8_t *moduleDataList)
{
  if (xSemaphoreTake(Peristaltic_Motors_xMutex, portMAX_DELAY) == pdTRUE) {
    digitalWrite(STCP, LOW);
    vTaskDelay(5);
    String settingContent = "";
    for (int index = moduleNum-1;index >= 0;index--) {
      uint8_t i;

      for(i = 0; i < 8; i++) {
          digitalWrite(DATA, !!(moduleDataList[index] & (1 << (7 - i))));
          vTaskDelay(5);
          digitalWrite(SHCP, HIGH);
          vTaskDelay(5);
          digitalWrite(SHCP, LOW);
          vTaskDelay(5);
      }



      // shiftOut(DATA, SHCP, MSBFIRST, moduleDataList[index]);

      char btyeContent[9];
      btyeContent[8] = '\0';
      for (int i = 0; i <= 7; i++) {
        btyeContent[i] = ((moduleDataList[index] >> i) & 1) ? '1' : '0';
      }
      settingContent += String(btyeContent) + "|";
    }
    digitalWrite(STCP, HIGH);
    ESP_LOGV("蠕動馬達", "當前設定: %s", settingContent.c_str());
    vTaskDelay(10);
    xSemaphoreGive(Peristaltic_Motors_xMutex);
  }
}


void C_Peristaltic_Motors_Ctrl::SetAllMotorStop()
{
  if (xSemaphoreTake(Peristaltic_Motors_xMutex, portMAX_DELAY) == pdTRUE) {
    memset(moduleDataList, 0, moduleNum);
    xSemaphoreGive(Peristaltic_Motors_xMutex);
  }
  RunMotor(moduleDataList);
}

void C_Peristaltic_Motors_Ctrl::OpenAllPin()
{
  if (xSemaphoreTake(Peristaltic_Motors_xMutex, portMAX_DELAY) == pdTRUE) {
    memset(moduleDataList, 0b11111111, moduleNum);
    xSemaphoreGive(Peristaltic_Motors_xMutex);
  }
  RunMotor(moduleDataList);
}

void C_Peristaltic_Motors_Ctrl::SetMotorStatus(int index, PeristalticMotorStatus status)
{
  int moduleChoseIndex = index / 4;
  int motortChoseIndexInModule = index % 4;
  // ESP_LOGI("蠕動馬達", "將模組: %d 的第 %d 顆馬達數值更改為 %d", moduleChoseIndex+1, motortChoseIndexInModule+1, status);
  if (moduleChoseIndex >= moduleNum) {}
  else {
    if (xSemaphoreTake(Peristaltic_Motors_xMutex, portMAX_DELAY) == pdTRUE) {
      moduleDataList[moduleChoseIndex] &= ~(0b11 << motortChoseIndexInModule*2);
      moduleDataList[moduleChoseIndex] |= (status << motortChoseIndexInModule*2);
      xSemaphoreGive(Peristaltic_Motors_xMutex);
    }
  }
}

void C_Peristaltic_Motors_Ctrl::ShowNowSetting()
{
  char btyeContent[9];
  btyeContent[8] = '\0';
  for (int index = 0;index < moduleNum;index++) {
    for (int i = 0; i <= 7; i++) {
      btyeContent[i] = ((moduleDataList[index] >> i) & 1) ? '1' : '0';
    }
    ESP_LOGI("蠕動馬達", "當前設定: %d -> %s", index, btyeContent);
  }
}

bool C_Peristaltic_Motors_Ctrl::IsAllStop() {
  if (xSemaphoreTake(Peristaltic_Motors_xMutex, portMAX_DELAY) == pdTRUE) {
    for (int index = moduleNum-1;index >= 0;index--) {
      if ((int)moduleDataList[index] != 0) {
        xSemaphoreGive(Peristaltic_Motors_xMutex);
        return false;
      }
    }
    xSemaphoreGive(Peristaltic_Motors_xMutex);
    return true;
  }
  return false;
}
