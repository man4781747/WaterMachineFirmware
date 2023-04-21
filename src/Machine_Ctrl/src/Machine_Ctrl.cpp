#include "Machine_Ctrl.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include <Pools_Ctrl.h>
#include <Machine_Base_info.h>
#include <Motor_Ctrl.h>
#include <vector>
#include <variant>

TaskHandle_t TASK_SwitchMotorScan = NULL;
TaskHandle_t TASK_PeristalticMotorScan = NULL;


////////////////////////////////////////////////////
// RUN_MOTOR_GROUP 設定
////////////////////////////////////////////////////

/**
 * @brief 清空混合室
 * 
 */
RUN_MOTOR_GROUP Clear_MixRoom {
  "清空混合室", "清空混合室內所有液體，將其作為廢液排出。",
  {
    {PWM_POSITION_MAPPING::S_M8, MotorSwitchStatus::LeftCenter},
  },
  {PERISTALTIC_MOTOR_MAPPING::M7, PeristalticMotorStatus::FORWARD, 5}
};

/**
 * @brief 混合混合室的液體
 * 
 */
RUN_MOTOR_GROUP Mix_Liquid_In_MixRoom {
  "混合混合室的液體", "混合混合室的液體。",
  {
    {PWM_POSITION_MAPPING::S_M8, MotorSwitchStatus::LeftCenter},
  },
  {PERISTALTIC_MOTOR_MAPPING::M7, PeristalticMotorStatus::REVERSR, 5}
};

/**
 * @brief 抽取試劑水至混合室
 * 
 */
RUN_MOTOR_GROUP Push_RO_Liquid_To_MixRoom {
  "抽取試劑水至混合室", "抽取試劑水，並送至混合室",
  {
    {PWM_POSITION_MAPPING::S_M1, MotorSwitchStatus::LeftCenter},
    {PWM_POSITION_MAPPING::S_M2, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M3, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M4, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M5, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M6, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M7, MotorSwitchStatus::LeftRight},
  },
  {PERISTALTIC_MOTOR_MAPPING::M4, PeristalticMotorStatus::FORWARD, 5}
};

/**
 * @brief 抽取樣本水至混合室
 * 
 */
RUN_MOTOR_GROUP Push_Sample_Liquid_To_MixRoom {
  "抽取樣本水至混合室", "抽取樣本水，並送至混合室",
  {
    {PWM_POSITION_MAPPING::S_M0, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M1, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M2, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M3, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M4, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M5, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M6, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M7, MotorSwitchStatus::LeftRight},
  },
  {PERISTALTIC_MOTOR_MAPPING::M4, PeristalticMotorStatus::FORWARD, 5}
};


/**
 * @brief 抽取亞硝酸鹽至混合室
 * 
 */
RUN_MOTOR_GROUP Push_NO2_Liquid_To_MixRoom {
  "抽取亞硝酸鹽至混合室", "抽取亞硝酸鹽至混合室",
  {
    {PWM_POSITION_MAPPING::S_M4, MotorSwitchStatus::LeftCenter},
    {PWM_POSITION_MAPPING::S_M5, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M6, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_M7, MotorSwitchStatus::LeftRight},
  },
  {PERISTALTIC_MOTOR_MAPPING::M4, PeristalticMotorStatus::FORWARD, 5}
};

/**
 * @brief 抽取氨氮R1至混合室
 * 
 */
RUN_MOTOR_GROUP Push_NH3R1_Liquid_To_MixRoom {
  "抽取氨氮R1至混合室", "抽取氨氮R1至混合室",
  {
    {PWM_POSITION_MAPPING::S_M6, MotorSwitchStatus::LeftCenter},
    {PWM_POSITION_MAPPING::S_M7, MotorSwitchStatus::LeftRight},
  },
  {PERISTALTIC_MOTOR_MAPPING::M4, PeristalticMotorStatus::FORWARD, 5}
};

/**
 * @brief 抽取氨氮R2至混合室
 * 
 */
RUN_MOTOR_GROUP Push_NH3R2_Liquid_To_MixRoom {
  "抽取氨氮R2至混合室", "抽取氨氮R2至混合室",
  {},
  {PERISTALTIC_MOTOR_MAPPING::M5, PeristalticMotorStatus::FORWARD, 5}
};



/**
 * @brief 抽取混合室溶液至亞硝酸鹽光度檢測室
 * 
 */
