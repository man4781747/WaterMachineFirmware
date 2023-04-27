#include "Machine_Ctrl.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include <TimeLib.h>   

#include <Pools_Ctrl.h>
#include <Machine_Base_info.h>
#include <Motor_Ctrl.h>
#include <vector>
#include <variant>

TaskHandle_t TASK_SwitchMotorScan = NULL;
TaskHandle_t TASK_PeristalticMotorScan = NULL;

////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

/**
 * @brief SPIFFS 初始化
 * 同時會讀取基礎資訊&參數設定檔
 * 
 */
void SMachine_Ctrl::INIT_SPIFFS_config()
{
  spiffs.INIT_SPIFFS();
  MachineInfo = spiffs.LoadMachineSetting();
  spiffs.GetDeviceSetting();
}

/**
 * @brief 初始化伺服馬達控制物件
 * 同時依spiffs內的參數設定檔來建立伺服馬達物件
 * 
 */
void SMachine_Ctrl::INIT_SW_Moter()
{
  motorCtrl.INIT_Motors();
  JsonObject obj = spiffs.DeviceSetting->as<JsonObject>();
  UpdatePWMMotorSetting(obj["pwm_motor"]);
}

/**
 * @brief 初始化蠕動馬達控制物件
 * 同時依spiffs內的參數設定檔來建立蠕動馬達物件
 * 
 */
void SMachine_Ctrl::INIT_Peristaltic_Moter()
{
  peristalticMotorsCtrl.INIT_Motors();
  JsonObject obj = spiffs.DeviceSetting->as<JsonObject>();
  UpdatePeristalticMotorSetting(obj["peristaltic_motor"]);
}

/**
 * @brief 初始化事件組合
 * 
 */
void SMachine_Ctrl::INIT_UpdateEventGroupSetting()
{
  D_eventGroupList.clear();
  JsonObject obj = spiffs.DeviceSetting->as<JsonObject>();
  UpdateEventGroupSetting(obj["event_group"]);
}

/**
 * @brief 初始化流程組合
 * 
 */
void SMachine_Ctrl::INIT_UpdateStepGroupSetting()
{
  D_stepGroupList.clear();
  JsonObject obj = spiffs.DeviceSetting->as<JsonObject>();
  UpdateStepGroupSetting(obj["steps_group"]);
}

////////////////////////////////////////////////////
// For 更新設定
////////////////////////////////////////////////////

/**
 * @brief 依輸入內容重設伺服馬達設定
 * 
 * @param PWMMotorSetting JSON物件
 * {
 *  "馬達ID": {
 *    "index": pwm index,
 *    "title": 馬達名稱
 *    "description": 說明
 *  }, ...
 * }
 */
void SMachine_Ctrl::UpdatePWMMotorSetting(JsonObject PWMMotorSetting)
{
  motorCtrl.motorsDict.clear();
  for (JsonObject::iterator it = PWMMotorSetting.begin(); it != PWMMotorSetting.end(); ++it) {
    motorCtrl.AddNewMotor(
      PWMMotorSetting[it->key().c_str()]["index"], String(it->key().c_str()), 
      PWMMotorSetting[it->key().c_str()]["title"], PWMMotorSetting[it->key().c_str()]["description"]
    );
  }
}

/**
 * @brief 依輸入內容重設蠕動馬達設定
 * 
 * @param PeristalticMotorSetting JSON物件
 * {
 *  "馬達ID": {
 *    "index": index,
 *    "title": 馬達名稱
 *    "description": 說明
 *  }, ...
 * }
 */
void SMachine_Ctrl::UpdatePeristalticMotorSetting(JsonObject PeristalticMotorSetting)
{
  peristalticMotorsCtrl.motorsDict.clear();
  for (JsonObject::iterator it = PeristalticMotorSetting.begin(); it != PeristalticMotorSetting.end(); ++it) {
    peristalticMotorsCtrl.AddNewMotor(
      PeristalticMotorSetting[it->key().c_str()]["index"], String(it->key().c_str()),
      PeristalticMotorSetting[it->key().c_str()]["title"], PeristalticMotorSetting[it->key().c_str()]["description"]
    );
  }
}

