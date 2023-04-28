#include "Machine_Ctrl.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"

#include <TimeLib.h>   

#include <Pools_Ctrl.h>
#include <Machine_Base_info.h>
#include <Motor_Ctrl.h>
#include <vector>
#include <variant>
#include <functional>

TaskHandle_t TASK_SwitchMotorScan = NULL;
TaskHandle_t TASK_PeristalticMotorScan = NULL;

// extern AsyncWebSocket ws;

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
  // JsonObject obj = spiffs.DeviceSetting->as<JsonObject>();
  // UpdatePWMMotorSetting(obj["pwm_motor"]);
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

void SMachine_Ctrl::PrintAllPWNMotorSetting()
{
  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  if (DeviceSetting.containsKey("pwm_motor")) {
    JsonObject D_pwmMotorSetting = DeviceSetting["pwm_motor"];
    ESP_LOGI("INFO", "====================================================");
    int pwmMotorCount = 1;
    for (JsonPair pwmMotorItem : D_pwmMotorSetting) {
      ESP_LOGI("INFO", "%02d.\t伺服馬達ID:\t%s", pwmMotorCount, pwmMotorItem.key().c_str());
      ESP_LOGI("INFO", " |\t伺服馬達控制編號:\t%d", pwmMotorItem.value()["index"].as<int>());
      ESP_LOGI("INFO", " |\t伺服馬達名稱:\t%s", pwmMotorItem.value()["title"].as<String>().c_str());
      ESP_LOGI("INFO", " |\t伺服馬達說明:\t%s", pwmMotorItem.value()["description"].as<String>().c_str());
      pwmMotorCount++;
    }
  }
}

void SMachine_Ctrl::PrintAllPeristalticMotorSetting()
{
  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  if (DeviceSetting.containsKey("peristaltic_motor")) {
    JsonObject D_PeristalticMotorSetting = DeviceSetting["peristaltic_motor"];
    ESP_LOGI("INFO", "====================================================");
    int peristalticMotorCount = 1;
    for (JsonPair peristalticMotorItem : D_PeristalticMotorSetting) {
      ESP_LOGI("INFO", "%02d.\t蠕動馬達ID:\t%s", peristalticMotorCount, peristalticMotorItem.key().c_str());
      ESP_LOGI("INFO", " |\t蠕動馬達控制編號:\t%d", peristalticMotorItem.value()["index"].as<int>());
      ESP_LOGI("INFO", " |\t蠕動馬達名稱:\t%s", peristalticMotorItem.value()["title"].as<String>().c_str());
      ESP_LOGI("INFO", " |\t蠕動馬達說明:\t%s", peristalticMotorItem.value()["description"].as<String>().c_str());
      peristalticMotorCount++;
    }
  }
}

void SMachine_Ctrl::PrintAllEventSetting()
{
  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  if (DeviceSetting.containsKey("event_group")) {
    JsonObject D_eventGroup = DeviceSetting["event_group"];
    for (JsonPair eventItem : D_eventGroup) {
      ESP_LOGI("INFO", "====================================================");
      ESP_LOGI("INFO", "Event Group ID:\t%s", eventItem.key().c_str());
      ESP_LOGI("INFO", "Title:\t%s", eventItem.value()["title"].as<String>().c_str());
      ESP_LOGI("INFO", "Desp:\t%s", eventItem.value()["description"].as<String>().c_str());
      ESP_LOGI("INFO", "Events:");
      int eventCount = 1;
      for (JsonVariant event : eventItem.value()["event"].as<JsonArray>()) {
        if (event.containsKey("pwm_motor_list")) {
          ESP_LOGI("INFO", "\t%d.\t伺服馬達控制", eventCount);
          for (JsonVariant event : event["pwm_motor_list"].as<JsonArray>()) {
            ESP_LOGI("INFO", "\t|\t - %s -> %03d", event["pwm_motor_id"].as<String>().c_str(), event["status"].as<int>());
          }
        } else if (event.containsKey("peristaltic_motor_list")) {
          ESP_LOGI("INFO", "\t%d.\t蠕動馬達控制", eventCount);
          for (JsonVariant event : event["peristaltic_motor_list"].as<JsonArray>()) {
            ESP_LOGI(
              "INFO", "\t|\t - %s -> %d in %d seconds", 
              event["peristaltic_motor_id"].as<String>().c_str(), 
              event["status"].as<int>(),
              event["time"].as<int>()
            );
          }
        } else if (event.containsKey("wait")) {
          ESP_LOGI("INFO", "\t%d.\t等待時間: %d 秒", eventCount, event["wait"].as<int>());
        }
        eventCount++;
      }
    }
  }
}

