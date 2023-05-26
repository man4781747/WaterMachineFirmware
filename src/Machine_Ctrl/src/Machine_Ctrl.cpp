#include "Machine_Ctrl.h"
#include <esp_log.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include <ESPAsyncWebServer.h>
// #include <AsyncWebServer_ESP32_W5500.h>
#include "AsyncTCP.h"

#include <TimeLib.h>   

#include <Pools_Ctrl.h>
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
  spiffs.LoadDeviceBaseInfo();
  spiffs.GetDeviceSetting();
  spiffs.LoadWiFiConfig();
}

void SMachine_Ctrl::INIT_I2C_Wires()
{
  WireOne.begin(WireOne_SDA, WireOne_SCL);
}

////////////////////////////////////////////////////
// For 更新設定
////////////////////////////////////////////////////


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

typedef struct {
  String message;
} TaskParameters;

void SMachine_Ctrl::LOAD__ACTION(String actionJSONString)
{
  loadedAction->clear();
  DeserializationError error = deserializeJson(*loadedAction, actionJSONString,  DeserializationOption::NestingLimit(20));
  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
  }
}


void LOADED_ACTION(void* parameter)
{ 
  //* Pipiline開始
  ESP_LOGI("LOADED_ACTION","START");
  JsonObject D_loadedActionJSON = Machine_Ctrl.loadedAction->as<JsonObject>();
  String poolID = D_loadedActionJSON["pool"].as<String>();
  ESP_LOGI("LOADED_ACTION","執行流程: %s", D_loadedActionJSON["title"].as<String>().c_str());
  ESP_LOGI("LOADED_ACTION","流程說明: %s", D_loadedActionJSON["description"].as<String>().c_str());
  ESP_LOGI("LOADED_ACTION","蝦池ID: %s", poolID.c_str());
  ESP_LOGI("LOADED_ACTION","是否為測試: %s", D_loadedActionJSON["data_type"].as<String>() == String("TEST")? "是" : "否");
  DynamicJsonDocument poolSensorData(30000);
  bool AnySensorData = false;
  int stepCount = 1;
  for (JsonVariant stepItem : D_loadedActionJSON["step_list"].as<JsonArray>()) {
    //* 個別Step執行
    JsonObject D_stepItem = stepItem.as<JsonObject>();
    for (JsonPair D_stepItem_ : D_stepItem) {
      ESP_LOGI("LOADED_ACTION","  [%d]%s - %s",
        stepCount, 
        D_stepItem_.value()["title"].as<String>().c_str(),
        D_stepItem_.value()["description"].as<String>().c_str()
      );
      int eventGroupCount = 1;
      for (JsonVariant eventGroupItem : D_stepItem_.value()["event_group_list"].as<JsonArray>()) {
        JsonObject D_eventGroupItem = eventGroupItem.as<JsonObject>();
        for (JsonPair D_eventGroupItem_ : D_eventGroupItem) {
          ESP_LOGI("LOADED_ACTION","    [%d-%d]%s - %s", 
            stepCount, eventGroupCount, 
            D_eventGroupItem_.value()["title"].as<String>().c_str(),
            D_eventGroupItem_.value()["description"].as<String>().c_str()
          );
          int eventCount = 1;
          JsonObject D_eventGroupItemDetail = D_eventGroupItem_.value().as<JsonObject>();
          for (JsonVariant eventItem : D_eventGroupItemDetail["event_list"].as<JsonArray>()) {
            // time_t = now();
            if (eventItem.containsKey("pwm_motor_list")) {
              ESP_LOGI("LOADED_ACTION","      [%d-%d-%d]伺服馬達控制",stepCount,eventGroupCount,eventCount);
              for (JsonVariant pwmMotorItem : eventItem["pwm_motor_list"].as<JsonArray>()) {
                ESP_LOGI("LOADED_ACTION","       - %s(%d) 轉至 %d 度", 
                  pwmMotorItem["pwn_motor"]["title"].as<String>().c_str(), pwmMotorItem["pwn_motor"]["index"].as<int>(), 
                  pwmMotorItem["status"].as<int>()
                );
                Machine_Ctrl.motorCtrl.SetMotorTo(pwmMotorItem["pwn_motor"]["index"].as<int>(), pwmMotorItem["status"].as<int>());
                pwmMotorItem["finish_time"].set(now());
              }
              vTaskDelay(2000/portTICK_PERIOD_MS);
            }

            else if (eventItem.containsKey("peristaltic_motor_list")) {
              ESP_LOGI("LOADED_ACTION","      [%d-%d-%d]蠕動馬達控制",stepCount,eventGroupCount,eventCount);
              for (JsonVariant peristalticMotorItem : eventItem["peristaltic_motor_list"].as<JsonArray>()) {
                ESP_LOGI("LOADED_ACTION","       - %s(%d) %s 持續 %.2f 秒", 
                  peristalticMotorItem["peristaltic_motor"]["title"].as<String>().c_str(), 
                  peristalticMotorItem["peristaltic_motor"]["index"].as<int>(), 
                  peristalticMotorItem["status"].as<int>()==-1 ? "正轉" : "反轉", peristalticMotorItem["time"].as<float>()
                );

                Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(
                  peristalticMotorItem["peristaltic_motor"]["index"].as<int>(), 
                  peristalticMotorItem["status"].as<int>() == 1 ? PeristalticMotorStatus::FORWARD : PeristalticMotorStatus::REVERSR
                );
                Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
                vTaskDelay(peristalticMotorItem["time"].as<float>()*1000/portTICK_PERIOD_MS);
                Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
                peristalticMotorItem["finish_time"].set(now());
              }
            }

            else if (eventItem.containsKey("spectrophotometer_list")) {
              ESP_LOGI("LOADED_ACTION","      [%d-%d-%d]分光光度計控制",stepCount,eventGroupCount,eventCount);
              for (JsonVariant spectrophotometerItem : eventItem["spectrophotometer_list"].as<JsonArray>()) {
                int spectrophotometerIndex = spectrophotometerItem["spectrophotometer"]["index"].as<int>();
                String GainStr = spectrophotometerItem["gain"].as<String>();
                String value_name = spectrophotometerItem["value_name"].as<String>();
                ESP_LOGI("LOADED_ACTION","       - %s(%d) %s 測量倍率，並紀錄為: %s", 
                  spectrophotometerItem["spectrophotometer"]["title"].as<String>().c_str(), 
                  spectrophotometerIndex, 
                  GainStr.c_str(), value_name.c_str()
                );
                Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(spectrophotometerIndex);
                if (GainStr == "1X") {
                  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_1X);
                }
                else if (GainStr == "2X") {
                  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_2X);
                }
                else if (GainStr == "4X") {
                  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_4X);
                }
                else if (GainStr == "8X") {
                  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_8X);
                }
                else if (GainStr == "48X") {
                  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_48X);
                }
                else if (GainStr == "96X") {
                  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
                }
                ALS_01_Data_t testValue = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
                time_t nowTime = now();
                spectrophotometerItem["finish_time"].set(now());
                spectrophotometerItem["measurement_time"].set(now());
                poolSensorData[poolID][value_name]["Gain"].set(GainStr);
                poolSensorData[poolID][value_name]["Value"]["CH0"].set(testValue.CH_0);
                poolSensorData[poolID][value_name]["Value"]["CH1"].set(testValue.CH_1);
                char datetimeChar[30];
                sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
                  year(nowTime), month(nowTime), day(nowTime),
                  hour(nowTime), minute(nowTime), second(nowTime)
                );
                poolSensorData[poolID][value_name]["Time"].set(datetimeChar);
                AnySensorData = true;
              }
            }

            JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("Auto").as<JsonObject>();
            D_baseInfoJSON["status"].set("OK");
            D_baseInfoJSON["action"]["target"].set("RunHistory");
            D_baseInfoJSON["action"]["method"].set("Update");
            D_baseInfoJSON["parameter"]["step"] = stepCount;
            D_baseInfoJSON["parameter"]["eventGroup"] = eventGroupCount;
            D_baseInfoJSON["parameter"]["event"] = eventCount;
            D_baseInfoJSON["parameter"]["status"] = "DONE";
            // D_baseInfoJSON["parameter"].set(D_loadedActionJSON);
            String returnString;
            serializeJsonPretty(D_baseInfoJSON, returnString);
            Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
            eventCount += 1;
            vTaskDelay(100/portTICK_PERIOD_MS);
          }
          D_eventGroupItemDetail["finish_time"].set(now());
          JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("Auto").as<JsonObject>();
          D_baseInfoJSON["status"].set("OK");
          D_baseInfoJSON["action"]["target"].set("RunHistory");
          D_baseInfoJSON["action"]["method"].set("Update");
          D_baseInfoJSON["parameter"]["step"] = stepCount;
          D_baseInfoJSON["parameter"]["eventGroup"] = eventGroupCount;
          D_baseInfoJSON["parameter"]["event"] = -1;
          D_baseInfoJSON["parameter"]["status"] = "DONE";
          // D_baseInfoJSON["parameter"].set(D_loadedActionJSON);
          String returnString;
          serializeJsonPretty(D_baseInfoJSON, returnString);
          Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
          vTaskDelay(100/portTICK_PERIOD_MS);
        }
        eventGroupCount+=1;
      }
      D_stepItem_.value()["finish_time"].set(now());
      JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("Auto").as<JsonObject>();
      D_baseInfoJSON["status"].set("OK");
      D_baseInfoJSON["action"]["target"].set("RunHistory");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"]["step"] = stepCount;
      D_baseInfoJSON["parameter"]["eventGroup"] = -1;
      D_baseInfoJSON["parameter"]["event"] = -1;
      D_baseInfoJSON["parameter"]["status"] = "DONE";
      // D_baseInfoJSON["parameter"].set(D_loadedActionJSON);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
    stepCount+=1;
  }
  ESP_LOGI("LOADED_ACTION","END");
  D_loadedActionJSON["finish_time"].set(now());
  // JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("Auto").as<JsonObject>();
  JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("Auto").as<JsonObject>();
  D_baseInfoJSON["device_status"].set("Idle");
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("RunHistory");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"]["step"] = -1;
  D_baseInfoJSON["parameter"]["eventGroup"] = -1;
  D_baseInfoJSON["parameter"]["event"] = -1;
  D_baseInfoJSON["parameter"]["status"] = "DONE";
  // D_baseInfoJSON["parameter"].set(D_loadedActionJSON);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
  vTaskDelay(100/portTICK_PERIOD_MS);


  if (AnySensorData) {
    JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("Auto").as<JsonObject>();
    D_baseInfoJSON["device_status"].set("Idle");
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PoolSensorData");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"].set(poolSensorData);
    String AfterSensorData;
    serializeJsonPretty(D_baseInfoJSON, AfterSensorData);
    Machine_Ctrl.BackendServer.ws_->binaryAll(AfterSensorData);
  }

  Machine_Ctrl.TASK__NOW_ACTION = NULL;
  vTaskDelete(NULL);
}
void SMachine_Ctrl::RUN__LOADED_ACTION()
{
  eTaskState taskState = eTaskGetState(&TASK__NOW_ACTION);
  if (TASK__NOW_ACTION == NULL | taskState == eDeleted) {
    ESP_LOGI("RUN__LOADED_ACTION","RUN");
    xTaskCreate(
      LOADED_ACTION, "LOADED_ACTION",
      10000, NULL, configMAX_PRIORITIES-1, &TASK__NOW_ACTION
    );
  } else {
    switch (taskState) {
      case eRunning:
        ESP_LOGW("PWMMotorTestEvent","eRunning");break;
      case eBlocked: // 任務正在等待某些事件的發生
        ESP_LOGW("PWMMotorTestEvent","eBlocked");break;
      case eSuspended: // 任務已經被暫停
        ESP_LOGW("PWMMotorTestEvent","eSuspended");break;
      default: // 未知的狀態
        ESP_LOGW("PWMMotorTestEvent","unknow");break;
    }
  } 
}