/**
 * @brief 依輸入內容重設事件組
 * 
 * @param EventListSetting JSON物件
 * {
 *  "事件組ID": {
 *    "title": 事件組名稱
 *    "description": 說明,
 *    "event": [  // 清單形式，順序即為執行順率
 *      {
 *        "pwm_motor_list": [
 *          {
 *            "pwm_motor_id": <馬達ID>,"status": <0, 90 ,180 馬達角度>
 *          }, ...
 *        ]
 *      },
 *      {
 *        "peristaltic_motor_list": [
 *          {
 *            "peristaltic_motor_id": <蠕動馬達ID>,"status": <1 or -1>,"time": int <秒數>
 *          }, ...
 *        ]
 *      },
 *      {
 *        "wait": int <秒數>
 *      }
 *    ]
 *  }, ...
 * }
 */
void SMachine_Ctrl::UpdateEventGroupSetting(JsonObject EventListSetting)
{
  D_eventGroupList.clear();
  for (JsonObject::iterator it = EventListSetting.begin(); it != EventListSetting.end(); ++it) {
    String Title = EventListSetting[it->key().c_str()]["title"];
    String Description = EventListSetting[it->key().c_str()]["description"];
    std::vector<EVENT> EventList;
    JsonArray event_list = EventListSetting[it->key().c_str()]["event"];
    for (JsonVariant event_chose : event_list) {
      if (event_chose["pwm_motor_list"]) {
        JsonArray pwm_motor_list = event_chose["pwm_motor_list"];
        std::vector<PWM_MOTOR_STATUS_SET_OBJ> *PWM_motorList = new std::vector<PWM_MOTOR_STATUS_SET_OBJ>;
        for (JsonVariant pwm_motor_chose : pwm_motor_list) {
          PWM_motorList->push_back(
            {pwm_motor_chose["pwm_motor_id"], pwm_motor_chose["status"]}
          );
        }
        EVENT newEvent(PWM_motorList);
        EventList.push_back(newEvent);
      } else if (event_chose["peristaltic_motor_list"]) {
        JsonArray peristaltic_motor_list = event_chose["peristaltic_motor_list"];
        std::vector<PERISTALTIC_STATUS_SET_OBJ> *peristalticMotorList = new std::vector<PERISTALTIC_STATUS_SET_OBJ>;
        for (JsonVariant peristaltic_motor_chose : peristaltic_motor_list) {
          peristalticMotorList->push_back(
            {
              peristaltic_motor_chose["peristaltic_motor_id"], 
              peristaltic_motor_chose["status"], 
              peristaltic_motor_chose["time"]
            }
          );
          EVENT newEvent(peristalticMotorList);
          EventList.push_back(newEvent);
        }
      } else if (event_chose["wait"]) {
        WAIT_EVENT_OBJ *waitObj = new WAIT_EVENT_OBJ;
        waitObj->waitTime = event_chose["wait"];
        EVENT newEvent(waitObj);
        EventList.push_back(newEvent);
      }
    }
    D_eventGroupList[it->key().c_str()] = {
      Title, Description, EventList
    };
  }
}

/**
 * @brief 依輸入內容重設流程設定
 * 
 * @param StepGroupSetting JSON物件
 * {
 *  "流程ID": {
 *    "title": 流程名稱
 *    "description": 說明,
 *    "steps": [<事件ID>,...]
 *  }, ...
 * }
 */
