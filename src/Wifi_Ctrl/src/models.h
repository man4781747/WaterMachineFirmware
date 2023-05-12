#ifndef WIFI_MODEL_H
#define WIFI_MODEL_H


#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"
#include <map>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "Machine_Ctrl/src/Machine_Ctrl.h"
extern SMachine_Ctrl Machine_Ctrl;

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