RUN_MOTOR_GROUP Push_MixRoom_To_NO2_SensorRoom {
  "抽取混合室溶液至亞硝酸鹽光度檢測室", "抽取混合室溶液，並送至亞硝酸鹽光度檢測室",
  {
    {PWM_POSITION_MAPPING::S_M8, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_L1, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_L2, MotorSwitchStatus::LeftCenter},
  },
  {PERISTALTIC_MOTOR_MAPPING::M7, PeristalticMotorStatus::FORWARD, 5}
};

/**
 * @brief 亞硝酸鹽光度檢測室溶液退回混合室
 * 
 */
RUN_MOTOR_GROUP Clear_NO2_SensorRoom_To_MixRoom {
  "亞硝酸鹽光度檢測室溶液退回混合室", "亞硝酸鹽光度檢測室溶液退回混合室",
  {
    {PWM_POSITION_MAPPING::S_M8, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_L1, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_L2, MotorSwitchStatus::LeftCenter},
  },
  {PERISTALTIC_MOTOR_MAPPING::M7, PeristalticMotorStatus::REVERSR, 5}
};



/**
 * @brief 抽取混合室溶液至氮氧光度檢測室
 * 
 */
RUN_MOTOR_GROUP Push_MixRoom_To_NH3_SensorRoom {
  "抽取混合室溶液至氮氧光度檢測室", "抽取混合室溶液至氮氧光度檢測室",
  {
    {PWM_POSITION_MAPPING::S_M8, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_L1, MotorSwitchStatus::LeftCenter},
  },
  {PERISTALTIC_MOTOR_MAPPING::M7, PeristalticMotorStatus::FORWARD, 5}
};

/**
 * @brief 亞硝酸鹽光度檢測室溶液退回混合室
 * 
 */
RUN_MOTOR_GROUP Clear_NH3_SensorRoom_To_MixRoom {
  "亞硝酸鹽光度檢測室溶液退回混合室", "亞硝酸鹽光度檢測室溶液退回混合室",
  {
    {PWM_POSITION_MAPPING::S_M8, MotorSwitchStatus::LeftRight},
    {PWM_POSITION_MAPPING::S_L1, MotorSwitchStatus::LeftCenter},
  },
  {
    PERISTALTIC_MOTOR_MAPPING::M7, PeristalticMotorStatus::REVERSR, 5
  }
};

////////////////////////////////////////////////////
// For 事件組設定
////////////////////////////////////////////////////

RUN_EVENT_GROUP RUN_NO2_Original_Value {
  "亞硝酸鹽原點檢測", "亞硝酸鹽原點檢測",
  {
    EVENT_GROUP(&Clear_MixRoom),
    EVENT_GROUP(&Push_RO_Liquid_To_MixRoom),
    EVENT_GROUP(&Push_MixRoom_To_NO2_SensorRoom),
    EVENT_GROUP(&Clear_NO2_SensorRoom_To_MixRoom),
    EVENT_GROUP(&Clear_MixRoom),
  }
};

RUN_EVENT_GROUP RUN_NO2_Test_Solution_Value {
  "亞硝酸鹽待測液檢測", "亞硝酸鹽待測液檢測",
  {
    EVENT_GROUP(&Clear_MixRoom),
    EVENT_GROUP(&Push_RO_Liquid_To_MixRoom),
    EVENT_GROUP(&Push_Sample_Liquid_To_MixRoom),
    EVENT_GROUP(&Mix_Liquid_In_MixRoom),
    EVENT_GROUP(&Push_NO2_Liquid_To_MixRoom),
    EVENT_GROUP(&Mix_Liquid_In_MixRoom),
    EVENT_GROUP(&Push_MixRoom_To_NO2_SensorRoom),
    EVENT_GROUP(&Clear_NO2_SensorRoom_To_MixRoom),
    EVENT_GROUP(&Clear_MixRoom),
  }
};

RUN_EVENT_GROUP RUN_NH3_Original_Value {
  "氨氮原點檢測", "氨氮原點檢測",
  {
    EVENT_GROUP(&Clear_MixRoom),
    EVENT_GROUP(&Push_Sample_Liquid_To_MixRoom),
    EVENT_GROUP(&Push_MixRoom_To_NH3_SensorRoom),
    EVENT_GROUP(&Clear_NH3_SensorRoom_To_MixRoom),
    EVENT_GROUP(&Clear_MixRoom),
  }
};