void SMachine_Ctrl::UpdateStepGroupSetting(JsonObject StepGroupSetting)
{
  D_stepGroupList.clear();
  for (JsonObject::iterator it = StepGroupSetting.begin(); it != StepGroupSetting.end(); ++it) {
    String Title = StepGroupSetting[it->key().c_str()]["title"];
    String Description = StepGroupSetting[it->key().c_str()]["description"];
    JsonArray step_list = StepGroupSetting[it->key().c_str()]["steps"];
    std::vector<String> EventGroupNameList;
    for (JsonVariant step_chose : step_list) {
      EventGroupNameList.push_back(step_chose);
    }
    D_stepGroupList[it->key().c_str()] = {
      Title, Description, EventGroupNameList
    };
  }
}

////////////////////////////////////////////////////
// For 資訊獲得
////////////////////////////////////////////////////  

void SMachine_Ctrl::GetAllEventSetting()
{
  for (auto it = D_eventGroupList.begin(); it != D_eventGroupList.end(); ++it) {
    Serial.println("====================================================");
    Serial.println("Key:\t"+String(it->first.c_str()));
    Serial.println("Title:\t"+D_eventGroupList[it->first].Title);
    Serial.println("Desp:\t"+D_eventGroupList[it->first].Description);
    int eventCount = 1;
    for (auto& eventChose : D_eventGroupList[it->first].EventList) {
      Serial.printf("EVENT - %d :\r\n", eventCount);
      eventCount++;
      if (eventChose.PWM_MotorList) {
        for (auto it = eventChose.PWM_MotorList->begin(); it != eventChose.PWM_MotorList->end(); ++it) {
          Serial.println("PWM_MOTOR_STATUS_SET_OBJ\t: "+it->motorID+"\tto\t"+String(it->motortStatus));
        }
      } else if (eventChose.peristalticMotorList) {
        for (auto it = eventChose.peristalticMotorList->begin(); it != eventChose.peristalticMotorList->end(); ++it) {
          Serial.println("PERISTALTIC_STATUS_SET_OBJ\t: "+it->motorID+"\tto\t"+String(it->motortStatus)+" in "+String(it->activeTime) + " s");
        }
      } else if (eventChose.waitTime) {
        // vTaskDelay(eventChose.waitTime->waitTime);
      }
    }
  }
}

void SMachine_Ctrl::GetAllStepSetting()
{
  for (auto it = D_stepGroupList.begin(); it != D_stepGroupList.end(); ++it) {
    Serial.println("====================================================");
    Serial.println("Key:\t"+String(it->first.c_str()));
    Serial.println("Title:\t"+D_stepGroupList[it->first].Title);
    Serial.println("Desp:\t"+D_stepGroupList[it->first].Description);
    for (const auto& stepChose : D_stepGroupList[it->first].EventGroupNameList) {
      Serial.println("STEP\t: "+stepChose);
    }
  }
}

String SMachine_Ctrl::GetRunHistoryDetailString()
{
  String returnString;
  serializeJsonPretty(*RunHistoryItem, returnString);
  return returnString;
}

////////////////////////////////////////////////////
// For 事件執行
////////////////////////////////////////////////////

EVENT_RESULT SMachine_Ctrl::RUN__PWMMotorEvent()
{

}

////////////////////////////////////////////////////
// For 不間斷監聽
////////////////////////////////////////////////////

/**
 * @brief 伺服馬達控制監聽迴圈
 * 
 * @param parameter 
 */
