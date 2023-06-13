#include "Machine_Ctrl.h"
#include <esp_log.h>
#include "esp_random.h"

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
#include <random>
#include <ctime>

TaskHandle_t TASK_SwitchMotorScan = NULL;
TaskHandle_t TASK_PeristalticMotorScan = NULL;

// extern AsyncWebSocket ws;

////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

double getRandomNumber(double mean, double stddev) {

  // std::random_device rd;  // 随机数种子
  // std::mt19937 gen(rd()); // 随机数引擎
  // std::normal_distribution<double> dist(mean, stddev);
  // double sample = dist(gen); 
  return mean + (double)((esp_random() % 1000)/1000.);
}

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

void SMachine_Ctrl::INIT_PoolData()
{
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  for (JsonPair D_poolItem : D_pools) {
    (*sensorDataSave)[D_poolItem.key()]["PoolID"].set(D_poolItem.key());
    (*sensorDataSave)[D_poolItem.key()]["PoolName"].set(D_poolItem.value().as<JsonObject>()["title"].as<String>());
    (*sensorDataSave)[D_poolItem.key()]["PoolDescription"].set(D_poolItem.value().as<JsonObject>()["description"].as<String>());
    (*sensorDataSave)[D_poolItem.key()]["NO2_wash_volt"].set(-1.);
    (*sensorDataSave)[D_poolItem.key()]["NO2_test_volt"].set(-1.);
    (*sensorDataSave)[D_poolItem.key()]["NO2"].set(-1.);
    (*sensorDataSave)[D_poolItem.key()]["NH4_wash_volt"].set(-1.);
    (*sensorDataSave)[D_poolItem.key()]["NH4_test_volt"].set(-1.);
    (*sensorDataSave)[D_poolItem.key()]["NH4"].set(-1.);
    (*sensorDataSave)[D_poolItem.key()]["pH_volt"].set(-1.);
    (*sensorDataSave)[D_poolItem.key()]["pH"].set(-1.);
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

void SMachine_Ctrl::LOAD__ACTION(JsonObject actionJSON)
{
  ESP_LOGI("LOADED_ACTION","CLEAR");
  loadedAction->clear();
  ESP_LOGI("LOADED_ACTION","SET");
  loadedAction->set(actionJSON);
  ESP_LOGI("LOADED_ACTION","OK");
  // serializeJsonPretty(*loadedAction, Serial);
  // DeserializationError error = deserializeJson(*loadedAction, actionJSONString,  DeserializationOption::NestingLimit(20));
  // if (error) {
  //   Serial.print("deserializeJson() failed: ");
  //   Serial.println(error.c_str());
  // }
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
  Machine_Ctrl.SetLog(
    3, 
    "執行流程: "+D_loadedActionJSON["title"].as<String>(),
    "流程說明: "+D_loadedActionJSON["description"].as<String>(),
    Machine_Ctrl.BackendServer.ws_, NULL
  );
  
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

          Machine_Ctrl.SetLog(
            3, 
            "執行事件: "+D_eventGroupItem_.value()["title"].as<String>(),
            "主流程: "+ D_stepItem_.value()["title"].as<String>(),
            Machine_Ctrl.BackendServer.ws_, NULL
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
                Peristaltic_task_config config_;
                config_.index = peristalticMotorItem["peristaltic_motor"]["index"].as<int>();
                config_.status = peristalticMotorItem["status"].as<int>() == 1 ? PeristalticMotorStatus::FORWARD : PeristalticMotorStatus::REVERSR;
                config_.time = peristalticMotorItem["time"].as<float>();
                String untilString = peristalticMotorItem["until"].as<String>();
                if (untilString == "RO") {
                  config_.until = 1;
                } 
                else if (untilString == "SAMPLE") {
                  config_.until = 2;
                }
                else {
                  config_.until = -1;
                }
                Machine_Ctrl.RUN__PeristalticMotorEvent(&config_);
                // while (Machine_Ctrl.TASK__Peristaltic_MOTOR != NULL) {
                //   vTaskDelay(10);
                // }


                // Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(
                //   peristalticMotorItem["peristaltic_motor"]["index"].as<int>(), 
                //   peristalticMotorItem["status"].as<int>() == 1 ? PeristalticMotorStatus::FORWARD : PeristalticMotorStatus::REVERSR
                // );
                // Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
                // long timecheck = millis();
                

                // vTaskDelay(peristalticMotorItem["time"].as<float>()*1000/portTICK_PERIOD_MS);
                // Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
                // peristalticMotorItem["finish_time"].set(now());
              }

              vTaskDelay(1000/portTICK_PERIOD_MS);

              bool allDone = false;
              while (allDone == false) {
                allDone = true;
                for (int index=0;index<sizeof(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);index++) {
                  if (
                    Machine_Ctrl.peristalticMotorsCtrl.moduleDataList[index] != 0
                  ) {
                    allDone = false;
                  }
                  vTaskDelay(1/portTICK_PERIOD_MS);
                }
                vTaskDelay(10/portTICK_PERIOD_MS);
              }
              // peristalticMotorItem["finish_time"].set(now());

            }

            else if (eventItem.containsKey("spectrophotometer_list")) {
              ESP_LOGI("LOADED_ACTION","      [%d-%d-%d]分光光度計控制",stepCount,eventGroupCount,eventCount);
              for (JsonVariant spectrophotometerItem : eventItem["spectrophotometer_list"].as<JsonArray>()) {
                int spectrophotometerIndex = spectrophotometerItem["spectrophotometer"]["index"].as<int>();
                String GainStr = spectrophotometerItem["gain"].as<String>();
                String value_name = spectrophotometerItem["value_name"].as<String>();
                String spectrophotometerName = spectrophotometerItem["spectrophotometer"]["title"].as<String>();
                ESP_LOGI("LOADED_ACTION","       - %s(%d) %s 測量倍率，並紀錄為: %s", 
                  spectrophotometerName.c_str(), 
                  spectrophotometerIndex, 
                  GainStr.c_str(), value_name.c_str()
                );
                Machine_Ctrl.WireOne.beginTransmission(0x70);
                Machine_Ctrl.WireOne.write(1 << spectrophotometerIndex);
                Machine_Ctrl.WireOne.endTransmission();
                vTaskDelay(100/portTICK_PERIOD_MS);
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
                Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();

                time_t nowTime = now();
                spectrophotometerItem["finish_time"].set(now());
                spectrophotometerItem["measurement_time"].set(now());
                char datetimeChar[30];
                sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
                  year(nowTime), month(nowTime), day(nowTime),
                  hour(nowTime), minute(nowTime), second(nowTime)
                );
                /**
                 * 當 ["action"]["method"] == "RUN" 時，代表為正式執行的流程
                 * 其收到的Sensor數值必須存入 Machine_Ctrl.sensorDataSave 中
                 * 
                 * 反之，若 ["action"]["method"] == "TEST" 時，代表目前為""測試""中
                 * 其收到的Sensor數值""不能""存入 Machine_Ctrl.sensorDataSave 中
                 * 
                 */
                Serial.println(D_loadedActionJSON["data_type"].as<String>());
                if (D_loadedActionJSON["data_type"].as<String>() == "RUN") {
                  (*Machine_Ctrl.sensorDataSave)[poolID][value_name].set(testValue.CH_0);
                  (*Machine_Ctrl.sensorDataSave)[poolID]["Data_datetime"].set(datetimeChar);
                }
                poolSensorData[poolID][value_name]["Gain"].set(GainStr);
                poolSensorData[poolID][value_name]["Value"]["CH0"].set(testValue.CH_0);
                poolSensorData[poolID][value_name]["Value"]["CH1"].set(testValue.CH_1);
                poolSensorData[poolID][value_name]["Time"].set(datetimeChar);


                Machine_Ctrl.SetLog(
                  3, 
                  "分光光度計: " + spectrophotometerName + "獲得新量測值",
                  "資料名稱: "+value_name+"倍率: "+GainStr + ",CH0: "+String(testValue.CH_0)+",CH1: "+String(testValue.CH_1),
                  Machine_Ctrl.BackendServer.ws_, NULL
                );

                AnySensorData = true;
              }
            }
            eventCount += 1;
            vTaskDelay(100/portTICK_PERIOD_MS);
          }
          D_eventGroupItemDetail["finish_time"].set(now());
          vTaskDelay(100/portTICK_PERIOD_MS);
        }
        eventGroupCount+=1;
      }
      D_stepItem_.value()["finish_time"].set(now());
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
    stepCount+=1;
  }
  ESP_LOGI("LOADED_ACTION","END");
  Machine_Ctrl.TASK__NOW_ACTION = NULL;
  D_loadedActionJSON["finish_time"].set(now());

  Machine_Ctrl.SetLog(
    3, 
    "流程: " + D_loadedActionJSON["title"].as<String>() + " 完成",
    "",
    Machine_Ctrl.BackendServer.ws_, NULL
  );

  vTaskDelay(100/portTICK_PERIOD_MS);


  if (AnySensorData) {
    JsonObject D_baseInfoJSON = Machine_Ctrl.BackendServer.GetBaseWSReturnData("Auto").as<JsonObject>();
    D_baseInfoJSON["device_status"].set("Idle");
    D_baseInfoJSON["cmd"].set("poolData");
    if (D_loadedActionJSON["data_type"].as<String>() == String("TEST")) {
      D_baseInfoJSON["action"]["target"].set("TEST_PoolSensorData");
      D_baseInfoJSON["action"]["message"].set("獲得測試用數據");
      D_baseInfoJSON["parameter"].set(poolSensorData);
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["action"]["status"].set("OK");
      String AfterSensorData;
      serializeJsonPretty(D_baseInfoJSON, AfterSensorData);
      Machine_Ctrl.BackendServer.ws_->binaryAll(AfterSensorData);
    } else {
      Machine_Ctrl.BroadcastNewPoolData(poolID);
    }
  }
  
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