RUN_EVENT_GROUP RUN_NH3_Test_Solution_Value {
  "氨氮試劑檢測", "氨氮試劑檢測",
  {
    EVENT_GROUP(&Clear_MixRoom),
    EVENT_GROUP(&Push_Sample_Liquid_To_MixRoom),
    EVENT_GROUP(&Push_NH3R1_Liquid_To_MixRoom),
    EVENT_GROUP(&Clear_MixRoom),
    EVENT_GROUP(&Push_NH3R2_Liquid_To_MixRoom),
    EVENT_GROUP(&Clear_MixRoom),
    EVENT_GROUP(&Push_MixRoom_To_NH3_SensorRoom),
    EVENT_GROUP(&Clear_NH3_SensorRoom_To_MixRoom),
    EVENT_GROUP(&Clear_MixRoom),
  }
};


////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

void SMachine_Ctrl::INIT_SPIFFS_config()
{
  spiffs.INIT_SPIFFS();
  MachineInfo = spiffs.LoadMachineSetting();
  // DynamicJsonDocument DeviceSetting_ = spiffs.GetDeviceSetting();
  spiffs.GetDeviceSetting();

  // JsonObject obj = DeviceSetting->as<JsonObject>();
  // obj = obj["pwm_motor"];


  // UpdatePWMMotorSetting(obj);

  // for (JsonObject::iterator it = obj.begin(); it != obj.end(); ++it) {
  //   Serial.println(it->key().c_str());
  // }
}

void SMachine_Ctrl::INIT_SW_Moter()
{
  motorCtrl.INIT_Motors();
  JsonObject obj = spiffs.DeviceSetting->as<JsonObject>();
  UpdatePWMMotorSetting(obj["pwm_motor"]);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_B1);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_B2);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_B3);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_B4);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M0);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M1);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M2);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M3);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M4);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M5);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M6);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M7);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_M8);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_L1);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_L2);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_PH1);
  // motorCtrl.AddNewMotor(PWM_POSITION_MAPPING::S_PH2);
}

void SMachine_Ctrl::INIT_Peristaltic_Moter()
{
  peristalticMotorsCtrl.INIT_Motors();
  JsonObject obj = spiffs.DeviceSetting->as<JsonObject>();
  UpdatePeristalticMotorSetting(obj["peristaltic_motor"]);


  // peristalticMotorsCtrl.AddNewMotor(PERISTALTIC_MOTOR_MAPPING::M1);
  // peristalticMotorsCtrl.AddNewMotor(PERISTALTIC_MOTOR_MAPPING::M2);
  // peristalticMotorsCtrl.AddNewMotor(PERISTALTIC_MOTOR_MAPPING::M3);
  // peristalticMotorsCtrl.AddNewMotor(PERISTALTIC_MOTOR_MAPPING::M4);
  // peristalticMotorsCtrl.AddNewMotor(PERISTALTIC_MOTOR_MAPPING::M5);
  // peristalticMotorsCtrl.AddNewMotor(PERISTALTIC_MOTOR_MAPPING::M6);
  // peristalticMotorsCtrl.AddNewMotor(PERISTALTIC_MOTOR_MAPPING::M7);
}

////////////////////////////////////////////////////
// For 更新設定
////////////////////////////////////////////////////

void SMachine_Ctrl::UpdatePWMMotorSetting(JsonObject PWMMotorSetting)
{
  for (JsonObject::iterator it = PWMMotorSetting.begin(); it != PWMMotorSetting.end(); ++it) {
    motorCtrl.AddNewMotor(
      PWMMotorSetting[it->key().c_str()]["index"], String(it->key().c_str()), 
      PWMMotorSetting[it->key().c_str()]["title"], PWMMotorSetting[it->key().c_str()]["description"]
    );
  }
}

void SMachine_Ctrl::UpdatePeristalticMotorSetting(JsonObject PeristalticMotorSetting)
{
  for (JsonObject::iterator it = PeristalticMotorSetting.begin(); it != PeristalticMotorSetting.end(); ++it) {
    peristalticMotorsCtrl.AddNewMotor(
      PeristalticMotorSetting[it->key().c_str()]["index"], String(it->key().c_str()),
      PeristalticMotorSetting[it->key().c_str()]["title"], PeristalticMotorSetting[it->key().c_str()]["description"]
    );
  }
}


////////////////////////////////////////////////////
// For 不間斷監聽
////////////////////////////////////////////////////

