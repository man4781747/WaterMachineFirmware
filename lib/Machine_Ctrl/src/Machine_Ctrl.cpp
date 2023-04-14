#include "Machine_Ctrl.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include <Pools_Ctrl.h>
#include "../../Machine_Base_info/src/Machine_Base_info.h"
#include <Motor_Ctrl.h>


// 伺服馬達相關
// Motor_Ctrl motorCtrl;

extern Machine_Info MachineInfo;
extern SMachine_Ctrl Machine_Ctrl;

TaskHandle_t TASK_PumpPoolWaterToTempTank = NULL;
TaskHandle_t TASK_ChangeMotorStatus = NULL;

DynamicJsonDocument SMachine_Ctrl::GetDeviceInfos()
{
  DynamicJsonDocument json_doc(6000);
  JsonVariant json_obj = json_doc.to<JsonVariant>();
  json_doc["device_no"].set(MachineInfo.device_no);
  json_doc["FIRMWARE_VERSION"].set(MachineInfo.FIRMWARE_VERSION);
  json_doc["mode"].set("Mode_Slave");
  json_doc["parameter"].set(poolsCtrl.GetAllPoolsBaseInfo());
  return json_doc;
};

String SMachine_Ctrl::GetDeviceInfosString()
{
  void* json_output = malloc(6000);
  DynamicJsonDocument json_doc(6000);
  JsonVariant json_obj = json_doc.to<JsonVariant>();
  json_doc["device_no"].set(MachineInfo.device_no);
  json_doc["FIRMWARE_VERSION"].set(MachineInfo.FIRMWARE_VERSION);
  json_doc["mode"].set("Mode_Slave");
  json_doc["parameter"].set(poolsCtrl.GetAllPoolsBaseInfo());
  serializeJsonPretty(json_doc, json_output, 6000);
  String returnString = String((char*)json_output);
  free(json_output);
  return returnString;
};

void SMachine_Ctrl::UpdateAllPoolsDataRandom()
{
  poolsCtrl.UpdateAllPoolsDataRandom();
};


void Task_PumpPoolWaterToTempTank(void * parameter)
{
  ESP_LOGI("Task_PumpPoolWaterToTempTank","PumpPoolWaterToTempTank START");
  vTaskDelay(3000);
  ESP_LOGI("Task_PumpPoolWaterToTempTank","PumpPoolWaterToTempTank END");
  vTaskDelete(NULL);
};
void SMachine_Ctrl::PumpPoolWaterToTempTank()
{
  TaskHandle_t myTaskHandle;
  myTaskHandle = xTaskGetHandle("TPumpPoolWater");
  if (myTaskHandle == NULL) {
    xTaskCreate(
      Task_PumpPoolWaterToTempTank, "TPumpPoolWater",
      10000, NULL, 1, &TASK_PumpPoolWaterToTempTank
    );
  } else {
    ESP_LOGW("PumpPoolWaterToTempTank","Task Busy");
  } 
};


void Task_ChangeMotorStatus(void * parameter)
{
  ESP_LOGI("Task_ChangeMotorStatus","ChangeMotorStatus START");
  int arrayLength = sizeof(Machine_Ctrl.motorCtrl.motorsArray) / sizeof(Machine_Ctrl.motorCtrl.motorsArray[0]);
  for (int index=0; index<arrayLength; index++) {
    if (Machine_Ctrl.motorCtrl.motorsArray[index].channelIndex == -1) {
      ESP_LOGI("Task_ChangeMotorStatus","Skip Motor %02d!", index);
      continue;
    }
    if ((Machine_Ctrl.motorCtrl.motorsStatusCode >> index) & 1) {
      ESP_LOGI("Task_ChangeMotorStatus","Motor %02d OPEN!", index);
      Machine_Ctrl.motorCtrl.SetMotorTo(index, 180);
    } else {
      ESP_LOGI("Task_ChangeMotorStatus","Motor %02d CLOSE!", index);
      Machine_Ctrl.motorCtrl.SetMotorTo(index, 0);
    }
    vTaskDelay(10);
  }
  vTaskDelay(1000);
  ESP_LOGI("Task_ChangeMotorStatus","ChangeMotorStatus END");
  vTaskDelete(NULL);
};
void SMachine_Ctrl::ChangeMotorStatus(MOTOR_STATUS StatusCode)
{
  Machine_Ctrl.motorCtrl.motorsStatusCode = StatusCode;
  TaskHandle_t myTaskHandle;
  myTaskHandle = xTaskGetHandle("TCgMotorStatus");
  if (myTaskHandle == NULL) {
    xTaskCreate(
      Task_ChangeMotorStatus, "TCgMotorStatus",
      10000, NULL, 1, &TASK_ChangeMotorStatus
    );
  } else {
    ESP_LOGW("ChangeMotorStatus","Task Busy");
  } 
};