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
    peristalticMotorSet_1["peristaltic_motor"]["finish_time"].set(-1);
    peristalticMotorSet_1["status"].set(1);
    peristalticMotorSet_1["time"].set(1.0);

    DynamicJsonDocument eventItem_2(500);
    JsonArray L_peristalticMotorList_2 = eventItem_2.createNestedArray("peristaltic_motor_list");
    DynamicJsonDocument peristalticMotorSet_2(500);
    
    peristalticMotorSet_2["peristaltic_motor"]["index"].set(D_peristalticMotorSetting["index"].as<int>());
    peristalticMotorSet_2["peristaltic_motor"]["title"].set(D_peristalticMotorSetting["title"].as<String>());
    peristalticMotorSet_2["peristaltic_motor"]["description"].set(D_peristalticMotorSetting["description"].as<String>());
    peristalticMotorSet_2["peristaltic_motor"]["finish_time"].set(-1);
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
    pwmMotorSet_1["pwn_motor"]["finish_time"].set(-1);
    pwmMotorSet_1["status"].set(0);

    DynamicJsonDocument eventItem_2(500);
    JsonArray L_pwnMotorList_2 = eventItem_2.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_2(500);
    
    pwmMotorSet_2["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_2["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_2["pwn_motor"]["description"].set(D_pwmMotorSetting["description"].as<String>());
    pwmMotorSet_2["pwn_motor"]["finish_time"].set(-1);
    pwmMotorSet_2["status"].set(90);

    DynamicJsonDocument eventItem_3(500);
    JsonArray L_pwnMotorList_3 = eventItem_3.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_3(500);
    pwmMotorSet_3["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_3["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_3["pwn_motor"]["description"].set(D_pwmMotorSetting["description"].as<String>());
    pwmMotorSet_3["pwn_motor"]["finish_time"].set(-1);
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


void ws_TestEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Test Event, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
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
  else if (D_event_group.containsKey(TargetName)) {

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
    D_baseInfoJSON["parameter"]["message"].set("找不到事件組: " + TargetName);
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
    serializeJsonPretty(D_newConfig["event"], Serial);
    serializeJsonPretty(D_oldConfig, Serial);
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


#endif