void Task_SwitchMotorScan(void * parameter)
{
  ESP_LOGI("SMachine_Ctrl","Task_SwitchMotorScan Run");
  for (;;) {
    if (Machine_Ctrl.motorCtrl.active == MotorCtrlSteps::Active) {
      Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Running;
      int arrayLength = sizeof(Machine_Ctrl.motorCtrl.motorsArray) / sizeof(Machine_Ctrl.motorCtrl.motorsArray[0]);
      for (int index=0; index<arrayLength; index++) {
        if (Machine_Ctrl.motorCtrl.motorsArray[index].channelIndex == -1) {
          // ESP_LOGI("Task_ChangeMotorStatus","Skip Motor %02d!", index);
          continue;
        }
        Machine_Ctrl.motorCtrl.MotorStatusChange(index);
        vTaskDelay(10);
      }
      vTaskDelay(2000);
      Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Idel;
    }
    vTaskDelay(100);
  }
}
void SMachine_Ctrl::Build_SwitchMotorScan()
{
  TaskHandle_t myTaskHandle;
  myTaskHandle = xTaskGetHandle("SwitchMotorScan");
  if (myTaskHandle == NULL) {
    ESP_LOGI("SMachine_Ctrl","Build_SwitchMotorScan");
    xTaskCreate(
      Task_SwitchMotorScan, "SwitchMotorScan",
      10000, NULL, 1, &TASK_SwitchMotorScan
    );
  } else {
    ESP_LOGW("Build_SwitchMotorScan","Task Busy");
  } 
}

