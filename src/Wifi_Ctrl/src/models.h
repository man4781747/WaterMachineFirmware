#ifndef WIFI_MODEL_H
#define WIFI_MODEL_H


#include <ESPAsyncWebServer.h>
// #include <AsyncWebServer_ESP32_W5500.h>
#include "AsyncTCP.h"
#include <map>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "Machine_Ctrl/src/Machine_Ctrl.h"
extern SMachine_Ctrl Machine_Ctrl;

JsonObject BuildPWNMotorEventJSONItem(JsonArray pwmMotorEventList){
  JsonObject D_AllpwmMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  DynamicJsonDocument D_pwmMotorListItem(5000);
  JsonObject returnJSON = D_pwmMotorListItem.as<JsonObject>();
  JsonArray L_allPwmEventList = returnJSON.createNestedArray("pwm_motor_list");
  for (JsonVariant pwmMotorEventItem : pwmMotorEventList) {
    String pwmID = pwmMotorEventItem["pwm_motor_id"].as<String>();
    if (!D_AllpwmMotorSetting.containsKey(pwmID)) {
      continue;
    }
    DynamicJsonDocument D_OnePwmMotorSetItem(500);
    D_OnePwmMotorSetItem["pwn_motor"]["index"].set(D_AllpwmMotorSetting[pwmID]["index"].as<int>());
    D_OnePwmMotorSetItem["pwn_motor"]["title"].set(D_AllpwmMotorSetting[pwmID]["title"].as<String>());
    D_OnePwmMotorSetItem["pwn_motor"]["description"].set(D_AllpwmMotorSetting[pwmID]["description"].as<String>());
    D_OnePwmMotorSetItem["finish_time"].set(-1);
    D_OnePwmMotorSetItem["status"].set(D_AllpwmMotorSetting["status"].as<int>());
    L_allPwmEventList.add(D_OnePwmMotorSetItem);
  }
  return returnJSON;
}

/**
 * @brief 強制停止所有動作
 * 
 */
void ws_StopAllActionTask(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    vTaskSuspend(Machine_Ctrl.TASK__NOW_ACTION);
    vTaskDelete(Machine_Ctrl.TASK__NOW_ACTION);
  }

  // vTaskSuspend(Machine_Ctrl.TASK__PWM_MOTOR);
  // vTaskDelete(Machine_Ctrl.TASK__PWM_MOTOR);
  // vTaskSuspend(Machine_Ctrl.TASK__Peristaltic_MOTOR);
  // vTaskDelete(Machine_Ctrl.TASK__Peristaltic_MOTOR);
  // vTaskSuspend(Machine_Ctrl.TASK__History);
  // vTaskDelete(Machine_Ctrl.TASK__History);

  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("System");
  D_baseInfoJSON["action"]["method"].set("STOP");
  D_baseInfoJSON["parameter"]["message"].set("強制停止機器的所有動作，回歸待機狀態");
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}



void ws_GetDeiveConfig(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();


  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("DeiveConfig");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(Machine_Ctrl.spiffs.DeviceBaseInfo->as<JsonObject>());
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_PatchDeiveConfig(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_oldConfig = Machine_Ctrl.spiffs.DeviceBaseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  for (JsonPair newConfigItem : D_newConfig) {
    if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
      D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
    }
  }
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("DeiveConfig");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_oldConfig);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}


//!蝦池設定相關API

void ws_PatchPoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Patch Spectrophotometer Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  if (D_pools.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    JsonObject D_oldConfig = D_pools[TargetName];
    for (JsonPair newConfigItem : D_newConfig) {
      if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
      }
    }
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Pool");
    D_baseInfoJSON["action"]["method"].set("Update");
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蝦池: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_DeletePoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  if (D_pools.containsKey(TargetName)) {
    D_pools.remove(TargetName);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    D_baseInfoJSON["action"]["target"].set("Pool");
    D_baseInfoJSON["action"]["method"].set("Delete");
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蝦池: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_GetPoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Pools Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  if (D_pools.containsKey(TargetName)) {
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Pool");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_pools[TargetName]);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蝦池: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetAllPoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("Pool");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_pools);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_AddNewPoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title")) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    char random_name[16];
    uint8_t random_bytes[8];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    for (int i = 0; i < sizeof(random_bytes); i++) {
      sprintf(&random_name[i*2], "%02x", random_bytes[i]);
    }
    JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
    D_pools[String(random_name)]["title"].set(D_newConfig["title"].as<String>());
    D_pools[String(random_name)]["description"].set(D_newConfig["description"].as<String>());
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Pool");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][String(random_name)].set(D_pools[String(random_name)]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  }
}