void Task_SwitchMotorScan(void * parameter)
{
  ESP_LOGI("SMachine_Ctrl","Task_SwitchMotorScan Run");
  for (;;) {
    if (Machine_Ctrl.motorCtrl.active == MotorCtrlSteps::Active) {
      Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Running;

      for (auto it = Machine_Ctrl.motorCtrl.motorsDict.begin(); it != Machine_Ctrl.motorCtrl.motorsDict.end(); ++it) {
        if (Machine_Ctrl.motorCtrl.motorsDict[it->first].channelIndex == -1) {
          continue;
        }
        Machine_Ctrl.motorCtrl.MotorStatusChange(String(it->first.c_str()));
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

/**
 * @brief 蠕動馬達控制迴圈
 * 
 * @param parameter 
 */
void Task_PeristalticMotorScan(void * parameter)
{
  ESP_LOGI("SMachine_Ctrl","Task_PeristalticMotorScan Run");
  for (;;) {
    for (auto it = Machine_Ctrl.peristalticMotorsCtrl.motorsDict.begin(); it != Machine_Ctrl.peristalticMotorsCtrl.motorsDict.end(); ++it) {
      time_t nowTime = now();
      if (nowTime < it->second.motorEndTime) {
        if (it->second.motorNowStatus != it->second.motorNextStatus) {
          ESP_LOGI("Task_PeristalticMotorScan","Peristaltic Motors %s change status: %d -> %d",
            it->second.motorID.c_str(), it->second.motorNowStatus, it->second.motorNextStatus
          );
          it->second.motorNowStatus = it->second.motorNextStatus;
        }
      } else {
        if (it->second.motorNowStatus != MotorCtrlSteps::Idel) {
          ESP_LOGI("Task_PeristalticMotorScan","Peristaltic Motors %s stop", it->second.motorID.c_str());
          it->second.motorNowStatus  = MotorCtrlSteps::Idel;
          it->second.motorNextStatus = MotorCtrlSteps::Idel;
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

/**
 * @brief 獲得儀器基本資訊
 * 
 * @return String 
 */
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
    // json_doc["now_step_name"].set(
    //   // NowDeviceStatus.NowRunningEvent->eventGroup[NowDeviceStatus.NowStep].motorGroupEvent->Title
    // );
  }

  return json_doc;
}

////////////////////////////////////////////////////
// For 基礎行為
////////////////////////////////////////////////////

void SMachine_Ctrl::LoadNewSettings(String StepID, String TrigerBy)
{
  std::string stdStepID = std::string(StepID.c_str());
  if (D_stepGroupList.count(stdStepID) > 0) {
    RunHistoryItem->clear();
    JsonObject RunHistoryItem_ = RunHistoryItem->to<JsonObject>();
    RunHistoryItem_["step_id"] = StepID;
    RunHistoryItem_["triger_by"] = TrigerBy;
    time_t NOW = now();
    RunHistoryItem_["create_time"] = NOW;
    JsonArray enent_group_list = RunHistoryItem_.createNestedArray("enent_group_list");
    for (
      auto eventGroupName = D_stepGroupList[stdStepID].EventGroupNameList.begin(); 
      eventGroupName != D_stepGroupList[stdStepID].EventGroupNameList.end(); 
      ++eventGroupName  // 依 Event Name 跑迴圈
    ) {
      DynamicJsonDocument eventGroupItem(10000);
      std::string stdEventName = std::string(eventGroupName->c_str());
      eventGroupItem["event_group_id"] = String(stdEventName.c_str());
      eventGroupItem["active_time"] = -1;
      eventGroupItem["end_time"] = -1;

      JsonArray enent_list = eventGroupItem.createNestedArray("enent_list");
      for (auto& eventChose : D_eventGroupList[stdEventName].EventList) {
        DynamicJsonDocument eventItem(4000);
        eventItem["active_time"] = -1;
        eventItem["end_time"] = -1;
        if (eventChose.PWM_MotorList) {
          eventItem["event"] = "pwn_motor";
          JsonArray pwn_motor_list = eventItem.createNestedArray("pwn_motor_list");
          for (auto pwnMotorChose = eventChose.PWM_MotorList->begin(); pwnMotorChose != eventChose.PWM_MotorList->end(); ++pwnMotorChose) {
            DynamicJsonDocument pwnMotorItem(100);
            pwnMotorItem["pwm_motor_id"] = pwnMotorChose->motorID;
            pwnMotorItem["status"] = pwnMotorChose->motortStatus;
            pwn_motor_list.add(pwnMotorItem);
          }
        }
        else if (eventChose.peristalticMotorList) {
          eventItem["event"] = "peristaltic_motor";
          JsonArray peristaltic_motor_list = eventItem.createNestedArray("peristaltic_motor_list");
          for (auto peristalticMotorChose = eventChose.peristalticMotorList->begin(); peristalticMotorChose != eventChose.peristalticMotorList->end(); ++peristalticMotorChose) {
            DynamicJsonDocument peristalticMotorItem(100);
            peristalticMotorItem["pwm_motor_id"] = peristalticMotorChose->motorID;
            peristalticMotorItem["status"] = peristalticMotorChose->motortStatus;
            peristalticMotorItem["time"] = peristalticMotorChose->activeTime;
            peristaltic_motor_list.add(peristalticMotorItem);
          }
        }
        else if (eventChose.waitTime) {
          eventItem["event"] = "wait";
          eventItem["wait"] = eventChose.waitTime->waitTime;
        }
        enent_list.add(eventItem);
      }

     
      enent_group_list.add(eventGroupItem);
    }
  }
}


////////////////////////////////////////////////////
// For 組合行為
////////////////////////////////////////////////////

void SMachine_Ctrl::RUN_EventGroup(String EVENT_GROUP_NAME)
{
  ESP_LOGI("RUN_EVENT_GROUP","執行事件:\t%s", EVENT_GROUP_NAME.c_str());
  if (D_eventGroupList.count(std::string(EVENT_GROUP_NAME.c_str())) > 0){
    std::string eventGroupName = std::string(EVENT_GROUP_NAME.c_str());
    ESP_LOGI("RUN_EVENT_GROUP","事件說明:\t%s", D_eventGroupList[eventGroupName].Description.c_str());
    int eventCount = 1;
    for (auto& eventItem : D_eventGroupList[eventGroupName].EventList) {
      ESP_LOGI("RUN_EVENT_GROUP","EVENT:\t%d", eventCount);
      eventCount++;
      if (eventItem.PWM_MotorList) {
        for (auto it = eventItem.PWM_MotorList->begin(); it != eventItem.PWM_MotorList->end(); ++it) {
          Machine_Ctrl.motorCtrl.SetMotorTo(it->motorID, it->motortStatus);
        }
        Machine_Ctrl.motorCtrl.active = MotorCtrlSteps::Active;
        while (Machine_Ctrl.motorCtrl.active != MotorCtrlSteps::Idel) {
          vTaskDelay(10);
        }
      }
      else if (eventItem.peristalticMotorList) {
        for (auto it = eventItem.peristalticMotorList->begin(); it != eventItem.peristalticMotorList->end(); ++it) {
          Machine_Ctrl.peristalticMotorsCtrl.RunMotor(it->motorID, it->motortStatus, it->activeTime);
          vTaskDelay(it->activeTime*1000+500);
        }
      } else if (eventItem.waitTime) {
        vTaskDelay(eventItem.waitTime->waitTime);
      }
    }

    ESP_LOGI("RUN_EVENT_GROUP","事件:\t%s\t執行完畢",eventGroupName.c_str());
  } else {
    ESP_LOGE("RUN_EVENT_GROUP","事件:\t%s\t不存在", EVENT_GROUP_NAME.c_str());
  }
}

void SMachine_Ctrl::RUN_Step(String STEP_NAME){
  std::string stepName = std::string(STEP_NAME.c_str());
  ESP_LOGI("RUN_STEP","執行流程:\t%s", stepName.c_str());
  if (D_stepGroupList.count(stepName) > 0){
    ESP_LOGI("RUN_STEP","流程說明:\t%s", D_stepGroupList[stepName].Description.c_str());
    for (auto& eventGroupName : D_stepGroupList[stepName].EventGroupNameList) {
      RUN_EventGroup(eventGroupName);
    }
    ESP_LOGI("RUN_STEP","流程:\t%s\t執行完畢",stepName.c_str());
  } else {
    ESP_LOGE("RUN_STEP","流程:\t%s\t不存在", stepName.c_str());
  }
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