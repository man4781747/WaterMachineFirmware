#ifndef WIFI_MODEL_H
#define WIFI_MODEL_H


#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"
#include <map>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "Machine_Ctrl/src/Machine_Ctrl.h"
extern SMachine_Ctrl Machine_Ctrl;

void ws_GetPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD("Websocket", "GetPeristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_PatchPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_peristaltic_motor.containsKey(TargetName)) {
    JsonObject D_newConfig = D_data->as<JsonObject>();
    JsonObject D_oldConfig = D_peristaltic_motor[TargetName];
    for (JsonPair newConfigItem : D_newConfig) {
      if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
      }
    }
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_DeletePeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor_list = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_peristaltic_motor_list.containsKey(TargetName)) {
    D_peristaltic_motor_list.remove(TargetName);
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Delete");
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  server->textAll(returnString);
}

void ws_GetAllPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_peristaltic_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_AddNewPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_data->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "index 與 title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>7) {
      D_baseInfoJSON["message"].set("FAIL");
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
      D_baseInfoJSON["message"].set("OK");
      D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_peristaltic_motor[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      server->textAll(returnString);
    }
  }
}

void ws_GetPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD("websocket API", "GetPeristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到伺服馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_PatchPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD("websocket API", "GetPeristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    JsonObject D_newConfig = D_data->as<JsonObject>();
    JsonObject D_oldConfig = D_steps_group[TargetName];
    for (JsonPair newConfigItem : D_newConfig) {
      if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
      }
    }
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到伺服馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  ws.textAll(returnString);
}

void ws_DeletePwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD("websocket API", "Delete Peristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_steps_group.remove(TargetName);
    D_baseInfoJSON["message"] = "OK";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "PwmMotor";
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到伺服馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  Serial.println(returnString);
  ws.textAll(returnString);
}

void ws_AddNewPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_data->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "index 與 title 參數為必要項目";
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>32) {
      D_baseInfoJSON["message"].set("FAIL");
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
      D_baseInfoJSON["message"].set("OK");
      D_baseInfoJSON["action"]["target"].set("PwmMotor");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_pwm_motor[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      ws.textAll(returnString);
    }
  }
}

void ws_GetAllPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pwm_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["action"]["target"].set("PwmMotor");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_pwm_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}


#endif