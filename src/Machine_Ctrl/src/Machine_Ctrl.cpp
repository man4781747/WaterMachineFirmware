#include "Machine_Ctrl.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include <Pools_Ctrl.h>
#include <Machine_Base_info.h>
#include <Motor_Ctrl.h>

TaskHandle_t TASK_SwitchMotorScan = NULL;

////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

void SMachine_Ctrl::INIT_SPIFFS_config()
{
  spiffs.INIT_SPIFFS();
  MachineInfo = spiffs.LoadMachineSetting();
}


////////////////////////////////////////////////////
// For 不間斷監聽
////////////////////////////////////////////////////

void Task_SwitchMotorScan(void * parameter)
{
  for (;;) {
    if (Machine_Ctrl.motorCtrl.active == MotorCtrlSteps::Active) {
      Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Running;
      int arrayLength = sizeof(Machine_Ctrl.motorCtrl.motorsArray) / sizeof(Machine_Ctrl.motorCtrl.motorsArray[0]);
      for (int index=0; index<arrayLength; index++) {
        if (Machine_Ctrl.motorCtrl.motorsArray[index].channelIndex == -1) {
          ESP_LOGI("Task_ChangeMotorStatus","Skip Motor %02d!", index);
          continue;
        }
        // if ((Machine_Ctrl.motorCtrl.motorsStatusCode >> index) & 1) {
        if ((Machine_Ctrl.motorCtrl.motorsStatusCode >> index) & 1) {
          ESP_LOGI("Task_ChangeMotorStatus","Motor %02d OPEN!", index);
          Machine_Ctrl.motorCtrl.SetMotorTo(index, MotorSwitchStatus::OPEN);
        } else {
          ESP_LOGI("Task_ChangeMotorStatus","Motor %02d CLOSE!", index);
          Machine_Ctrl.motorCtrl.SetMotorTo(index, MotorSwitchStatus::CLOSE);
        }
        vTaskDelay(10);
      }
      vTaskDelay(2000);
      Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Idel;
    }
    vTaskDelay(100);
  }
}
void SMachine_Ctrl::Build_SwitchMotorScan() {
  TaskHandle_t myTaskHandle;
  myTaskHandle = xTaskGetHandle("SwitchMotorScan");
  if (myTaskHandle == NULL) {
    xTaskCreate(
      Task_SwitchMotorScan, "SwitchMotorScan",
      10000, NULL, 1, &TASK_SwitchMotorScan
    );
  } else {
    ESP_LOGW("Build_SwitchMotorScan","Task Busy");
  } 
}


////////////////////////////////////////////////////
// For 互動相關
////////////////////////////////////////////////////

/**
 * @brief 獲得儀器基本資訊
 * 
 * @return DynamicJsonDocument 
 */
DynamicJsonDocument SMachine_Ctrl::GetDeviceInfos()
{
  return MachineInfo.GetDeviceInfo();
};

String SMachine_Ctrl::GetDeviceInfosString()
{
  void* json_output = malloc(10000);
  DynamicJsonDocument json_doc = MachineInfo.GetDeviceInfo();
  serializeJsonPretty(json_doc, json_output, 10000);
  String returnString = String((char*)json_output);
  free(json_output);
  json_doc.clear();
  return returnString;
};

////////////////////////////////////////////////////
// For 基礎行為
////////////////////////////////////////////////////


////////////////////////////////////////////////////
// For 組合行為
////////////////////////////////////////////////////

void SMachine_Ctrl::PumpPoolWaterToTempTank()
{
  while (Machine_Ctrl.motorCtrl.active != MotorCtrlSteps::Idel) {delay(10);}
  Machine_Ctrl.motorCtrl.motorsStatusCode = MOTOR_STATUS::ALL_CLOSE;
  Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Active;

  while (Machine_Ctrl.motorCtrl.active != MotorCtrlSteps::Idel) {delay(10);}
  Machine_Ctrl.motorCtrl.motorsStatusCode = MOTOR_STATUS::ALL_OPEN;
  Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Active;

  while (Machine_Ctrl.motorCtrl.active != MotorCtrlSteps::Idel) {delay(10);}
  Machine_Ctrl.motorCtrl.motorsStatusCode = MOTOR_STATUS::GET_REAGENT_1;
  Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Active;

  while (Machine_Ctrl.motorCtrl.active != MotorCtrlSteps::Idel) {delay(10);}
  Machine_Ctrl.motorCtrl.motorsStatusCode = MOTOR_STATUS::GET_REAGENT_2;
  Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Active;

  while (Machine_Ctrl.motorCtrl.active != MotorCtrlSteps::Idel) {delay(10);}
  Machine_Ctrl.motorCtrl.motorsStatusCode = MOTOR_STATUS::GET_REAGENT_3;
  Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Active;

  while (Machine_Ctrl.motorCtrl.active != MotorCtrlSteps::Idel) {delay(10);}
  Machine_Ctrl.motorCtrl.motorsStatusCode = MOTOR_STATUS::GET_REAGENT_4;
  Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Active;
  
};