void SMachine_Ctrl::PrintAllStepSetting()
{
  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  if (DeviceSetting.containsKey("steps_group")) {
    JsonObject D_stepGroup = DeviceSetting["steps_group"];
    for (JsonPair stepItem : D_stepGroup) {
      ESP_LOGI("INFO", "====================================================");
      ESP_LOGI("INFO", "Step Group ID:\t%s", stepItem.key().c_str());
      ESP_LOGI("INFO", "Title:\t%s", stepItem.value()["title"].as<String>().c_str());
      ESP_LOGI("INFO", "Desp:\t%s", stepItem.value()["description"].as<String>().c_str());
      JsonArray L_steps = stepItem.value()["steps"].as<JsonArray>();
      ESP_LOGI("INFO", "Steps:");
      int stepCount = 1;
      for (JsonVariant step : L_steps) {
        ESP_LOGI("INFO", "\t%d.\t%s", stepCount, step.as<String>().c_str());
        stepCount++;
      }
    }
  }
}


////////////////////////////////////////////////////
// For 數值轉換
////////////////////////////////////////////////////  

int SMachine_Ctrl::pwmMotorIDToMotorIndex(String motorID)
{
  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  if (DeviceSetting["pwm_motor"].containsKey(motorID)) {
    return DeviceSetting["pwm_motor"][motorID]["index"];
  } else {
    return -1;
  }
}

int SMachine_Ctrl::PeristalticMotorIDToMotorIndex(String motorID)
{
  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  if (DeviceSetting["peristaltic_motor"].containsKey(motorID)) {
    return DeviceSetting["peristaltic_motor"][motorID]["index"];
  } else {
    return -1;
  }
}

////////////////////////////////////////////////////
// For 事件執行
////////////////////////////////////////////////////
void PWMMotorEvent(void* parameter)
{
  // for(;;){
  ESP_LOGW("PWMMotorEvent","START");
  JsonArray PWMMotorEventList = *((JsonArray*) parameter);
  for (JsonVariant event : PWMMotorEventList) {
    int MotorIndex = Machine_Ctrl.pwmMotorIDToMotorIndex(event["pwm_motor_id"].as<String>());
    if (MotorIndex > -1) {
      Machine_Ctrl.motorCtrl.SetMotorTo(MotorIndex, event["status"].as<int>());
      vTaskDelay(100);
    }
  }
  vTaskDelay(2500);
  ESP_LOGW("PWMMotorEvent","END");
  Machine_Ctrl.TASK__PWM_MOTOR = NULL;
  vTaskDelete(NULL);
    // vTaskDelete(Machine_Ctrl.TASK__PWM_MOTOR);
  // }
}
EVENT_RESULT SMachine_Ctrl::RUN__PWMMotorEvent(JsonArray PWMMotorEventList)
{
  EVENT_RESULT returnData;
  eTaskState taskState = eTaskGetState(&TASK__PWM_MOTOR);
  returnData.status = taskState;
  if (TASK__PWM_MOTOR == NULL | taskState == eDeleted) {
    xTaskCreate(
      PWMMotorEvent, "PWM_MOTOR_RUN",
      10000, &PWMMotorEventList, 1, &TASK__PWM_MOTOR
    );
    returnData.message = "OK";
  } else {
    switch (taskState) {
      case eRunning:
        returnData.message = "eRunning";ESP_LOGW("PWMMotorEvent","eRunning");break;
      case eBlocked: // 任務正在等待某些事件的發生
        returnData.message = "eBlocked";ESP_LOGW("PWMMotorEvent","eBlocked");break;
      case eSuspended: // 任務已經被暫停
        returnData.message = "eSuspended";ESP_LOGW("PWMMotorEvent","eSuspended");break;
      default: // 未知的狀態
        returnData.message = "unknow";ESP_LOGW("PWMMotorEvent","unknow");break;
    }
  } 
  return returnData;
}