//!分光光度計設定相關API

void ws_TestSpectrophotometer(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "分光光度計測試 ID: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("儀器忙碌中，請稍後再試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
  else if (D_spectrophotometer.containsKey(TargetName)) {
    DynamicJsonDocument actionItem(10000);
    actionItem["title"].set("分光光度計測試");
    actionItem["description"].set("分光光度計測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["finish_time"].set(-1);

    JsonArray L_stepList = actionItem.createNestedArray("step_list");

    DynamicJsonDocument stepItem(5000);
    stepItem["SpectrophotometerTest"]["title"].set("分光光度計測試");
    stepItem["SpectrophotometerTest"]["description"].set("分光光度計測試");
    stepItem["SpectrophotometerTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["SpectrophotometerTest"].createNestedArray("event_group_list");

    DynamicJsonDocument eventGroupItem(1000);
    eventGroupItem["SpectrophotometerTest"]["title"].set("分光光度計測試");
    eventGroupItem["SpectrophotometerTest"]["description"].set("分光光度計測試");
    eventGroupItem["SpectrophotometerTest"]["finish_time"].set(-1);
    JsonArray L_eventList = eventGroupItem["SpectrophotometerTest"].createNestedArray("event_list");

    DynamicJsonDocument spectrophotometerInfo(1000);

    JsonObject D_spectrophotometerSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"][TargetName];

    
    DynamicJsonDocument eventItem(5000);
    JsonArray L_spectrophotometer = eventItem.createNestedArray("spectrophotometer_list");

    DynamicJsonDocument spectrophotometerSet(500);
    spectrophotometerSet["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet["spectrophotometer"]["description"].set(D_spectrophotometerSetting["description"].as<String>());
    spectrophotometerSet["gain"].set("1X");
    spectrophotometerSet["value_name"].set("test");
    spectrophotometerSet["measurement_time"].set(-1);
    spectrophotometerSet["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet);

    DynamicJsonDocument spectrophotometerSet_2(500);
    spectrophotometerSet_2["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_2["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_2["spectrophotometer"]["description"].set(D_spectrophotometerSetting["description"].as<String>());
    spectrophotometerSet_2["gain"].set("2X");
    spectrophotometerSet_2["value_name"].set("test");
    spectrophotometerSet["measurement_time"].set(-1);
    spectrophotometerSet_2["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_2);

    DynamicJsonDocument spectrophotometerSet_3(500);
    spectrophotometerSet_3["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_3["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_3["spectrophotometer"]["description"].set(D_spectrophotometerSetting["description"].as<String>());
    spectrophotometerSet_3["gain"].set("4X");
    spectrophotometerSet_3["value_name"].set("test");
    spectrophotometerSet["measurement_time"].set(-1);
    spectrophotometerSet_3["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_3);

    DynamicJsonDocument spectrophotometerSet_4(500);
    spectrophotometerSet_4["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_4["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_4["spectrophotometer"]["description"].set(D_spectrophotometerSetting["description"].as<String>());
    spectrophotometerSet_4["gain"].set("8X");
    spectrophotometerSet_4["value_name"].set("test");
    spectrophotometerSet["measurement_time"].set(-1);
    spectrophotometerSet_4["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_4);

    DynamicJsonDocument spectrophotometerSet_5(500);
    spectrophotometerSet_5["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_5["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_5["spectrophotometer"]["description"].set(D_spectrophotometerSetting["description"].as<String>());
    spectrophotometerSet_5["gain"].set("48X");
    spectrophotometerSet_5["value_name"].set("test");
    spectrophotometerSet["measurement_time"].set(-1);
    spectrophotometerSet_5["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_5);

    DynamicJsonDocument spectrophotometerSet_6(500);
    spectrophotometerSet_6["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_6["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_6["spectrophotometer"]["description"].set(D_spectrophotometerSetting["description"].as<String>());
    spectrophotometerSet_6["gain"].set("96X");
    spectrophotometerSet_6["value_name"].set("test");
    spectrophotometerSet["measurement_time"].set(-1);
    spectrophotometerSet_6["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_6);

    L_eventList.add(eventItem);
    L_eventGroupList.add(eventGroupItem);
    L_stepList.add(stepItem);

    String settingString;
    serializeJson(actionItem,settingString);
    actionItem.clear();
    Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.RUN__LOADED_ACTION();

    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("start");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("分光光度計: " + TargetName+" 開始測試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("找不到分光光度計: " + TargetName);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }

}

void ws_DeleteSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  if (D_spectrophotometer.containsKey(TargetName)) {
    D_spectrophotometer.remove(TargetName);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Delete");
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到分光光度計: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_PatchSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Patch Spectrophotometer Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  if (D_spectrophotometer.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    JsonObject D_oldConfig = D_spectrophotometer[TargetName];
    for (JsonPair newConfigItem : D_newConfig) {
      if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
      }
    }
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Update");
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_GetSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Spectrophotometer Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  if (D_spectrophotometer.containsKey(TargetName)) {
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_spectrophotometer[TargetName]);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到分光光度計: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetAllSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_spectrophotometer);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_AddNewSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "index 與 title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>3) {
      D_baseInfoJSON["status"].set("FAIL");
      D_baseInfoJSON["parameter"]["message"] = "index需介於0~3(含)";
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      client->text(returnString);
    } else {
      char random_name[16];
      uint8_t random_bytes[8];
      esp_fill_random(random_bytes, sizeof(random_bytes));
      for (int i = 0; i < sizeof(random_bytes); i++) {
        sprintf(&random_name[i*2], "%02x", random_bytes[i]);
      }

      JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
      D_spectrophotometer[String(random_name)]["index"].set(newIndex);
      D_spectrophotometer[String(random_name)]["title"].set(D_newConfig["title"].as<String>());
      D_spectrophotometer[String(random_name)]["description"].set(D_newConfig["description"].as<String>());
      D_baseInfoJSON["status"].set("OK");
      D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_spectrophotometer[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      server->textAll(returnString);
    }
  }
}


//!蠕動馬達設定相關API

void ws_TestPeristalticMotor(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Test Peristaltic Motor, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("儀器忙碌中，請稍後再試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
  else if (D_peristaltic_motor.containsKey(TargetName)) {
    DynamicJsonDocument actionItem(10000);
    actionItem["title"].set("蠕動馬達測試");
    actionItem["description"].set("蠕動馬達測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["finish_time"].set(-1);

    JsonArray L_stepList = actionItem.createNestedArray("step_list");

    DynamicJsonDocument stepItem(5000);
    stepItem["peristalticMotorTest"]["title"].set("蠕動馬達測試");
    stepItem["peristalticMotorTest"]["description"].set("蠕動馬達測試");
    stepItem["peristalticMotorTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["peristalticMotorTest"].createNestedArray("event_group_list");

    DynamicJsonDocument eventGroupItem(1000);
    eventGroupItem["peristalticMotorTest"]["title"].set("蠕動馬達測試");
    eventGroupItem["peristalticMotorTest"]["description"].set("蠕動馬達測試");
    eventGroupItem["peristalticMotorTest"]["finish_time"].set(-1);
    JsonArray L_eventList = eventGroupItem["peristalticMotorTest"].createNestedArray("event_list");

    DynamicJsonDocument peristalticMotorInfo(1000);

    JsonObject D_peristalticMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"][TargetName];

    
    DynamicJsonDocument eventItem_1(500);
    JsonArray L_peristalticMotorList_1 = eventItem_1.createNestedArray("peristaltic_motor_list");
    DynamicJsonDocument peristalticMotorSet_1(500);
    
    peristalticMotorSet_1["peristaltic_motor"]["index"].set(D_peristalticMotorSetting["index"].as<int>());
    peristalticMotorSet_1["peristaltic_motor"]["title"].set(D_peristalticMotorSetting["title"].as<String>());
    peristalticMotorSet_1["peristaltic_motor"]["description"].set(D_peristalticMotorSetting["description"].as<String>());
    peristalticMotorSet_1["finish_time"].set(-1);
    peristalticMotorSet_1["status"].set(1);
    peristalticMotorSet_1["time"].set(1.0);

    DynamicJsonDocument eventItem_2(500);
    JsonArray L_peristalticMotorList_2 = eventItem_2.createNestedArray("peristaltic_motor_list");
    DynamicJsonDocument peristalticMotorSet_2(500);
    
    peristalticMotorSet_2["peristaltic_motor"]["index"].set(D_peristalticMotorSetting["index"].as<int>());
    peristalticMotorSet_2["peristaltic_motor"]["title"].set(D_peristalticMotorSetting["title"].as<String>());
    peristalticMotorSet_2["peristaltic_motor"]["description"].set(D_peristalticMotorSetting["description"].as<String>());
    peristalticMotorSet_2["finish_time"].set(-1);
    peristalticMotorSet_2["status"].set(-1);
    peristalticMotorSet_2["time"].set(1.0);


    L_peristalticMotorList_1.add(peristalticMotorSet_1);
    L_eventList.add(eventItem_1);
    L_peristalticMotorList_2.add(peristalticMotorSet_2);
    L_eventList.add(eventItem_2);
    L_eventGroupList.add(eventGroupItem);
    L_stepList.add(stepItem);

    String settingString;
    serializeJson(actionItem,settingString);
    actionItem.clear();
    Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.RUN__LOADED_ACTION();

    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("start");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("蠕動馬達: " + TargetName+" 開始測試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("找不到蠕動馬達: " + TargetName);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }

}

void ws_GetPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "GetPeristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_PatchPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_peristaltic_motor.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    JsonObject D_oldConfig = D_peristaltic_motor[TargetName];
    for (JsonPair newConfigItem : D_newConfig) {
      if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
      }
    }
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_DeletePeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor_list = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_peristaltic_motor_list.containsKey(TargetName)) {
    D_peristaltic_motor_list.remove(TargetName);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Delete");
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_GetAllPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_peristaltic_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_AddNewPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "index 與 title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>7) {
      D_baseInfoJSON["status"].set("FAIL");
      D_baseInfoJSON["parameter"]["message"] = "index需介於0~7(含)";
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      client->text(returnString);
    } else {
      char random_name[16];
      uint8_t random_bytes[8];
      esp_fill_random(random_bytes, sizeof(random_bytes));
      for (int i = 0; i < sizeof(random_bytes); i++) {
        sprintf(&random_name[i*2], "%02x", random_bytes[i]);
      }

      JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
      D_peristaltic_motor[String(random_name)]["index"].set(newIndex);
      D_peristaltic_motor[String(random_name)]["title"].set(D_newConfig["title"].as<String>());
      D_peristaltic_motor[String(random_name)]["description"].set(D_newConfig["description"].as<String>());
      D_baseInfoJSON["status"].set("OK");
      D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_peristaltic_motor[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      server->textAll(returnString);
    }
  }
}


//!伺服馬達設定相關API

void ws_TestPwmMotor(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Test PWM Motor, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pwm_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("儀器忙碌中，請稍後再試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
  else if (D_pwm_motor.containsKey(TargetName)) {

    DynamicJsonDocument actionItem(10000);
    actionItem["title"].set("伺服馬達測試");
    actionItem["description"].set("伺服馬達測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["finish_time"].set(-1);

    JsonArray L_stepList = actionItem.createNestedArray("step_list");

    DynamicJsonDocument stepItem(5000);
    stepItem["pwmMotorTest"]["title"].set("伺服馬達測試");
    stepItem["pwmMotorTest"]["description"].set("伺服馬達測試");
    stepItem["pwmMotorTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["pwmMotorTest"].createNestedArray("event_group_list");

    DynamicJsonDocument eventGroupItem(1000);
    eventGroupItem["pwmMotorTest"]["title"].set("伺服馬達測試");
    eventGroupItem["pwmMotorTest"]["description"].set("伺服馬達測試");
    eventGroupItem["pwmMotorTest"]["finish_time"].set(-1);
    JsonArray L_eventList = eventGroupItem["pwmMotorTest"].createNestedArray("event_list");

    DynamicJsonDocument pwmMotorInfo(1000);

    JsonObject D_pwmMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"][TargetName];

    
    DynamicJsonDocument eventItem_1(500);
    JsonArray L_pwnMotorList_1 = eventItem_1.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_1(500);
    
    pwmMotorSet_1["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_1["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_1["pwn_motor"]["description"].set(D_pwmMotorSetting["description"].as<String>());
    pwmMotorSet_1["finish_time"].set(-1);
    pwmMotorSet_1["status"].set(0);

    DynamicJsonDocument eventItem_2(500);
    JsonArray L_pwnMotorList_2 = eventItem_2.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_2(500);
    
    pwmMotorSet_2["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_2["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_2["pwn_motor"]["description"].set(D_pwmMotorSetting["description"].as<String>());
    pwmMotorSet_2["finish_time"].set(-1);
    pwmMotorSet_2["status"].set(90);

    DynamicJsonDocument eventItem_3(500);
    JsonArray L_pwnMotorList_3 = eventItem_3.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_3(500);
    pwmMotorSet_3["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_3["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_3["pwn_motor"]["description"].set(D_pwmMotorSetting["description"].as<String>());
    pwmMotorSet_3["finish_time"].set(-1);
    pwmMotorSet_3["status"].set(180);

    L_pwnMotorList_1.add(pwmMotorSet_1);
    L_eventList.add(eventItem_1);
    L_pwnMotorList_2.add(pwmMotorSet_2);
    L_eventList.add(eventItem_2);
    L_pwnMotorList_3.add(pwmMotorSet_3);
    L_eventList.add(eventItem_3);
    L_eventGroupList.add(eventGroupItem);
    L_stepList.add(stepItem);

    String settingString;
    serializeJson(actionItem,settingString);
    actionItem.clear();
    Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.RUN__LOADED_ACTION();

    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("start");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("伺服馬達: " + TargetName+" 開始測試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["motor_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("找不到伺服馬達: " + TargetName);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }

}

void ws_GetPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "GetPeristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到伺服馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_PatchPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "GetPeristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    JsonObject D_oldConfig = D_steps_group[TargetName];
    for (JsonPair newConfigItem : D_newConfig) {
      if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
      }
    }
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到伺服馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  ws.textAll(returnString);
}

void ws_DeletePwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Delete Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_steps_group.remove(TargetName);
    D_baseInfoJSON["status"] = "OK";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "PwmMotor";
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到伺服馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  Serial.println(returnString);
  ws.textAll(returnString);
}

void ws_AddNewPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "index 與 title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>32) {
      D_baseInfoJSON["status"].set("FAIL");
      D_baseInfoJSON["parameter"]["message"] = "index需介於0~31(含)";
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      client->text(returnString);
    } else {
      char random_name[16];
      uint8_t random_bytes[8];
      esp_fill_random(random_bytes, sizeof(random_bytes));
      for (int i = 0; i < sizeof(random_bytes); i++) {
        sprintf(&random_name[i*2], "%02x", random_bytes[i]);
      }

      JsonObject D_pwm_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
      D_pwm_motor[String(random_name)]["index"].set(newIndex);
      D_pwm_motor[String(random_name)]["title"].set(D_newConfig["title"].as<String>());
      D_pwm_motor[String(random_name)]["description"].set(D_newConfig["description"].as<String>());
      D_baseInfoJSON["status"].set("OK");
      D_baseInfoJSON["action"]["target"].set("PwmMotor");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_pwm_motor[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      ws.textAll(returnString);
    }
  }
}

void ws_GetAllPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pwm_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("PwmMotor");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_pwm_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}


//!事件組設定相關API

void ws_TestEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Test EVENT, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pwm_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["event_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("儀器忙碌中，請稍後再試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
  else if (D_pwm_motor.containsKey(TargetName)) {

    DynamicJsonDocument actionItem(10000);
    actionItem["title"].set("事件組測試");
    actionItem["description"].set("事件組測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["finish_time"].set(-1);

    JsonArray L_stepList = actionItem.createNestedArray("step_list");

    DynamicJsonDocument stepItem(5000);
    stepItem["pwmMotorTest"]["title"].set("事件組測試");
    stepItem["pwmMotorTest"]["description"].set("事件組測試");
    stepItem["pwmMotorTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["pwmMotorTest"].createNestedArray("event_group_list");

    DynamicJsonDocument eventGroupItem(1000);
    JsonObject D_thisEventSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"][TargetName];
    eventGroupItem[TargetName]["title"].set(D_thisEventSetting["title"].as<String>());
    eventGroupItem[TargetName]["description"].set(D_thisEventSetting["description"].as<String>());
    eventGroupItem[TargetName]["finish_time"].set(-1);
    JsonArray L_eventList = eventGroupItem[TargetName].createNestedArray("event_list");
    


    JsonObject D_pwmMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
    JsonObject D_peristalticMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];

    for (JsonVariant eventItem : D_thisEventSetting["event"].as<JsonArray>()) {
      if (eventItem.containsKey("pwm_motor_list")) {
        DynamicJsonDocument D_pwmMotorListItem(1000);
        JsonArray L_pwmMotorList = D_pwmMotorListItem.createNestedArray("pwm_motor_list");
        for (JsonVariant pwmMotorSetItem : eventItem["pwm_motor_list"].as<JsonArray>()) {
          String pwmID = pwmMotorSetItem["pwm_motor_id"].as<String>();
          if (!D_pwmMotorSetting.containsKey(pwmID)) {
            continue;
          }
          DynamicJsonDocument D_pwmMotorSetItem(500);
          D_pwmMotorSetItem["pwn_motor"]["index"].set(D_pwmMotorSetting[pwmID]["index"].as<int>());
          D_pwmMotorSetItem["pwn_motor"]["title"].set(D_pwmMotorSetting[pwmID]["title"].as<String>());
          D_pwmMotorSetItem["pwn_motor"]["description"].set(D_pwmMotorSetting[pwmID]["description"].as<String>());
          D_pwmMotorSetItem["pwn_motor"]["finish_time"].set(-1);
          D_pwmMotorSetItem["status"].set(pwmMotorSetItem["status"].as<int>());
          L_pwmMotorList.add(D_pwmMotorSetItem);
        }
        L_eventList.add(D_pwmMotorListItem);
      }
      else if (eventItem.containsKey("peristaltic_motor_list")) {
        DynamicJsonDocument D_peristalticMotorListItem(1000);
        JsonArray L_peristalticMotorList = D_peristalticMotorListItem.createNestedArray("peristaltic_motor_list");
        for (JsonVariant peristalticMotorSetItem : eventItem["peristaltic_motor_list"].as<JsonArray>()) {
          String peristalticID = peristalticMotorSetItem["peristaltic_motor_id"].as<String>();
          if (!D_peristalticMotorSetting.containsKey(peristalticID)) {
            continue;
          }
          DynamicJsonDocument D_peristalticMotorSetItem(500);
          D_peristalticMotorSetItem["peristaltic_motor"]["index"].set(D_peristalticMotorSetting[peristalticID]["index"].as<int>());
          D_peristalticMotorSetItem["peristaltic_motor"]["title"].set(D_peristalticMotorSetting[peristalticID]["title"].as<String>());
          D_peristalticMotorSetItem["peristaltic_motor"]["description"].set(D_peristalticMotorSetting[peristalticID]["description"].as<String>());
          D_peristalticMotorSetItem["peristaltic_motor"]["finish_time"].set(-1);
          D_peristalticMotorSetItem["status"].set(peristalticMotorSetItem["status"].as<int>());
          D_peristalticMotorSetItem["time"].set(peristalticMotorSetItem["time"].as<float>());
          L_peristalticMotorList.add(D_peristalticMotorSetItem);
        }
        L_eventList.add(D_peristalticMotorListItem);
      }
    }

    L_eventGroupList.add(eventGroupItem);
    L_stepList.add(stepItem);

    String settingString;
    serializeJson(actionItem,settingString);
    // serializeJsonPretty(actionItem, Serial);
    actionItem.clear();
    Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.RUN__LOADED_ACTION();

    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("start");
    D_baseInfoJSON["parameter"]["event_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("事件組: " + TargetName+" 開始測試");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Test");
    D_baseInfoJSON["parameter"]["status"].set("end");
    D_baseInfoJSON["parameter"]["event_id"].set(TargetName);
    D_baseInfoJSON["parameter"]["message"].set("找不到事件組 " + TargetName);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }

}

void ws_GetAllEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("Event");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_event_group);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Event Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  if (D_event_group.containsKey(TargetName)) {
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_event_group[TargetName]);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Evevnt: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_PatchEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Patch Event Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  if (D_event_group.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    JsonObject D_oldConfig = D_event_group[TargetName];


    if (D_oldConfig["title"].as<String>() != D_newConfig["title"].as<String>()) {
      D_oldConfig["title"] = D_newConfig["title"].as<String>();
    }
    if (D_oldConfig["description"].as<String>() != D_newConfig["description"].as<String>()) {
      D_oldConfig["description"] = D_newConfig["description"].as<String>();
    }
    D_oldConfig.remove("event");
    D_oldConfig["event"].set(D_newConfig["event"]);
    
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    ws.textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Event: " + TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
}

void ws_DeleteEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Delete Event Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  if (D_event_group.containsKey(TargetName)) {
    D_event_group.remove(TargetName);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Event";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Event: " + TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
}

void ws_AddNewEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title")) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
    char random_name[16];
    uint8_t random_bytes[8];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    for (int i = 0; i < sizeof(random_bytes); i++) {
      sprintf(&random_name[i*2], "%02x", random_bytes[i]);
    }
    String NewIDString = String(random_name);
    D_event_group[NewIDString]["title"].set(D_newConfig["title"].as<String>());
    D_event_group[NewIDString]["description"].set(D_newConfig["description"].as<String>());
    D_event_group[NewIDString].createNestedArray("event");
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][NewIDString].set(D_event_group[NewIDString]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    ws.textAll(returnString);
  }
}


//!步驟設定相關API

void ws_GetAllStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("Step");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_event_group);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Step Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Step");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Step: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_DeleteStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Delete Event Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  if (D_steps_group.containsKey(TargetName)) {
    D_steps_group.remove(TargetName);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Step";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Step: " + TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
}

void ws_PatchStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Patch Step Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  if (D_steps_group.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    JsonObject D_oldConfig = D_steps_group[TargetName];

    if (D_oldConfig["title"].as<String>() != D_newConfig["title"].as<String>()) {
      D_oldConfig["title"] = D_newConfig["title"].as<String>();
    }
    if (D_oldConfig["description"].as<String>() != D_newConfig["description"].as<String>()) {
      D_oldConfig["description"] = D_newConfig["description"].as<String>();
    }
    D_oldConfig.remove("steps");
    D_oldConfig["steps"].set(D_newConfig["steps"]);
    
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Step");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    ws.textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Step: " + TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
}

void ws_AddNewStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title")) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
    char random_name[16];
    uint8_t random_bytes[8];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    for (int i = 0; i < sizeof(random_bytes); i++) {
      sprintf(&random_name[i*2], "%02x", random_bytes[i]);
    }
    String NewIDString = String(random_name);
    D_steps_group[NewIDString]["title"].set(D_newConfig["title"].as<String>());
    D_steps_group[NewIDString]["description"].set(D_newConfig["description"].as<String>());
    D_steps_group[NewIDString].createNestedArray("steps");
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Step");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][NewIDString].set(D_steps_group[NewIDString]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    ws.textAll(returnString);
  }
}


//!流程設定相關API


void ws_GetPipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Pipeline Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  if (D_pipeline.containsKey(TargetName)) {
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Pipeline");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_pipeline[TargetName]);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Pipeline: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_PatchPipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Patch Pipeline Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  if (D_pipeline.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    JsonObject D_oldConfig = D_pipeline[TargetName];

    if (D_oldConfig["title"].as<String>() != D_newConfig["title"].as<String>()) {
      D_oldConfig["title"] = D_newConfig["title"].as<String>();
    }
    if (D_oldConfig["description"].as<String>() != D_newConfig["description"].as<String>()) {
      D_oldConfig["description"] = D_newConfig["description"].as<String>();
    }
    if (D_oldConfig["pool"].as<String>() != D_newConfig["pool"].as<String>()) {
      D_oldConfig["pool"] = D_newConfig["pool"].as<String>();
    }
    D_oldConfig.remove("steps");
    D_oldConfig["steps"].set(D_newConfig["steps"]);
    
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Pipeline");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    ws.textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Pipeline: " + TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
}


void ws_DeletePipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Delete Event Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  if (D_pipeline.containsKey(TargetName)) {
    D_pipeline.remove(TargetName);
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Pipeline";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->textAll(returnString);
  } else {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Delete: " + TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
}

void ws_GetAllPipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("Pipeline");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_pipeline);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_AddNewPipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title") | !D_newConfig.containsKey("pool")) {
    D_baseInfoJSON["status"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "title 參數與 pool 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
    char random_name[16];
    uint8_t random_bytes[8];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    for (int i = 0; i < sizeof(random_bytes); i++) {
      sprintf(&random_name[i*2], "%02x", random_bytes[i]);
    }
    String NewIDString = String(random_name);
    D_pipeline[NewIDString]["title"].set(D_newConfig["title"].as<String>());
    D_pipeline[NewIDString]["description"].set(D_newConfig["description"].as<String>());
    D_pipeline[NewIDString]["pool"].set(D_newConfig["pool"].as<String>());
    D_pipeline[NewIDString].createNestedArray("steps");
    D_baseInfoJSON["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Pipeline");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][NewIDString].set(D_pipeline[NewIDString]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    ws.textAll(returnString);
  }
}



#endif