void PWMMotorTestEvent(void* parameter)
{
  ESP_LOGI("PWMMotorTestEvent","START");
  TaskParameters* params = (TaskParameters*) parameter;
  String motorID = params->message;
  int MotorIndex = Machine_Ctrl.pwmMotorIDToMotorIndex(motorID);
  if (MotorIndex > -1) {
    Machine_Ctrl.motorCtrl.SetMotorTo(MotorIndex, 0);
    vTaskDelay(2000);
    Machine_Ctrl.motorCtrl.SetMotorTo(MotorIndex, 90);
    vTaskDelay(2000);
    Machine_Ctrl.motorCtrl.SetMotorTo(MotorIndex, 180);
    JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("").as<JsonObject>();
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["motor_id"].set(motorID);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
  } else {
    ESP_LOGE("PWMMotorTestEvent","找不到伺服馬達資訊: %s", motorID.c_str());
  }
  ESP_LOGI("PWMMotorTestEvent","END");
  delete params;
  Machine_Ctrl.TASK__NOW_ACTION = NULL;
  vTaskDelete(NULL);
}
EVENT_RESULT SMachine_Ctrl::RUN__PWMMotorTestEvent(String motorID)
{
  EVENT_RESULT returnData;
  eTaskState taskState = eTaskGetState(&TASK__NOW_ACTION);
  returnData.status = taskState;
  if (TASK__NOW_ACTION == NULL | taskState == eDeleted) {
    TaskParameters* taskParams = new TaskParameters;
    taskParams->message = motorID;

    xTaskCreate(
      PWMMotorTestEvent, "PWM_MOTOR_TEST",
      10000, taskParams, 1, &TASK__NOW_ACTION
    );
    returnData.message = "OK";
  } else {
    switch (taskState) {
      case eRunning:
        returnData.message = "eRunning";ESP_LOGW("PWMMotorTestEvent","eRunning");break;
      case eBlocked: // 任務正在等待某些事件的發生
        returnData.message = "eBlocked";ESP_LOGW("PWMMotorTestEvent","eBlocked");break;
      case eSuspended: // 任務已經被暫停
        returnData.message = "eSuspended";ESP_LOGW("PWMMotorTestEvent","eSuspended");break;
      default: // 未知的狀態
        returnData.message = "unknow";ESP_LOGW("PWMMotorTestEvent","unknow");break;
    }
  } 
  return returnData;
}



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
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      ESP_LOGW("PeristalticMotorEvent","loop time: %d", (xTaskGetTickCount() - startTick - Machine_Ctrl.TaskSuspendTimeSum) * portTICK_PERIOD_MS / 1000);
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
  Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
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
      D_history["first_active_time"] = String(NOW);
    }
    D_history["last_active_time"] = String(NOW);
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
          Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
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
      Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
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
      }
  }
}

////////////////////////////////////////////////////
// For 不間斷監聽
////////////////////////////////////////////////////

////////////////////////////////////////////////////
// For 互動相關
////////////////////////////////////////////////////

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