void Task_PeristalticMotorScan(void * parameter)
{
  ESP_LOGI("SMachine_Ctrl","Task_PeristalticMotorScan Run");
  for (;;) {
    for (int motorChose=0;motorChose<16;motorChose++) {
      time_t nowTime = now();
      if (nowTime < Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorEndTime) {
        if (
          Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNowStatus != Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNextStatus
        ) {
          ESP_LOGI("Task_PeristalticMotorScan","Peristaltic Motors %d change status: %d -> %d",
            motorChose, Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNowStatus,
            Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNextStatus
          );
          Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNowStatus = Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNextStatus;
        }
      } else {
        if (
          Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNowStatus != MotorCtrlSteps::Idel
        ) {
          ESP_LOGI("Task_PeristalticMotorScan","Peristaltic Motors %d stop", motorChose);
          Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNowStatus = MotorCtrlSteps::Idel;
          Machine_Ctrl.peristalticMotorsCtrl.motorsArray[motorChose].motorNextStatus = MotorCtrlSteps::Idel;
        }
      }
    }
    vTaskDelay(10);
  }
}
void SMachine_Ctrl::Build_PeristalticMotorScan()
{
  TaskHandle_t myTaskHandle;
  myTaskHandle = xTaskGetHandle("P_MotorScan");
  if (myTaskHandle == NULL) {
    ESP_LOGI("SMachine_Ctrl","Build_PeristalticMotorScan");
    xTaskCreate(
      Task_PeristalticMotorScan, "P_MotorScan",
      10000, NULL, 1, &TASK_PeristalticMotorScan
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

DynamicJsonDocument SMachine_Ctrl::GetEventStatus()
{
  DynamicJsonDocument json_doc(10000);
  if (NowDeviceStatus.deviceStatusCode == DeviceStatusCode::device_idel) {
    json_doc["device_status"].set("Idel");
  } else {
    json_doc["device_status"].set("Busy");
    json_doc["start_time"] = NowDeviceStatus.StartTime;
    json_doc["event"].set(NowDeviceStatus.NowRunningEvent->Title);
    json_doc["now_step"].set(NowDeviceStatus.NowStep);
    json_doc["now_step_name"].set(
      NowDeviceStatus.NowRunningEvent->eventGroup[NowDeviceStatus.NowStep].motorGroupEvent->Title
    );
  }

  return json_doc;
}

////////////////////////////////////////////////////
// For 基礎行為
////////////////////////////////////////////////////

void SMachine_Ctrl::Set_SW_MotorStatus(std::vector<PWM_MOTOR_STATUS_SET_OBJ> motorStatusList)
{
  ESP_LOGI("Machine_Ctrl","Set SW_Motor status with PWM_MOTOR_STATUS_SET_OBJ");
  for (auto& motorStatus : motorStatusList) {
    motorCtrl.SetMotorTo(motorStatus.motortIndex, motorStatus.motortStatus);
  }
  // int motorStatusListLen = sizeof(motorStatusList) / sizeof(PWM_MOTOR_STATUS_SET_OBJ);
  // int maxMotorLen = sizeof(motorCtrl.motorsArray) / sizeof(motorCtrl.motorsArray[0]);
  // ESP_LOGI("Machine_Ctrl","motorStatusListLen: %02d, maxMotorLen: %02d");
  // for (int motorDataChose=0;motorDataChose<len ;motorDataChose++){
  //   motorCtrl.SetMotorTo(motorStatusList[motorDataChose].motortIndex, motorStatusList[motorDataChose].motortStatus);
  // }
}
////////////////////////////////////////////////////
// For 組合行為
////////////////////////////////////////////////////

void SMachine_Ctrl::SwitchPWMMotor__AND__RunPeristalticMotor(RUN_MOTOR_GROUP *setting)
{
  ESP_LOGI("Machine_Ctrl","執行馬達控制設定組: %s", setting->Title.c_str());
  ESP_LOGI("Machine_Ctrl","%s", setting->Description.c_str());
  Machine_Ctrl.Set_SW_MotorStatus(setting->pwmCtrlList);
  motorCtrl.active = MotorCtrlSteps::Active;
  while(motorCtrl.active != MotorCtrlSteps::Idel){
    delay(1);
  }
  // setting->perostalicMotorCtrl.motortIndex
  peristalticMotorsCtrl.RunMotor(
    setting->perostalicMotorCtrl.motortIndex, 
    setting->perostalicMotorCtrl.motortStatus, 
    setting->perostalicMotorCtrl.activeTime
  );
  delay(setting->perostalicMotorCtrl.activeTime*1000+100);
  ESP_LOGI("Machine_Ctrl","執行馬達控制設定組: %s 完成!", setting->Title.c_str());
}

void SMachine_Ctrl::RUN_EVENT(RUN_EVENT_GROUP *eventGroupSetting){
  ESP_LOGI("RUN_EVENT","執行流程: %s", eventGroupSetting->Title.c_str());
  ESP_LOGI("RUN_EVENT","%s", eventGroupSetting->Description.c_str());
  int StepCount = 0;
  NowDeviceStatus.deviceStatusCode = DeviceStatusCode::device_busy;
  NowDeviceStatus.NowRunningEvent = eventGroupSetting;
  time_t nowTime = now();
  char datetimeChar[30];
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  NowDeviceStatus.StartTime = String(nowTime);
  for (auto& eventGroup : eventGroupSetting->eventGroup) {
    DynamicJsonDocument json_doc = BackendServer.GetBaseWSReturnData("event_update");
    json_doc["parameter"]["Message"] = "event update";
    json_doc["message"].set("OK");
    // 發出訊息 
    String returnString;
    serializeJsonPretty(json_doc, returnString);
    BackendServer.ws_->textAll(returnString);

    ESP_LOGI("RUN_EVENT","%s - 步驟 %d", eventGroupSetting->Description.c_str(), StepCount+1);
    NowDeviceStatus.NowStep = StepCount;
    StepCount++;
    if (eventGroup.motorGroupEvent != NULL) {
      SwitchPWMMotor__AND__RunPeristalticMotor(eventGroup.motorGroupEvent);
    } else {

    }
  }
  ESP_LOGI("Machine_Ctrl","流程: %s 執行完畢", eventGroupSetting->Title.c_str());
  NowDeviceStatus.deviceStatusCode = DeviceStatusCode::device_idel;
  NowDeviceStatus.NowRunningEvent = NULL;
};


void SMachine_Ctrl::RUN_NO2_Original_Value(){
  ESP_LOGI("Machine_Ctrl","亞硝酸鹽原點數值檢測流程開始");
  SwitchPWMMotor__AND__RunPeristalticMotor(&Clear_MixRoom);
  SwitchPWMMotor__AND__RunPeristalticMotor(&Push_RO_Liquid_To_MixRoom);
  SwitchPWMMotor__AND__RunPeristalticMotor(&Push_MixRoom_To_NO2_SensorRoom);
  ESP_LOGI("Machine_Ctrl","亞硝酸鹽原點數值檢測測試，目前還沒寫code");
  delay(3000);
  SwitchPWMMotor__AND__RunPeristalticMotor(&Clear_NO2_SensorRoom_To_MixRoom);
  SwitchPWMMotor__AND__RunPeristalticMotor(&Clear_MixRoom);

  ESP_LOGI("Machine_Ctrl","亞硝酸鹽原點數值檢測流程完畢");
};

////////////////////////////////////////////////////
// For 測試
////////////////////////////////////////////////////

void SMachine_Ctrl::UpdateAllPoolsDataRandom()
{
  poolsCtrl.UpdateAllPoolsDataRandom();
};

void SMachine_Ctrl::LoopTest()
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