////////////////////////////////////////////////////
// For 測試
////////////////////////////////////////////////////

void SMachine_Ctrl::UpdateAllPoolsDataRandom()
{
  poolsCtrl.UpdateAllPoolsDataRandom();
};


////////////////////////////////////////////////////
// 捨棄使用，純紀錄
////////////////////////////////////////////////////

// typedef struct {
//   bool waitForTigger = false;
//   char nextTaskName[17];
//   u_int32_t param;
// } TaskParamItem_t;
// 
// void Task_ChangeMotorStatus(void * parameter)
// {
//   ESP_LOGI("Task_ChangeMotorStatus","ChangeMotorStatus START");
//   TaskParamItem_t* myParams = (TaskParamItem_t*)parameter;
//   bool waitForTigger = myParams->waitForTigger;
//   char* nextTaskName = myParams->nextTaskName;
//   if (waitForTigger == true) {
//     ESP_LOGI("Task_ChangeMotorStatus","Wait for fontTask finish");
//     ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//   }
//   u_int32_t mororStatusCode = myParams->param;
//   int arrayLength = sizeof(Machine_Ctrl.motorCtrl.motorsArray) / sizeof(Machine_Ctrl.motorCtrl.motorsArray[0]);
//   for (int index=0; index<arrayLength; index++) {
//     if (Machine_Ctrl.motorCtrl.motorsArray[index].channelIndex == -1) {
//       ESP_LOGI("Task_ChangeMotorStatus","Skip Motor %02d!", index);
//       continue;
//     }
//     // if ((Machine_Ctrl.motorCtrl.motorsStatusCode >> index) & 1) {
//     if ((mororStatusCode >> index) & 1) {
//       ESP_LOGI("Task_ChangeMotorStatus","Motor %02d OPEN!", index);
//       Machine_Ctrl.motorCtrl.SetMotorTo(index, 180);
//     } else {
//       ESP_LOGI("Task_ChangeMotorStatus","Motor %02d CLOSE!", index);
//       Machine_Ctrl.motorCtrl.SetMotorTo(index, 0);
//     }
//     vTaskDelay(10);
//   }
//   vTaskDelay(2000);
//   ESP_LOGI("Task_ChangeMotorStatus","ChangeMotorStatus END");
//   TaskHandle_t nextTask = xTaskGetHandle(nextTaskName);
//   if (nextTask != NULL) {
//     xTaskNotifyGive(nextTask);
//   }
//   vTaskDelete(NULL);
// };
//
// /**
//  * @brief 建立伺服馬達控制Task
//  * 
//  * @param StatusCode 伺服馬達狀態碼
//  * @param TaskName Task名稱
//  * @param NextTaskName 下一個Triger的Task名稱 (default="")
//  * @param waitForTigger 是否等待觸發後才執行 (default=fasle)
//  */
// void SMachine_Ctrl::ChangeMotorStatus(MOTOR_STATUS StatusCode, char* TaskName, char* NextTaskName, bool waitForTigger)
// {
//   TaskHandle_t myTaskHandle;
//   myTaskHandle = xTaskGetHandle(TaskName);
//   if (myTaskHandle == NULL) {
//     TaskHandle_t* Task = new TaskHandle_t;
//     TaskParamItem_t* TaskPara = new TaskParamItem_t;
//     TaskPara->param = StatusCode;
//     strncpy(TaskPara->nextTaskName, NextTaskName, sizeof(TaskPara->nextTaskName));
//     TaskPara->waitForTigger = waitForTigger;
//     xTaskCreate(
//       Task_ChangeMotorStatus, TaskName,
//       10000, TaskPara, 1, Task
//     );
//   } else {
//     ESP_LOGW("ChangeMotorStatus","Task Busy");
//   } 
// };