void PeristalticMotorEvent(void* parameter)
{
  ESP_LOGW("PeristalticMotorEvent","START");
  JsonArray PeristalticMotorEventList = *((JsonArray*) parameter);
  for (JsonVariant event : PeristalticMotorEventList) {
    int MotorIndex = Machine_Ctrl.PeristalticMotorIDToMotorIndex(event["peristaltic_motor_id"].as<String>());
    if (MotorIndex > -1) {
      int totalTime = event["time"].as<int>()*1000;
      Machine_Ctrl.TaskSuspendTimeSum = 0;
      TickType_t startTick = xTaskGetTickCount();
      while ((xTaskGetTickCount() - startTick - Machine_Ctrl.TaskSuspendTimeSum) * portTICK_PERIOD_MS <= totalTime) {
        // ESP_LOGW("PeristalticMotorEvent","START");
        Machine_Ctrl.peristalticMotorsCtrl.RunMotor(MotorIndex, event["status"].as<int>());
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      ESP_LOGW("PeristalticMotorEvent","loop time: %d", (xTaskGetTickCount() - startTick - Machine_Ctrl.TaskSuspendTimeSum) * portTICK_PERIOD_MS / 1000);
      Machine_Ctrl.peristalticMotorsCtrl.RunMotor(MotorIndex, 0);
    }
  }
  ESP_LOGW("PeristalticMotorEvent","END");
  Machine_Ctrl.TASK__Peristaltic_MOTOR = NULL;
  vTaskDelete(NULL);
}
EVENT_RESULT SMachine_Ctrl::RUN__PeristalticMotorEvent(JsonArray PeristalticMotorEventList)
{
  EVENT_RESULT returnData;
  eTaskState taskState = eTaskGetState(&TASK__Peristaltic_MOTOR);
  returnData.status = taskState;
  if (TASK__Peristaltic_MOTOR == NULL | taskState == eDeleted) {
    ESP_LOGW("Build_SwitchMotorScan","START");
    xTaskCreate(
      PeristalticMotorEvent, "MOTOR_RUN",
      10000, &PeristalticMotorEventList, 1, &TASK__Peristaltic_MOTOR
    );
    returnData.message = "OK";
  } else {
    switch (taskState) {
      case eRunning:
        returnData.message = "eRunning";ESP_LOGW("Build_SwitchMotorScan","eRunning");break;
      case eReady: // 任務已經被排程但還沒有執行
        returnData.message = "eReady";ESP_LOGW("Build_SwitchMotorScan","eReady");break;
      case eBlocked: // 任務正在等待某些事件的發生
        returnData.message = "eBlocked";ESP_LOGW("Build_SwitchMotorScan","eBlocked");break;
      case eSuspended: // 任務已經被暫停
        returnData.message = "eSuspended";ESP_LOGW("Build_SwitchMotorScan","eSuspended");break;
      default: // 未知的狀態
        returnData.message = "unknow";ESP_LOGW("Build_SwitchMotorScan","unknow");break;
    }
  } 
  return returnData;
}


void wsSendStepStatus() 
{
  DynamicJsonDocument json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData("RunHistory");
  json_doc["parameter"]["message"].set("update");
  String returnString;
  serializeJsonPretty(json_doc, returnString);
  Machine_Ctrl.BackendServer.ws_->textAll(returnString);
}

void RunHistory(void* parameter)
{
  JsonObject DeviceSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>();
  JsonObject D_stepGroups = DeviceSetting["steps_group"];
  JsonObject D_eventGroup =  DeviceSetting["event_group"];
  if (DeviceSetting.containsKey("run_history")) {
    JsonObject D_history =  DeviceSetting["run_history"];
    String S_step_id = D_history["step_id"].as<String>();
    ESP_LOGI("HISTORY","執行流程::\t%s", S_step_id.c_str());
    ESP_LOGI("HISTORY","觸發者:\t%s", D_history["triger_by"].as<String>().c_str());
    time_t NOW = now();
    if (D_history["first_active_time"].as<int>() == -1) {
      D_history["first_active_time"] = NOW;
    }
    D_history["last_active_time"] = NOW;
    Machine_Ctrl.spiffs.ReWriteDeviceSetting();
    ////////////
    wsSendStepStatus();
    ////////////
    if (D_stepGroups.containsKey(S_step_id)) {
      int eventGroupCount = 0;
      for (JsonVariant D_eventGroupRunHistory : D_history["enent_group_list"].as<JsonArray>()) {
        eventGroupCount++;
        String S_eventGroupID = D_eventGroupRunHistory["event_group_id"].as<String>();
        ESP_LOGI("HISTORY", " - 步驟 %02d:\t%s", eventGroupCount, S_eventGroupID.c_str());
        if (D_eventGroup.containsKey(S_eventGroupID)) {
          JsonObject D_eventGroupSetting = D_eventGroup[S_eventGroupID];
          ESP_LOGI("HISTORY", " - 標題:\t%s", D_eventGroupSetting["title"].as<String>().c_str());
          ESP_LOGI("HISTORY", " - 說明:\t%s", D_eventGroupSetting["description"].as<String>().c_str());
          
          time_t endTime = D_eventGroupRunHistory["end_time"].as<time_t>();
          if (endTime != -1) {
            ESP_LOGI("HISTORY", " - 步驟 %02d %s 已在 %04d/%02d/%02d %02d:%02d:%02d 執行完畢，跳過", 
              eventGroupCount, S_eventGroupID.c_str(),
              year(endTime), month(endTime), day(endTime), hour(endTime), minute(endTime), second(endTime)
            );
            continue;
          }

          if (D_eventGroupRunHistory["active_time"].as<time_t>() == -1) {
            D_eventGroupRunHistory["active_time"] = now();
            Machine_Ctrl.spiffs.ReWriteDeviceSetting();
            ////////////
            wsSendStepStatus();
            ////////////
          }
          JsonArray L_eventRunHistory = D_eventGroupRunHistory["enent_list"].as<JsonArray>();
          int eventCount = -1;
          for (JsonVariant D_eventSettingChose : D_eventGroupSetting["event"].as<JsonArray>()) {
            eventCount++;
            ESP_LOGI("HISTORY", "   * 執行步驟 %d-%d", eventGroupCount, eventCount+1);
            ////////////
            wsSendStepStatus();
            ////////////
            time_t evenEndTime = L_eventRunHistory[eventCount]["end_time"].as<time_t>();
            if (evenEndTime != -1) {
              ESP_LOGI("HISTORY", "   * 步驟 %d-%d 已在 %04d/%02d/%02d %02d:%02d:%02d 執行完畢，跳過", 
                eventGroupCount, eventCount+1,
                year(evenEndTime), month(evenEndTime), day(evenEndTime), hour(evenEndTime), minute(evenEndTime), second(evenEndTime)
              );
              continue;
            }
            if (L_eventRunHistory[eventCount]["active_time"].as<int>() == -1) {
              L_eventRunHistory[eventCount]["active_time"] = now();
              Machine_Ctrl.spiffs.ReWriteDeviceSetting();
              ////////////
              wsSendStepStatus();
              ////////////
            }
            if (D_eventSettingChose.containsKey("pwm_motor_list")) {
              JsonArray L_pwmMotorSettingList = D_eventSettingChose["pwm_motor_list"].as<JsonArray>();
              while (Machine_Ctrl.TASK__PWM_MOTOR != NULL) {
                vTaskDelay(100);
              }
              Machine_Ctrl.RUN__PWMMotorEvent(L_pwmMotorSettingList);

              while (Machine_Ctrl.TASK__PWM_MOTOR != NULL) {
                vTaskDelay(100);
              }
            } else if (D_eventSettingChose.containsKey("peristaltic_motor_list")) {
              JsonArray L_peristalticMotorSettingList = D_eventSettingChose["peristaltic_motor_list"].as<JsonArray>();
              while (Machine_Ctrl.TASK__Peristaltic_MOTOR != NULL) {
                vTaskDelay(100);
              }

              Machine_Ctrl.RUN__PeristalticMotorEvent(L_peristalticMotorSettingList);
              
              while (Machine_Ctrl.TASK__Peristaltic_MOTOR != NULL) {
                vTaskDelay(100);
              }
            } else if (D_eventSettingChose.containsKey("wait")) {
              
            }
            L_eventRunHistory[eventCount]["end_time"] = now();
            Machine_Ctrl.spiffs.ReWriteDeviceSetting();
            ////////////
            wsSendStepStatus();
            ////////////
            ESP_LOGI("HISTORY", "   * 執行步驟 %d-%d 完畢，耗時 %d 秒", eventGroupCount, eventCount+1, 
              L_eventRunHistory[eventCount]["end_time"].as<time_t>() - L_eventRunHistory[eventCount]["active_time"].as<time_t>()
            );
          }
          
          D_eventGroupRunHistory["end_time"] = now();
          Machine_Ctrl.spiffs.ReWriteDeviceSetting();
          ////////////
          DynamicJsonDocument json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData("RunHistory");
          json_doc["parameter"]["message"].set("update");
          String returnString;
          serializeJsonPretty(json_doc, returnString);
          Machine_Ctrl.BackendServer.ws_->textAll(returnString);
          ////////////
          ESP_LOGI("HISTORY", " - 執行步驟 %02d %s 完畢，耗時 %d 秒", eventGroupCount, S_eventGroupID.c_str(), 
            D_eventGroupRunHistory["end_time"].as<time_t>() - D_eventGroupRunHistory["active_time"].as<time_t>()
          );
        } else {
          ESP_LOGE("HISTORY","找不到事件組設定 :\t%s", S_eventGroupID.c_str());
        }
      }

      D_history["end_time"] = now();
      Machine_Ctrl.spiffs.ReWriteDeviceSetting();
      ////////////
      DynamicJsonDocument json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData("RunHistory");
      json_doc["parameter"]["message"].set("update");
      String returnString;
      serializeJsonPretty(json_doc, returnString);
      Machine_Ctrl.BackendServer.ws_->textAll(returnString);
      ////////////
      ESP_LOGI("HISTORY","執行流程:\t%s 完畢", S_step_id.c_str());
      ESP_LOGI("HISTORY","耗時總時長:\t%d 秒", D_history["end_time"].as<time_t>() - D_history["first_active_time"].as<time_t>());

    } else {
      ESP_LOGE("HISTORY","找不到設定 step_id:\t%s", S_step_id.c_str());
    }
    // String printString;
    // serializeJsonPretty(D_history, printString);
    // Serial.println(printString);
  } else {
    ESP_LOGE("HISTORY","找不到歷史紀錄");
  }
  Machine_Ctrl.TASK__History = NULL;
  vTaskDelete(NULL);
}
EVENT_RESULT SMachine_Ctrl::RUN__History() {
  EVENT_RESULT returnData;
  eTaskState taskState = eTaskGetState(&TASK__History);
  returnData.status = taskState;
  if (TASK__History == NULL | taskState == eDeleted) {
    ESP_LOGW("RUN__History","START");
    xTaskCreate(
      RunHistory, "TASK_RUN",
      10000, NULL, 1, &TASK__History
    );
    returnData.message = "OK";
  } else {
    switch (taskState) {
      case eRunning:
        returnData.message = "eRunning";ESP_LOGW("Build_SwitchMotorScan","eRunning");break;
      case eReady: // 任務已經被排程但還沒有執行
        returnData.message = "eReady";ESP_LOGW("Build_SwitchMotorScan","eReady");break;
      case eBlocked: // 任務正在等待某些事件的發生
        returnData.message = "eBlocked";ESP_LOGW("Build_SwitchMotorScan","eBlocked");break;
      case eSuspended: // 任務已經被暫停
        returnData.message = "eSuspended";ESP_LOGW("Build_SwitchMotorScan","eSuspended");break;
      default: // 未知的狀態
        returnData.message = "unknow";ESP_LOGW("Build_SwitchMotorScan","unknow");break;
    }
  } 
  return returnData;
}

void SMachine_Ctrl::STOP_AllTask() 
{
  if (TASK__History != NULL) {
    ESP_LOGW("STOP_AllTask","STOP TASK__History");
    vTaskSuspend(TASK__History);
  }
  if (TASK__PWM_MOTOR != NULL) {
    ESP_LOGW("STOP_AllTask","STOP TASK__PWM_MOTOR");
    vTaskSuspend(TASK__PWM_MOTOR);
  }
  if (TASK__Peristaltic_MOTOR != NULL) {
    ESP_LOGW("STOP_AllTask","STOP TASK__Peristaltic_MOTOR");
    vTaskSuspend(TASK__Peristaltic_MOTOR);
    LastSuspendTick = xTaskGetTickCount();
    Stop_AllPeristalticMotor();
  }
}

void SMachine_Ctrl::RESUME_AllTask()
{
  if (TASK__Peristaltic_MOTOR != NULL) {
    ESP_LOGW("RESUME_AllTask","Resume TASK__Peristaltic_MOTOR");
    TaskSuspendTimeSum += xTaskGetTickCount() - LastSuspendTick;
    vTaskResume(TASK__Peristaltic_MOTOR);
  }
  if (TASK__PWM_MOTOR != NULL) {
    ESP_LOGW("RESUME_AllTask","Resume TASK__PWM_MOTOR");
    vTaskResume(TASK__PWM_MOTOR);
  }
  if (TASK__History != NULL) {
    ESP_LOGW("RESUME_AllTask","Resume TASK__History");
    vTaskResume(TASK__History);
  }
}

void SMachine_Ctrl::Stop_AllPeristalticMotor()
{
  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  if (DeviceSetting.containsKey("peristaltic_motor")) {
    for (JsonPair peristalticMotorItem : DeviceSetting["peristaltic_motor"].as<JsonObject>()) {
      peristalticMotorsCtrl.RunMotor(peristalticMotorItem.value()["index"].as<int>(),0);
    }
  }
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
  JsonObject DeviceSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>();
  if (NowDeviceStatus.deviceStatusCode == DeviceStatusCode::device_idel) {
    json_doc["device_status"].set("Idel");
    json_doc["run_history"].set(DeviceSetting["run_history"]);
  } 
  else if (NowDeviceStatus.deviceStatusCode == DeviceStatusCode::device_stop) {
    json_doc["device_status"].set("TaskSuspend");
    json_doc["run_history"].set(DeviceSetting["run_history"]);
  }
  else {
    json_doc["device_status"].set("Busy");
    json_doc["run_history"].set(DeviceSetting["run_history"]);
  }

  return json_doc;
}

////////////////////////////////////////////////////
// For 基礎行為
////////////////////////////////////////////////////

void SMachine_Ctrl::LoadStepToRunHistoryItem(String StepID, String TrigerBy)
{
  while (Machine_Ctrl.TASK__History != NULL) {
    return;
  }

  JsonObject DeviceSetting = spiffs.DeviceSetting->as<JsonObject>();
  JsonObject D_stepGroups = DeviceSetting["steps_group"];
  JsonObject D_eventGroup =  DeviceSetting["event_group"];

  DynamicJsonDocument D_runHistory(10000);
  if (D_stepGroups.containsKey(StepID)) {
    JsonObject D_stepGroupChose = D_stepGroups[StepID];
    D_runHistory["step_id"] = StepID;
    D_runHistory["triger_by"] = TrigerBy;
    time_t NOW = now();
    D_runHistory["create_time"] = NOW;
    D_runHistory["first_active_time"] = -1;
    D_runHistory["last_active_time"] = -1;
    D_runHistory["end_time"] = -1;

    JsonArray L_steps = D_stepGroupChose["steps"].as<JsonArray>();
    JsonArray enent_group_list = D_runHistory.createNestedArray("enent_group_list");
    for (JsonVariant step : L_steps) {
      DynamicJsonDocument eventGroupItem(10000);
      eventGroupItem["event_group_id"] = step.as<String>();
      eventGroupItem["active_time"] = -1;
      eventGroupItem["end_time"] = -1;
      
      JsonArray enent_list = eventGroupItem.createNestedArray("enent_list");
      JsonObject D_eventGroupChose = D_eventGroup[step.as<String>()];
      for (JsonVariant event : D_eventGroupChose["event"].as<JsonArray>()) {
        DynamicJsonDocument eventItem(1000);
        eventItem["active_time"] = -1;
        eventItem["end_time"] = -1;
        enent_list.add(eventItem);
      }
      enent_group_list.add(eventGroupItem);
    }
  } else {
    ESP_LOGE("INFO","流程:\t%s\t不存在", StepID.c_str());
  }
  DeviceSetting["run_history"].set(D_runHistory);
  spiffs.ReWriteDeviceSetting();
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
          // Machine_Ctrl.peristalticMotorsCtrl.RunMotor(it->motorID, it->motortStatus, it->activeTime);
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