void PeristalticMotorEvent(void* parameter)
{
  ESP_LOGW("PeristalticMotorEvent","START");
  Peristaltic_task_config config_ = *((Peristaltic_task_config*) parameter);
  PeristalticMotorStatus status = (config_.status==1 ? PeristalticMotorStatus::FORWARD : PeristalticMotorStatus::REVERSR);
  Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(
    config_.index, 
    status
  );
  Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
  long timecheck = millis();
  long maxTime = (long)(config_.time*1000);
  // Serial.printf("config.time: %.2f, time: %d", config_.time, maxTime);
  if (config_.until != -1) {
    pinMode(config_.until, INPUT);
  }
  while (
    millis() - timecheck <= maxTime
  ) {
    if (config_.until != -1) {
      int value = digitalRead(config_.until);
      if (value == HIGH) {
        break;
      }
    }
    vTaskDelay(10);
  }

  Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(
    config_.index, 
    PeristalticMotorStatus::STOP
  );
  Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
  // Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
  // ESP_LOGW("PeristalticMotorEvent","END");
  // Machine_Ctrl.TASK__Peristaltic_MOTOR = NULL;
  vTaskDelete(NULL);
}
EVENT_RESULT SMachine_Ctrl::RUN__PeristalticMotorEvent(Peristaltic_task_config *config_)
{
  EVENT_RESULT returnData;

  xTaskCreate(
    PeristalticMotorEvent, "MOTOR_RUN",
    10000, config_, 1, NULL
  );
  returnData.message = "OK";
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
// For 互動相關
////////////////////////////////////////////////////

/**
 * @brief 
 * 
 * @param Level 0: DEBUG, 1: ERROR, 2: WARNING, 3: INFO, 4: DETAIL, 5: SUCCESS, 6: FAIL
 * @param Title 
 * @param description 
 * @param wsBroadcast 
 * @return DynamicJsonDocument 
 */
DynamicJsonDocument SMachine_Ctrl::SetLog(int Level, String Title, String description, AsyncWebSocket *server, AsyncWebSocketClient *client)
{
  DynamicJsonDocument logItem(500);
  logItem["level"].set(Level);
  logItem["time"].set(GetNowTimeString());
  logItem["title"].set(Title);
  logItem["description"].set(description);
  logArray.add(logItem);
  if (logArray.size() > 100) {
    logArray.remove(0);
  }
  if (server != NULL or client != NULL) {
    DynamicJsonDocument D_baseInfo = BackendServer.GetBaseWSReturnData("LOG");
    D_baseInfo["cmd"].set("log");
    D_baseInfo["status"].set("OK");
    D_baseInfo["action"]["target"].set("Log");
    switch (Level)
    {
      case 0:
        D_baseInfo["action"]["method"].set("DEBUG");
        break;
      case 1:
        D_baseInfo["action"]["method"].set("ERROR");
        D_baseInfo["action"]["status"].set("FAIL");
        break;
      case 2:
        D_baseInfo["action"]["method"].set("WARNING");
        break;
      case 3:
        D_baseInfo["action"]["method"].set("INFO");
        break;
      case 4:
        D_baseInfo["action"]["method"].set("DETAIL");
        break;
      case 5:
        D_baseInfo["action"]["method"].set("SUCCESS");
        break;
      case 6:
        D_baseInfo["action"]["method"].set("FAIL");
        D_baseInfo["action"]["status"].set("FAIL");
        break;
      default:
        D_baseInfo["action"]["method"].set("INFO");
        break;
    }
    D_baseInfo["action"]["message"].set(Title);
    D_baseInfo["action"]["description"].set(description);
    D_baseInfo["action"]["status"].set("OK");
    String returnString;
    serializeJsonPretty(D_baseInfo, returnString);
    if (server != NULL) {
      server->binaryAll(returnString);
    }
    if (client != NULL) {
      client->binary(returnString);
    }
    D_baseInfo.clear();
  }
  return logItem;
}

//* 廣播指定池的感測器資料出去
//!! 注意，這個廣撥出去的資料是會進資料庫的
void SMachine_Ctrl::BroadcastNewPoolData(String poolID)
{
  DynamicJsonDocument D_baseInfo = BackendServer.GetBaseWSReturnData("");
  D_baseInfo["cmd"].set("poolData");
  D_baseInfo["action"]["message"].set("NewData");
  D_baseInfo["action"]["target"].set("PoolData");
  D_baseInfo["action"]["method"].set("Update");
  D_baseInfo["action"]["status"].set("OK");
  D_baseInfo["parameter"][poolID].set((*sensorDataSave)[poolID]);
  String AfterSensorData;
  serializeJsonPretty(D_baseInfo, AfterSensorData);
  BackendServer.ws_->binaryAll(AfterSensorData);
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

String SMachine_Ctrl::GetNowTimeString()
{
  char datetimeChar[30];
  time_t nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );

  return String(datetimeChar);
}

////////////////////////////////////////////////////
// For 組合行為
////////////////////////////////////////////////////

////////////////////////////////////////////////////
// For 測試
////////////////////////////////////////////////////

void SMachine_Ctrl::UpdateAllPoolsDataRandom()
{
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  for (JsonPair D_poolItem : D_pools) {
    (*sensorDataSave)[D_poolItem.key()]["Data_datetime"].set(Machine_Ctrl.GetNowTimeString());

    (*sensorDataSave)[D_poolItem.key()]["NO2_wash_volt"].set(3.5 + (double)((esp_random() % 100)/1000.));
    (*sensorDataSave)[D_poolItem.key()]["NO2_test_volt"].set(
      (*sensorDataSave)[D_poolItem.key()]["NO2_wash_volt"].as<double>() - (double)((esp_random() % 1000)/1000.)
    );
    (*sensorDataSave)[D_poolItem.key()]["NO2"].set(
      (*sensorDataSave)[D_poolItem.key()]["NO2_test_volt"].as<double>() + 1.5 - (double)((esp_random() % 200)/1000.)
    );
    (*sensorDataSave)[D_poolItem.key()]["NH4_wash_volt"].set(3.5 + (double)((esp_random() % 100)/1000.));
    (*sensorDataSave)[D_poolItem.key()]["NH4_test_volt"].set(
      (*sensorDataSave)[D_poolItem.key()]["NH4_wash_volt"].as<double>() - (double)((esp_random() % 1000)/1000.) 
    );
    (*sensorDataSave)[D_poolItem.key()]["NH4"].set(
      (*sensorDataSave)[D_poolItem.key()]["NH4_test_volt"].as<double>() - (double)((esp_random() % 200)/1000.)
    );
    (*sensorDataSave)[D_poolItem.key()]["pH_volt"].set(
      4 - (double)((esp_random() % 1000)/1000.)
    );
    (*sensorDataSave)[D_poolItem.key()]["pH"].set(
      (*sensorDataSave)[D_poolItem.key()]["pH_volt"].as<double>() + 3 - (double)((esp_random() % 500)/1000.)
    );
  }
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