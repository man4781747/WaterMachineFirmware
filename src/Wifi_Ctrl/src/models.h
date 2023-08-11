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
  JsonArray L_allPwmEventList = D_pwmMotorListItem.createNestedArray("pwm_motor_list");
  for (JsonVariant pwmMotorEventItem : pwmMotorEventList) {
    String pwmID = pwmMotorEventItem["id"].as<String>();
    if (!D_AllpwmMotorSetting.containsKey(pwmID)) {
      continue;
    }
    DynamicJsonDocument D_OnePwmMotorSetItem(500);
    D_OnePwmMotorSetItem["pwn_motor"]["index"].set(D_AllpwmMotorSetting[pwmID]["index"].as<int>());
    D_OnePwmMotorSetItem["pwn_motor"]["title"].set(D_AllpwmMotorSetting[pwmID]["title"].as<String>());
    D_OnePwmMotorSetItem["pwn_motor"]["desp"].set(D_AllpwmMotorSetting[pwmID]["desp"].as<String>());
    D_OnePwmMotorSetItem["finish_time"].set(-1);
    D_OnePwmMotorSetItem["status"].set(pwmMotorEventItem["status"].as<int>());
    L_allPwmEventList.add(D_OnePwmMotorSetItem);
  }
  return D_pwmMotorListItem.as<JsonObject>();
}

JsonObject BuildPeristalticMotorEventJSONItem(JsonArray peristalticMotorEventList){
  JsonObject D_AllPeristalticMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  DynamicJsonDocument D_PeristalticMotorListItem(5000);
  JsonArray L_allPeristalticEventList = D_PeristalticMotorListItem.createNestedArray("peristaltic_motor_list");
  for (JsonVariant peristalticMotorEventItem : peristalticMotorEventList) {
    String peristalticID = peristalticMotorEventItem["id"].as<String>();
    if (!D_AllPeristalticMotorSetting.containsKey(peristalticID)) {
      continue;
    }
    DynamicJsonDocument D_onePeristalticMotorSetItem(500);
    D_onePeristalticMotorSetItem["peristaltic_motor"]["index"].set(D_AllPeristalticMotorSetting[peristalticID]["index"].as<int>());
    D_onePeristalticMotorSetItem["peristaltic_motor"]["title"].set(D_AllPeristalticMotorSetting[peristalticID]["title"].as<String>());
    D_onePeristalticMotorSetItem["peristaltic_motor"]["desp"].set(D_AllPeristalticMotorSetting[peristalticID]["desp"].as<String>());
    D_onePeristalticMotorSetItem["finish_time"].set(-1);
    D_onePeristalticMotorSetItem["status"].set(peristalticMotorEventItem["status"].as<int>());
    D_onePeristalticMotorSetItem["time"].set(peristalticMotorEventItem["time"].as<float>());
    D_onePeristalticMotorSetItem["until"].set(peristalticMotorEventItem["until"].as<String>());
    L_allPeristalticEventList.add(D_onePeristalticMotorSetItem);
  }
  return D_PeristalticMotorListItem.as<JsonObject>();
}

JsonObject BuildSpectrophotometerEventJSONItem(JsonArray spectrophotometerEventList){
  JsonObject D_AllSpectrophotometerSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  DynamicJsonDocument D_SpectrophotometerListItem(5000);
  JsonArray L_allSpectrophotometerEventList = D_SpectrophotometerListItem.createNestedArray("spectrophotometer_list");
  for (JsonVariant spectrophotometerEventItem : spectrophotometerEventList) {
    String spectrophotometerID = spectrophotometerEventItem["id"].as<String>();
    if (!D_AllSpectrophotometerSetting.containsKey(spectrophotometerID)) {
      continue;
    }
    DynamicJsonDocument D_oneSpectrophotometerSetItem(500);
    D_oneSpectrophotometerSetItem["spectrophotometer"]["index"].set(D_AllSpectrophotometerSetting[spectrophotometerID]["index"].as<int>());
    D_oneSpectrophotometerSetItem["spectrophotometer"]["title"].set(D_AllSpectrophotometerSetting[spectrophotometerID]["title"].as<String>());
    D_oneSpectrophotometerSetItem["spectrophotometer"]["desp"].set(D_AllSpectrophotometerSetting[spectrophotometerID]["desp"].as<String>());
    D_oneSpectrophotometerSetItem["id"].set(spectrophotometerEventItem["id"].as<String>());
    D_oneSpectrophotometerSetItem["finish_time"].set(-1);
    D_oneSpectrophotometerSetItem["gain"].set(spectrophotometerEventItem["gain"].as<String>());
    D_oneSpectrophotometerSetItem["value_name"].set(spectrophotometerEventItem["value_name"].as<String>());
    D_oneSpectrophotometerSetItem["channel"].set(spectrophotometerEventItem["channel"].as<String>());
    D_oneSpectrophotometerSetItem["target"].set(spectrophotometerEventItem["target"].as<int>());
    D_oneSpectrophotometerSetItem["dilution"].set(spectrophotometerEventItem["dilution"].as<double>());
    L_allSpectrophotometerEventList.add(D_oneSpectrophotometerSetItem);
  }
  return D_SpectrophotometerListItem.as<JsonObject>();
}

JsonObject BuildEventJSONItem(String S_thisEventKey, JsonObject D_thisEventSetting) {
  DynamicJsonDocument eventGroupItem(10000);
  eventGroupItem[S_thisEventKey]["title"].set(D_thisEventSetting["title"].as<String>());
  eventGroupItem[S_thisEventKey]["desp"].set(D_thisEventSetting["desp"].as<String>());
  eventGroupItem[S_thisEventKey]["finish_time"].set(-1);
  JsonArray L_eventList = eventGroupItem[S_thisEventKey].createNestedArray("event_list");

  JsonObject D_pwmMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  JsonObject D_peristalticMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];

  for (JsonVariant eventItem : D_thisEventSetting["event"].as<JsonArray>()) {
    if (eventItem.containsKey("pwm_motor_list")) {
      JsonObject PWMMotorEventJSON = BuildPWNMotorEventJSONItem(
        eventItem["pwm_motor_list"].as<JsonArray>()
      );
      L_eventList.add(PWMMotorEventJSON);
    }
    else if (eventItem.containsKey("peristaltic_motor_list")) {

      JsonObject PeristalticMotorEventItemJSON = BuildPeristalticMotorEventJSONItem(
        eventItem["peristaltic_motor_list"].as<JsonArray>()
      );
      L_eventList.add(PeristalticMotorEventItemJSON);
    }
    else if (eventItem.containsKey("spectrophotometer_list")) {
      JsonObject SpectrophotometerEventItemJSON = BuildSpectrophotometerEventJSONItem(
        eventItem["spectrophotometer_list"].as<JsonArray>()
      );
      L_eventList.add(SpectrophotometerEventItemJSON);
    }
    //TODO 目前ph計只需一組，先以寫死方式運作
    else if (eventItem.containsKey("ph_meter")) {
      DynamicJsonDocument phMeterListItem(500);
      JsonArray L_phMeterEventList = phMeterListItem.createNestedArray("ph_meter");
      DynamicJsonDocument phMeterItem(200);
      phMeterItem["id"].set("A");
      L_phMeterEventList.add(phMeterItem);
      L_eventList.add(phMeterListItem.as<JsonObject>());
    }
    else if (eventItem.containsKey("wait")) {
      DynamicJsonDocument WaitItem(200);
      WaitItem["wait"].set(eventItem["wait"].as<int>());
      L_eventList.add(WaitItem.as<JsonObject>());
    }
    else if (eventItem.containsKey("upload")) {
      DynamicJsonDocument UploadItem(200);
      UploadItem.createNestedObject("upload");
      L_eventList.add(UploadItem.as<JsonObject>());
    }
  }
  return eventGroupItem.as<JsonObject>();
}

JsonObject BuildStepJSONItem(String S_thisStepKey, JsonObject D_thisStepSetting) {
  DynamicJsonDocument stepItem(50000);
  stepItem[S_thisStepKey]["title"].set(D_thisStepSetting["title"].as<String>());
  stepItem[S_thisStepKey]["desp"].set(D_thisStepSetting["desp"].as<String>());
  stepItem[S_thisStepKey]["finish_time"].set(-1);
  JsonArray L_eventGroupList = stepItem[S_thisStepKey].createNestedArray("event_group_list");
  JsonObject D_EventGroupSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  for (JsonVariant EventGroupID : D_thisStepSetting["steps"].as<JsonArray>()) {
    String EventGroupIDString = EventGroupID.as<String>();
    if (D_EventGroupSetting.containsKey(EventGroupIDString)) {
      JsonObject EventJSONItemJSON = BuildEventJSONItem(EventGroupIDString, D_EventGroupSetting[EventGroupIDString].as<JsonObject>());
      L_eventGroupList.add(EventJSONItemJSON);
    }
  }
  return stepItem.as<JsonObject>();
}

JsonObject BuildPipelineJSONItem(String S_thisPipelineKey, JsonObject D_thisPipelineSetting) {
  DynamicJsonDocument pipelineItem(100000);
  pipelineItem["title"].set(D_thisPipelineSetting["title"].as<String>());
  pipelineItem["desp"].set(D_thisPipelineSetting["desp"].as<String>());
  pipelineItem["finish_time"].set(-1);
  time_t nowTime = now();
  char datetimeChar[30];
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  pipelineItem["create_time"].set(nowTime);
  pipelineItem["pool"].set(D_thisPipelineSetting["pool"].as<String>());
  JsonArray L_stepList = pipelineItem.createNestedArray("step_list");
  JsonObject D_StepSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];

  for (JsonVariant StepID : D_thisPipelineSetting["steps"].as<JsonArray>()) {
    String StepIDString = StepID.as<String>();
    if (D_StepSetting.containsKey(StepIDString)) {
      JsonObject StepItemJSON = BuildStepJSONItem(StepIDString, D_StepSetting[StepIDString].as<JsonObject>());
      L_stepList.add(StepItemJSON);
    }
  }
  return pipelineItem.as<JsonObject>();
}


/**
 * @brief 強制停止所有動作
 * 
 */
void ws_StopAllActionTask(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  Machine_Ctrl.StopDeviceAndINIT();
  Machine_Ctrl.CleanAllStepTask();
  Machine_Ctrl.SetLog(
    2,
    "儀器排程被強制停止",
    "儀器排程被強制停止" ,
    server, NULL
  );
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
  client->binary(returnString);
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
  client->binary(returnString);
}


void ws_GetNowStatus(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();

  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("OK");
  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["parameter"]["message"].set("OK");
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

//!LOG相關API

void ws_GetLogs(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  int MaxLogNum = 10;
  if ((*D_QueryParameter).containsKey("max")) {
    MaxLogNum = (*D_QueryParameter)["max"].as<int>();
  }
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("OK");
  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("LogHistory");
  D_baseInfoJSON["action"]["method"].set("Update");

  JsonArray logsArray = D_baseInfoJSON["parameter"].createNestedArray("logs");

  JsonArray logSaves = (*Machine_Ctrl.DeviceLogSave)["Log"].as<JsonArray>();

  int lastIndex = logSaves.size() - 1;
  int startIndex = lastIndex - MaxLogNum;
  if (startIndex < 0) {
    startIndex = 0;
  }
  for (int indexChose=startIndex;indexChose<lastIndex;indexChose++) {
    logsArray.add(logSaves[indexChose].as<JsonObject>());
  }

  // int logCount = 0;
  // for (JsonVariant logItem : (*Machine_Ctrl.DeviceLogSave)["Log"].as<JsonArray>()) {
  //   if (logCount >= MaxLogNum) {
  //     break;
  //   }
  //   logsArray.add(
  //     logItem.as<JsonObject>()
  //   );
  //   logCount ++;
  // }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

//!機器步驟執行相關API

void ws_GetNRunHistoryInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_loadedAction = Machine_Ctrl.loadedAction->as<JsonObject>();
  D_baseInfoJSON["status"].set("OK");
  D_baseInfoJSON["action"]["target"].set("RunHistory");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_loadedAction);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

//!Pool結果資料

void ws_GetAllPoolData(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  (*D_baseInfo)["status"].set("OK");
  (*D_baseInfo)["cmd"].set("poolData");
  (*D_baseInfo)["action"]["target"].set("PoolData");
  (*D_baseInfo)["action"]["method"].set("Update");
  (*D_baseInfo)["action"]["message"].set("OK");
  (*D_baseInfo)["action"]["status"].set("OK");
  if (!(*D_baseInfo)["action"].containsKey("message")) {
    (*D_baseInfo)["action"]["message"].set("獲得各蝦池最新感測器資料");
  }

  for (JsonPair D_poolItem : (*Machine_Ctrl.spiffs.DeviceSetting)["pools"].as<JsonObject>()) {
    if ((*Machine_Ctrl.sensorDataSave)[D_poolItem.key()].containsKey("Data_datetime") == false) {
      (*Machine_Ctrl.sensorDataSave)[D_poolItem.key()]["Data_datetime"].set("");
    }
    (*D_baseInfo)["parameter"][D_poolItem.key()].set((*Machine_Ctrl.sensorDataSave)[D_poolItem.key()]);
  }
  String returnString;
  serializeJsonPretty((*D_baseInfo), returnString);
  client->binary(returnString);
  (*D_baseInfo).clear();
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
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("更新事件組設定完畢");
    D_baseInfoJSON["action"]["target"].set("Pool");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "更新事件組設定完畢",
      "事件組名稱: " + D_oldConfig["title"].as<String>(),
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "更新蝦池設定失敗",
      "找不到蝦池設定: " + TargetName,
      NULL, client
    );
  }

}

void ws_DeletePoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  if (D_pools.containsKey(TargetName)) {
    D_pools.remove(TargetName);
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("刪除蝦池設定完畢");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Pool";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "刪除蝦池設定完畢",
      "蝦池設定ID: " + TargetName,
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "更新蝦池設定失敗",
      "找不到蝦池設定: " + TargetName,
      NULL, client
    );
  }
}

void ws_GetPoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Pools Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  if (D_pools.containsKey(TargetName)) {
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("查詢蝦池設定完畢");
    D_baseInfoJSON["action"]["target"].set("Pool");
    D_baseInfoJSON["action"]["method"].set("Get");
    D_baseInfoJSON["parameter"][TargetName].set(D_pools[TargetName]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->binary(returnString);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "查詢蝦池設定失敗",
      "找不到蝦池設定: " + TargetName,
      NULL, client
    );
  }
}

void ws_GetAllPoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
  D_baseInfoJSON["parameter"].set(D_pools);

  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("查詢所有蝦池設定完畢");
  D_baseInfoJSON["action"]["target"].set("Pool");
  D_baseInfoJSON["action"]["method"].set("Update");
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

void ws_AddNewPoolInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title")) {
    Machine_Ctrl.SetLog(
      1,
      "新增蝦池設定失敗",
      "title 參數為必要項目",
      NULL, client
    );
  } else {
    char random_name[16];
    uint8_t random_bytes[8];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    for (int i = 0; i < sizeof(random_bytes); i++) {
      sprintf(&random_name[i*2], "%02x", random_bytes[i]);
    }
    JsonObject D_pools = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pools"];
    D_pools[String(random_name)]["title"].set(D_newConfig["title"].as<String>());
    D_pools[String(random_name)]["desp"].set(D_newConfig["desp"].as<String>());
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["target"].set("Pool");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][String(random_name)].set(D_pools[String(random_name)]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "新增蝦池設定: "+D_newConfig["title"].as<String>(),
      "說明: " + D_newConfig["desp"].as<String>(),
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
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
    Machine_Ctrl.SetLog(
      2,
      "測試分光光度計設定失敗",
      "儀器忙碌中，請稍後再試",
      NULL, client
    );
  }
  else if (D_spectrophotometer.containsKey(TargetName)) {
    DynamicJsonDocument actionItem(10000);
    actionItem["title"].set("分光光度計測試");
    actionItem["desp"].set("分光光度計測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["pool"].set("temp-pool");
    actionItem["finish_time"].set(-1);
    actionItem["data_type"] = String("TEST");

    JsonArray L_stepList = actionItem.createNestedArray("step_list");

    DynamicJsonDocument stepItem(5000);
    stepItem["SpectrophotometerTest"]["title"].set("分光光度計測試");
    stepItem["SpectrophotometerTest"]["desp"].set("分光光度計測試");
    stepItem["SpectrophotometerTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["SpectrophotometerTest"].createNestedArray("event_group_list");

    DynamicJsonDocument eventGroupItem(1000);
    eventGroupItem["SpectrophotometerTest"]["title"].set("分光光度計測試");
    eventGroupItem["SpectrophotometerTest"]["desp"].set("分光光度計測試");
    eventGroupItem["SpectrophotometerTest"]["finish_time"].set(-1);
    JsonArray L_eventList = eventGroupItem["SpectrophotometerTest"].createNestedArray("event_list");

    DynamicJsonDocument spectrophotometerInfo(1000);

    JsonObject D_spectrophotometerSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"][TargetName];

    
    DynamicJsonDocument eventItem(5000);
    JsonArray L_spectrophotometer = eventItem.createNestedArray("spectrophotometer_list");

    DynamicJsonDocument spectrophotometerSet(500);
    spectrophotometerSet["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet["spectrophotometer"]["desp"].set(D_spectrophotometerSetting["desp"].as<String>());
    spectrophotometerSet["gain"].set("1X");
    spectrophotometerSet["value_name"].set("sensor_test_1X");
    spectrophotometerSet["measurement_time"].set(-1);
    spectrophotometerSet["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet);

    DynamicJsonDocument spectrophotometerSet_2(500);
    spectrophotometerSet_2["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_2["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_2["spectrophotometer"]["desp"].set(D_spectrophotometerSetting["desp"].as<String>());
    spectrophotometerSet_2["gain"].set("2X");
    spectrophotometerSet_2["value_name"].set("sensor_test_2X");
    spectrophotometerSet_2["measurement_time"].set(-1);
    spectrophotometerSet_2["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_2);

    DynamicJsonDocument spectrophotometerSet_3(500);
    spectrophotometerSet_3["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_3["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_3["spectrophotometer"]["desp"].set(D_spectrophotometerSetting["desp"].as<String>());
    spectrophotometerSet_3["gain"].set("4X");
    spectrophotometerSet_3["value_name"].set("sensor_test_4X");
    spectrophotometerSet_3["measurement_time"].set(-1);
    spectrophotometerSet_3["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_3);

    DynamicJsonDocument spectrophotometerSet_4(500);
    spectrophotometerSet_4["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_4["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_4["spectrophotometer"]["desp"].set(D_spectrophotometerSetting["desp"].as<String>());
    spectrophotometerSet_4["gain"].set("8X");
    spectrophotometerSet_4["value_name"].set("sensor_test_8X");
    spectrophotometerSet_4["measurement_time"].set(-1);
    spectrophotometerSet_4["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_4);

    DynamicJsonDocument spectrophotometerSet_5(500);
    spectrophotometerSet_5["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_5["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_5["spectrophotometer"]["desp"].set(D_spectrophotometerSetting["desp"].as<String>());
    spectrophotometerSet_5["gain"].set("48X");
    spectrophotometerSet_5["value_name"].set("sensor_test_48X");
    spectrophotometerSet_5["measurement_time"].set(-1);
    spectrophotometerSet_5["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_5);

    DynamicJsonDocument spectrophotometerSet_6(500);
    spectrophotometerSet_6["spectrophotometer"]["index"].set(D_spectrophotometerSetting["index"].as<int>());
    spectrophotometerSet_6["spectrophotometer"]["title"].set(D_spectrophotometerSetting["title"].as<String>());
    spectrophotometerSet_6["spectrophotometer"]["desp"].set(D_spectrophotometerSetting["desp"].as<String>());
    spectrophotometerSet_6["gain"].set("96X");
    spectrophotometerSet_6["value_name"].set("sensor_test_96X");
    spectrophotometerSet_6["measurement_time"].set(-1);
    spectrophotometerSet_6["finish_time"].set(-1);
    L_spectrophotometer.add(spectrophotometerSet_6);

    L_eventList.add(eventItem);
    L_eventGroupList.add(eventGroupItem);
    L_stepList.add(stepItem);

    // String settingString;
    // serializeJson(actionItem,settingString);
    // actionItem.clear();
    // Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.LOAD__ACTION(actionItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();
  } else {
    Machine_Ctrl.SetLog(
      1,
      "測試分光光度計設定失敗",
      "找不到分光光度計設定: " + TargetName,
      NULL, client
    );

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
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("刪除分光光度計設定完畢");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Spectrophotometer";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "刪除分光光度計設定完畢",
      "分光光度計設定ID: " + TargetName,
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "刪除分光光度計設定失敗",
      "找不到分光光度計設定: " + TargetName,
      NULL, client
    );
  }

}

void ws_PatchSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Patch Spectrophotometer Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  if (D_spectrophotometer.containsKey(TargetName)) {
    JsonObject D_newConfig = D_FormData->as<JsonObject>();
    D_spectrophotometer[TargetName].set(D_newConfig);
    // JsonObject D_oldConfig = D_spectrophotometer[TargetName];
    // for (JsonPair newConfigItem : D_newConfig) {
    //   if (D_oldConfig[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
    //     D_oldConfig[newConfigItem.key()].set(newConfigItem.value().as<String>());
    //   }
    // }

    serializeJsonPretty(D_newConfig, Serial);

    D_baseInfoJSON["parameter"][TargetName].set(D_spectrophotometer[TargetName].as<JsonObject>());

    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("更新分光光度計設定完畢");
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJson(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "更新分光光度計設定完畢",
      "分光光度計設定名稱: " + D_spectrophotometer[TargetName]["title"].as<String>(),
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "更新分光光度計設定失敗",
      "找不到分光光度計設定: " + TargetName,
      NULL, client
    );
  }
}

void ws_GetSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Spectrophotometer Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  if (D_spectrophotometer.containsKey(TargetName)) {
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("查詢分光光度計設定完畢");
    D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_spectrophotometer[TargetName]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->binary(returnString);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "查詢分光光度計設定失敗",
      "找不到分光光度計設定: " + TargetName,
      NULL, client
    );
  }
}

void ws_GetAllSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_spectrophotometer = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["spectrophotometer"];
  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("查詢所有分光光度計設定完畢");
  D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_spectrophotometer);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

void ws_AddNewSpectrophotometerInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    Machine_Ctrl.SetLog(
      1,
      "新增分光光度計失敗",
      "index 與 title 參數為必要項目",
      NULL, client
    );
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>3) {
      Machine_Ctrl.SetLog(
        1,
        "新增分光光度計失敗",
        "index需介於0~3(含)",
        NULL, client
      );
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
      D_spectrophotometer[String(random_name)]["desp"].set(D_newConfig["desp"].as<String>());
      D_baseInfoJSON["action"]["status"].set("OK");
      D_baseInfoJSON["action"]["target"].set("Spectrophotometer");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_spectrophotometer[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      server->binaryAll(returnString);

      Machine_Ctrl.SetLog(
        5,
        "新增分光光度計設定: "+D_newConfig["title"].as<String>(),
        "說明: " + D_newConfig["desp"].as<String>(),
        server, NULL
      );

      String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
      if (RewriteConfigResult == "BUSY") {
        Machine_Ctrl.SetLog(
          1,
          "機器讀寫忙碌中", "請稍後再試",
          NULL, client
        );
      }
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
    Machine_Ctrl.SetLog(
      2,
      "測試蠕動馬達設定失敗",
      "儀器忙碌中，請稍後再試",
      NULL, client
    );
  }
  else if (D_peristaltic_motor.containsKey(TargetName)) {
    DynamicJsonDocument actionItem(10000);
    actionItem["title"].set("蠕動馬達測試");
    actionItem["desp"].set("蠕動馬達測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["pool"].set("temp-pool");
    actionItem["finish_time"].set(-1);
    actionItem["data_type"] = String("TEST");

    JsonArray L_stepList = actionItem.createNestedArray("step_list");

    DynamicJsonDocument stepItem(5000);
    stepItem["peristalticMotorTest"]["title"].set("蠕動馬達測試");
    stepItem["peristalticMotorTest"]["desp"].set("蠕動馬達測試");
    stepItem["peristalticMotorTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["peristalticMotorTest"].createNestedArray("event_group_list");

    DynamicJsonDocument eventGroupItem(1000);
    eventGroupItem["peristalticMotorTest"]["title"].set("蠕動馬達測試");
    eventGroupItem["peristalticMotorTest"]["desp"].set("蠕動馬達測試");
    eventGroupItem["peristalticMotorTest"]["finish_time"].set(-1);
    JsonArray L_eventList = eventGroupItem["peristalticMotorTest"].createNestedArray("event_list");

    DynamicJsonDocument peristalticMotorInfo(1000);

    JsonObject D_peristalticMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"][TargetName];

    
    DynamicJsonDocument eventItem_1(500);
    JsonArray L_peristalticMotorList_1 = eventItem_1.createNestedArray("peristaltic_motor_list");
    DynamicJsonDocument peristalticMotorSet_1(500);
    
    peristalticMotorSet_1["peristaltic_motor"]["index"].set(D_peristalticMotorSetting["index"].as<int>());
    peristalticMotorSet_1["peristaltic_motor"]["title"].set(D_peristalticMotorSetting["title"].as<String>());
    peristalticMotorSet_1["peristaltic_motor"]["desp"].set(D_peristalticMotorSetting["desp"].as<String>());
    peristalticMotorSet_1["finish_time"].set(-1);
    peristalticMotorSet_1["status"].set(1);
    peristalticMotorSet_1["time"].set(1.0);

    DynamicJsonDocument eventItem_2(500);
    JsonArray L_peristalticMotorList_2 = eventItem_2.createNestedArray("peristaltic_motor_list");
    DynamicJsonDocument peristalticMotorSet_2(500);
    
    peristalticMotorSet_2["peristaltic_motor"]["index"].set(D_peristalticMotorSetting["index"].as<int>());
    peristalticMotorSet_2["peristaltic_motor"]["title"].set(D_peristalticMotorSetting["title"].as<String>());
    peristalticMotorSet_2["peristaltic_motor"]["desp"].set(D_peristalticMotorSetting["desp"].as<String>());
    peristalticMotorSet_2["finish_time"].set(-1);
    peristalticMotorSet_2["status"].set(-1);
    peristalticMotorSet_2["time"].set(1.0);


    L_peristalticMotorList_1.add(peristalticMotorSet_1);
    L_eventList.add(eventItem_1);
    L_peristalticMotorList_2.add(peristalticMotorSet_2);
    L_eventList.add(eventItem_2);
    L_eventGroupList.add(eventGroupItem);
    L_stepList.add(stepItem);

    // String settingString;
    // serializeJson(actionItem,settingString);
    // actionItem.clear();
    // Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.LOAD__ACTION(actionItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();
  } else {
    Machine_Ctrl.SetLog(
      1,
      "測試蠕動馬達設定失敗",
      "找不到蠕動馬達設定: " + TargetName,
      NULL, client
    );
  }

}

void ws_GetPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "GetPeristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("查詢蠕動馬達設定完畢");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->binary(returnString);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "查詢蠕動馬達設定失敗",
      "找不到蠕動馬達設定: " + TargetName,
      NULL, client
    );
  }
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

    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("更新事件組設定完畢");
    D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "更新事件組設定完畢",
      "事件組設定名稱: " + D_oldConfig["title"].as<String>(),
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "更新蠕動馬達設定失敗",
      "找不到蠕動馬達設定: " + TargetName,
      NULL, client
    );
  }

}

void ws_DeletePeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket", "Get Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor_list = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_peristaltic_motor_list.containsKey(TargetName)) {
    D_peristaltic_motor_list.remove(TargetName);
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("刪除蠕動馬達設定完畢");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "PeristalticMotor";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "刪除蠕動馬達設定完畢",
      "蠕動馬達設定ID: " + TargetName,
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      2,
      "刪除蠕動馬達設定失敗",
      "找不到蠕動馬達設定: " + TargetName,
      NULL, client
    );
  }
}

void ws_GetAllPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("查詢所有蠕動馬達設定完畢");
  D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_peristaltic_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

void ws_AddNewPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    Machine_Ctrl.SetLog(
      2,
      "新增蠕動馬達失敗",
      "index 與 title 參數為必要項目",
      NULL, client
    );
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>7) {
      Machine_Ctrl.SetLog(
        2,
        "新增蠕動馬達失敗",
        "index需介於0~7(含)",
        NULL, client
      );
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
      D_peristaltic_motor[String(random_name)]["desp"].set(D_newConfig["desp"].as<String>());
      D_baseInfoJSON["action"]["status"].set("OK");
      D_baseInfoJSON["action"]["target"].set("PeristalticMotor");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_peristaltic_motor[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      server->binaryAll(returnString);

      Machine_Ctrl.SetLog(
        5,
        "新增蠕動馬達設定: "+D_newConfig["title"].as<String>(),
        "說明: " + D_newConfig["desp"].as<String>(),
        server, NULL
      );

      String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
      if (RewriteConfigResult == "BUSY") {
        Machine_Ctrl.SetLog(
          1,
          "機器讀寫忙碌中", "請稍後再試",
          NULL, client
        );
      }
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
    Machine_Ctrl.SetLog(
      2,
      "測試伺服馬達設定失敗",
      "儀器忙碌中，請稍後再試",
      NULL, client
    );
  }
  else if (D_pwm_motor.containsKey(TargetName)) {
    DynamicJsonDocument actionItem(10000);
    actionItem["title"].set("流程-伺服馬達測試");
    actionItem["desp"].set("測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["pool"].set("temp-pool");
    actionItem["create_time"].set(nowTime);
    actionItem["finish_time"].set(-1);
    actionItem["data_type"] = String("TEST");

    JsonArray L_stepList = actionItem.createNestedArray("step_list");

    DynamicJsonDocument stepItem(5000);
    stepItem["pwmMotorTest"]["title"].set("步驟-伺服馬達測試");
    stepItem["pwmMotorTest"]["desp"].set("測試");
    stepItem["pwmMotorTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["pwmMotorTest"].createNestedArray("event_group_list");

    DynamicJsonDocument eventGroupItem(1000);
    eventGroupItem["pwmMotorTest"]["title"].set("事件組-伺服馬達測試");
    eventGroupItem["pwmMotorTest"]["desp"].set("測試");
    eventGroupItem["pwmMotorTest"]["finish_time"].set(-1);
    JsonArray L_eventList = eventGroupItem["pwmMotorTest"].createNestedArray("event_list");

    DynamicJsonDocument pwmMotorInfo(1000);

    JsonObject D_pwmMotorSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"][TargetName];

    
    DynamicJsonDocument eventItem_1(500);
    JsonArray L_pwnMotorList_1 = eventItem_1.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_1(500);
    
    pwmMotorSet_1["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_1["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_1["pwn_motor"]["desp"].set(D_pwmMotorSetting["desp"].as<String>());
    pwmMotorSet_1["finish_time"].set(-1);
    pwmMotorSet_1["status"].set(0);

    DynamicJsonDocument eventItem_2(500);
    JsonArray L_pwnMotorList_2 = eventItem_2.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_2(500);
    
    pwmMotorSet_2["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_2["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_2["pwn_motor"]["desp"].set(D_pwmMotorSetting["desp"].as<String>());
    pwmMotorSet_2["finish_time"].set(-1);
    pwmMotorSet_2["status"].set(90);

    DynamicJsonDocument eventItem_3(500);
    JsonArray L_pwnMotorList_3 = eventItem_3.createNestedArray("pwm_motor_list");
    DynamicJsonDocument pwmMotorSet_3(500);
    pwmMotorSet_3["pwn_motor"]["index"].set(D_pwmMotorSetting["index"].as<int>());
    pwmMotorSet_3["pwn_motor"]["title"].set(D_pwmMotorSetting["title"].as<String>());
    pwmMotorSet_3["pwn_motor"]["desp"].set(D_pwmMotorSetting["desp"].as<String>());
    pwmMotorSet_3["finish_time"].set(-1);
    pwmMotorSet_3["status"].set(180);

    L_pwnMotorList_1.add(pwmMotorSet_1);
    L_eventList.add(eventItem_1);
    L_pwnMotorList_2.add(pwmMotorSet_2);
    L_eventList.add(eventItem_2);
    L_pwnMotorList_3.add(pwmMotorSet_3);
    L_eventList.add(eventItem_3);
    L_eventGroupList.add(eventGroupItem);
    JsonObject Reset_All_PWM_Motor_Event = BuildEventJSONItem(
      "Reset_All_PWM_Motor", 
      (*Machine_Ctrl.spiffs.DeviceSetting)["event_group"]["Reset_All_PWM_Motor"].as<JsonObject>()
    );
    L_eventGroupList.add(Reset_All_PWM_Motor_Event);
    L_stepList.add(stepItem);
    // serializeJsonPretty(L_eventGroupList, Serial);

    // String settingString;
    // serializeJson(actionItem,settingString);
    // actionItem.clear();
    // Machine_Ctrl.LOAD__ACTION(settingString);
    // Machine_Ctrl.RUN__LOADED_ACTION();

    Machine_Ctrl.LOAD__ACTION(actionItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();
  } else {
    Machine_Ctrl.SetLog(
      1,
      "測試伺服馬達設定失敗",
      "找不到伺服馬達設定: " + TargetName,
      NULL, client
    );
  }

}

void ws_GetPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "GetPeristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("查詢伺服馬達設定完畢");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->binary(returnString);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "查詢伺服馬達設定失敗",
      "找不到伺服馬達設定: " + TargetName,
      NULL, client
    );
  }
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

    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("更新伺服馬達設定完畢");
    D_baseInfoJSON["action"]["target"].set("PwmMotor");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "更新伺服馬達設定完畢",
      "伺服馬達設定名稱: " + D_oldConfig["title"].as<String>(),
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "更新伺服馬達設定失敗",
      "找不到伺服馬達設定: " + TargetName,
      NULL, client
    );
  }

}

void ws_DeletePwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Delete Peristaltic Motor Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_steps_group.remove(TargetName);
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("刪除伺服馬達設定完畢");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "PwmMotor";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "刪除伺服馬達設定完畢",
      "伺服馬達設定ID: " + TargetName,
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "刪除伺服馬達設定失敗",
      "找不到伺服馬達設定: " + TargetName,
      NULL, client
    );
  }
}

void ws_AddNewPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("index") | !D_newConfig.containsKey("title")) {
    Machine_Ctrl.SetLog(
      1,
      "新增伺服馬達設定失敗",
      "index 與 title 參數為必要項目",
        NULL, client
    );
  } else {
    int newIndex = D_newConfig["index"].as<int>();
    if (newIndex<0 | newIndex>32) {
      Machine_Ctrl.SetLog(
        1,
        "新增伺服馬達設定失敗",
        "index需介於0~31(含)",
        NULL, client
      );
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
      D_pwm_motor[String(random_name)]["desp"].set(D_newConfig["desp"].as<String>());
      D_baseInfoJSON["action"]["status"].set("OK");
      D_baseInfoJSON["action"]["target"].set("PwmMotor");
      D_baseInfoJSON["action"]["method"].set("Update");
      D_baseInfoJSON["parameter"][String(random_name)].set(D_pwm_motor[String(random_name)]);
      String returnString;
      serializeJsonPretty(D_baseInfoJSON, returnString);
      server->binaryAll(returnString);
      Machine_Ctrl.SetLog(
        5,
        "新增伺服馬達設定: "+D_newConfig["title"].as<String>(),
        "說明: " + D_newConfig["desp"].as<String>(),
        server, NULL
      );
      String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
      if (RewriteConfigResult == "BUSY") {
        Machine_Ctrl.SetLog(
          1,
          "機器讀寫忙碌中", "請稍後再試",
          NULL, client
        );
      }
    }
  }
}

void ws_GetAllPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pwm_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("查詢所有伺服馬達設定完畢");
  D_baseInfoJSON["action"]["target"].set("PwmMotor");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_pwm_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}


//!事件組設定相關API

void ws_TestEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Test EVENT, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pwm_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    Machine_Ctrl.SetLog(
      2,
      "測試事件組設定失敗",
      "儀器忙碌中，請稍後再試",
        NULL, client
    );
  }
  else if (D_pwm_motor.containsKey(TargetName)) {
    DynamicJsonDocument actionItem(100000);
    //? 流程設定
    actionItem["title"].set("(流程)事件組測試");
    actionItem["desp"].set("(流程)事件組測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["pool"].set("temp-pool");
    actionItem["data_type"] = String("TEST");
    actionItem["finish_time"].set(-1);

    JsonArray L_stepList = actionItem.createNestedArray("step_list");
    //? 步驟設定
    DynamicJsonDocument stepItem(50000);
    stepItem["pwmMotorTest"]["title"].set("(步驟)事件組測試");
    stepItem["pwmMotorTest"]["desp"].set("(步驟)事件組測試");
    stepItem["pwmMotorTest"]["finish_time"].set(-1);

    JsonArray L_eventGroupList = stepItem["pwmMotorTest"].createNestedArray("event_group_list");

    //? 事件組設定
    DynamicJsonDocument eventGroupItem(10000);
    JsonObject D_thisEventSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"][TargetName];
    JsonObject eventGroupItemJSON = BuildEventJSONItem(TargetName, D_thisEventSetting);

    L_eventGroupList.add(eventGroupItemJSON);
    L_stepList.add(stepItem);

    // String settingString;
    // serializeJson(actionItem,settingString);
    // actionItem.clear();
    // Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.LOAD__ACTION(actionItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();
  } else {
    Machine_Ctrl.SetLog(
      1,
      "測試事件組設定失敗",
      "找不到事件組設定: " + TargetName,
        NULL, client
    );
  }

}

void ws_GetAllEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("查詢所有事件組設定完畢");
  D_baseInfoJSON["action"]["target"].set("Event");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_event_group);
  String returnString;
  serializeJson(D_baseInfoJSON, returnString);
  client->binary(returnString);
  client->binary(String(returnString.length()));
}

void ws_GetEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Event Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  if (D_event_group.containsKey(TargetName)) {
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("查詢事件組設定完畢");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_event_group[TargetName]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->binary(returnString);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "查詢事件組設定失敗",
      "找不到事件組設定: " + TargetName,
        NULL, client
    );
  }
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
    if (D_oldConfig["desp"].as<String>() != D_newConfig["desp"].as<String>()) {
      D_oldConfig["desp"] = D_newConfig["desp"].as<String>();
    }
    D_oldConfig.remove("event");
    D_oldConfig["event"].set(D_newConfig["event"]);
    
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);

    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("更新事件組設定完畢");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "更新事件組設定完畢",
      "事件組設定名稱: " + D_oldConfig["title"].as<String>(),
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "更新事件組設定失敗",
      "找不到事件組設定: " + TargetName,
        NULL, client
    );
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
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("刪除事件組設定完畢");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Event";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "刪除事件組設定完畢",
      "事件組設定ID: " + TargetName,
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "刪除事件組設定失敗",
      "找不到事件組設定: " + TargetName,
        NULL, client
    );
  }
}

void ws_AddNewEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title")) {
    Machine_Ctrl.SetLog(
      1,
      "新增事件組設定失敗",
      "title 參數為必要項目",
        NULL, client
    );

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
    D_event_group[NewIDString]["desp"].set(D_newConfig["desp"].as<String>());
    D_event_group[NewIDString].createNestedArray("event");

    D_baseInfoJSON["parameter"][NewIDString].set(D_event_group[NewIDString]);
    
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("新增事件組設定完畢");
    D_baseInfoJSON["action"]["target"].set("Event");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);

    Machine_Ctrl.SetLog(
      5,
      "新增事件組設定: "+D_newConfig["title"].as<String>(),
      "說明: " + D_newConfig["desp"].as<String>(),
      server, NULL
    );

    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  }
}


//!步驟設定相關API

void ws_TestStep(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Test Step, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    Machine_Ctrl.SetLog(
      2,
      "測試步驟設定失敗",
      "儀器忙碌中，請稍後再試",
        NULL, client
    );
  }
  else if (D_steps_group.containsKey(TargetName)) {
    //? 流程設定
    DynamicJsonDocument actionItem(100000);
    actionItem["title"].set("(流程)步驟測試");
    actionItem["desp"].set("(流程)步驟測試");
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    actionItem["create_time"].set(nowTime);
    actionItem["pool"].set("temp-pool");
    actionItem["finish_time"].set(-1);
    actionItem["data_type"] = String("TEST");

    JsonArray L_stepList = actionItem.createNestedArray("step_list");
    //? 步驟設定
    JsonObject StepJSONItem = BuildStepJSONItem(TargetName, D_steps_group[TargetName].as<JsonObject>());
    L_stepList.add(StepJSONItem);

    // String settingString;
    // serializeJson(actionItem,settingString);
    // actionItem.clear();
    // Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.LOAD__ACTION(actionItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();
  } else {
    Machine_Ctrl.SetLog(
      1,
      "測試步驟設定失敗",
      "找不到步驟設定: "+TargetName,
        NULL, client
    );
  }
}

void ws_GetAllStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  D_baseInfoJSON["parameter"].set(D_event_group);

  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("查詢步驟設定完畢");
  D_baseInfoJSON["action"]["target"].set("Step");
  D_baseInfoJSON["action"]["method"].set("Update");

  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

void ws_GetStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Step Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("查詢步驟設定完畢");
    D_baseInfoJSON["action"]["target"].set("Step");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "查詢步驟設定失敗",
      "找不到步驟設定: "+TargetName,
        NULL, client
    );
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

void ws_DeleteStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Delete Event Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  if (D_steps_group.containsKey(TargetName)) {
    D_steps_group.remove(TargetName);
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("刪除步驟設定完畢");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Step";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);

    Machine_Ctrl.SetLog(
      5,
      "刪除步驟設定完畢",
      "步驟ID: "+TargetName,
        NULL, client
    );

    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "刪除步驟設定失敗",
      "找不到步驟設定: "+TargetName,
        NULL, client
    );
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
    if (D_oldConfig["desp"].as<String>() != D_newConfig["desp"].as<String>()) {
      D_oldConfig["desp"] = D_newConfig["desp"].as<String>();
    }
    D_oldConfig.remove("steps");
    D_oldConfig["steps"].set(D_newConfig["steps"]);
    
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);

    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("修改步驟設定完畢");
    D_baseInfoJSON["action"]["target"].set("Step");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);

    Machine_Ctrl.SetLog(
      5,
      "修改步驟設定完畢",
      "步驟名稱: "+D_newConfig["title"].as<String>(),
        server, NULL
    );

    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "修改步驟設定失敗",
      "找不到步驟設定: "+TargetName,
      NULL, client
    );
  }
}

void ws_AddNewStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title")) {
    Machine_Ctrl.SetLog(
      1,
      "新增流程設定失敗",
      "title 參數為必要項目",
      NULL, client
    );
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
    D_steps_group[NewIDString]["desp"].set(D_newConfig["desp"].as<String>());
    D_steps_group[NewIDString].createNestedArray("steps");
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("新增步驟設定完畢");
    D_baseInfoJSON["action"]["target"].set("Step");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][NewIDString].set(D_steps_group[NewIDString]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
    Machine_Ctrl.SetLog(
      5,
      "新增步驟設定完畢",
      "名稱: "+D_steps_group[NewIDString]["title"].as<String>(),
      server, NULL
    );
    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  }
}


//!流程設定相關API

void RunAllPoolDataGetTask(void* parameter) 
{
  String orderString = *((String*)parameter);
  DynamicJsonDocument PipelineItem(100000);
  JsonObject PipelineJSONItem;
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];

  Serial.println("132: "+orderString);
  int splitIndex = -1;
  int prevSpliIndex = 0;
  splitIndex = orderString.indexOf(',', prevSpliIndex);
  String noInfoString = "";
  while (splitIndex != -1) {
    String poolChose = orderString.substring(prevSpliIndex, splitIndex);
    if (D_pipeline.containsKey(poolChose)) {
      PipelineJSONItem = BuildPipelineJSONItem(poolChose, D_pipeline[poolChose].as<JsonObject>());
      PipelineItem.set(PipelineJSONItem);
      PipelineItem["data_type"] = String("RUN");
      Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
      Machine_Ctrl.RUN__LOADED_ACTION();
      while (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
        vTaskDelay(100/portTICK_PERIOD_MS);
      }
      PipelineItem.clear();
    }
    prevSpliIndex = splitIndex + 1;
    splitIndex = orderString.indexOf(',', prevSpliIndex);
  }
  String lastPool = orderString.substring(prevSpliIndex);
  Serial.println("456: "+lastPool);
  if (D_pipeline.containsKey(lastPool)) {
    PipelineJSONItem = BuildPipelineJSONItem(lastPool, D_pipeline[lastPool].as<JsonObject>());
    PipelineItem.set(PipelineJSONItem);
    PipelineItem["data_type"] = String("RUN");
    Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();
    while (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
    PipelineItem.clear();
  }


  // PipelineJSONItem = BuildPipelineJSONItem("pool_1_all_data_get", D_pipeline["pool_1_all_data_get"].as<JsonObject>());
  // PipelineItem.set(PipelineJSONItem);
  // PipelineItem["data_type"] = String("RUN");
  // Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
  // Machine_Ctrl.RUN__LOADED_ACTION();
  // while (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
  //   vTaskDelay(100/portTICK_PERIOD_MS);
  // }
  // PipelineItem.clear();

  // PipelineJSONItem = BuildPipelineJSONItem("pool_2_all_data_get", D_pipeline["pool_2_all_data_get"].as<JsonObject>());
  // PipelineItem.set(PipelineJSONItem);
  // PipelineItem["data_type"] = String("RUN");
  // Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
  // Machine_Ctrl.RUN__LOADED_ACTION();
  // while (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
  //   vTaskDelay(100/portTICK_PERIOD_MS);
  // }
  // PipelineItem.clear();

  // PipelineJSONItem = BuildPipelineJSONItem("pool_3_all_data_get", D_pipeline["pool_3_all_data_get"].as<JsonObject>());
  // PipelineItem.set(PipelineJSONItem);
  // PipelineItem["data_type"] = String("RUN");
  // Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
  // Machine_Ctrl.RUN__LOADED_ACTION();
  // while (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
  //   vTaskDelay(100/portTICK_PERIOD_MS);
  // }
  // PipelineItem.clear();

  // PipelineJSONItem = BuildPipelineJSONItem("pool_4_all_data_get", D_pipeline["pool_4_all_data_get"].as<JsonObject>());
  // PipelineItem.set(PipelineJSONItem);
  // PipelineItem["data_type"] = String("RUN");
  // Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
  // Machine_Ctrl.RUN__LOADED_ACTION();

  vTaskDelete(NULL);
}

//? [GET]/api/Pipeline/pool_all_data_get/RUN 這支API比較特別，目前是寫死的
//? 目的在於執行時，他會依序執行所有池的資料，每池檢測完後丟出一次NewData
void ws_RunAllPoolPipeline(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  ESP_LOGD("websocket API", "執行所有檢測流程所需的排程");
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    Machine_Ctrl.SetLog(
      2,
      "執行流程設定失敗",
      "儀器忙碌中，請稍後再試",
      NULL, client
    );
  }
  else {
    ESP_LOGI("ws_RunAllPoolPipeline","RUN");

    if ((*D_QueryParameter).containsKey("order")) {
      String orderString = (*D_QueryParameter)["order"].as<String>();
      if (orderString.length() == 0) {
        Machine_Ctrl.SetLog(
          1,
          "執行蝦池數值檢測流程失敗",
          "order參數不得為空",
          NULL, client
        );
      } else {
        //* 分割order文字
        int splitIndex = -1;
        int prevSpliIndex = 0;
        splitIndex = orderString.indexOf(',', prevSpliIndex);
        String noInfoString = "";
        while (splitIndex != -1) {
          String token = orderString.substring(prevSpliIndex, splitIndex);
          Serial.println(token);
          if (!(*Machine_Ctrl.spiffs.DeviceSetting)["pipeline"].containsKey(token)) {
            noInfoString += token + ",";
          }
          prevSpliIndex = splitIndex + 1;
          splitIndex = orderString.indexOf(',', prevSpliIndex);
        }
        String lastToken = orderString.substring(prevSpliIndex);
        if (!(*Machine_Ctrl.spiffs.DeviceSetting)["pipeline"].containsKey(lastToken)) {
          noInfoString += lastToken;
        }

        
        if (noInfoString.length()!=0) {
          Machine_Ctrl.SetLog(
            1,
            "執行蝦池數值檢測流程失敗",
            "找不到以下設定: "+noInfoString,
            NULL, client
          );
        } else {
          xTaskCreate(
            RunAllPoolDataGetTask, "ALL_DATA_GET",
            10000, (void*)&orderString, configMAX_PRIORITIES-1, NULL
          );

          //? 這邊的code試給NodeRed判斷用的，讓他知道他觸發的排程有正常被執行
          D_baseInfoJSON["action"]["message"].set("觸發排程成功");
          D_baseInfoJSON["action"]["status"].set("OK");
          String returnString;
          serializeJsonPretty(D_baseInfoJSON, returnString);
          server->binaryAll(returnString);
        }
      }
    } else {
      Machine_Ctrl.SetLog(
        1,
        "執行蝦池數值檢測流程失敗",
        "API需要order參數",
        NULL, client
      );
    }
  }
}


void ws_RunPipeline(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "RUN Pipeline, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {

    Machine_Ctrl.SetLog(
      2,
      "執行流程設定失敗",
      "儀器忙碌中，請稍後再試",
      NULL, client
    );
  }
  else if (D_pipeline.containsKey(TargetName)) {
    //? 流程設定
    DynamicJsonDocument PipelineItem(100000);
    JsonObject PipelineJSONItem = BuildPipelineJSONItem(TargetName, D_pipeline[TargetName].as<JsonObject>());
    PipelineItem.set(PipelineJSONItem);
    PipelineItem["data_type"] = String("RUN");
    Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();


    //? 這邊的code試給NodeRed判斷用的，讓他知道他觸發的排程有正常被執行
    D_baseInfoJSON["action"]["message"].set("觸發排程成功");
    D_baseInfoJSON["action"]["status"].set("OK");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "執行流程失敗",
      "找不到流程設定: "+TargetName,
      NULL, client
    );
  }

}

void ws_v2_RunPipeline(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  //? 先判斷儀器是否空閒
  //! 注意，這邊流程只有失敗時會釋放互斥鎖，但如果收到訓則會將互斥鎖鎖起來，要記得再其他流程釋放他
  //! 如果執行失敗，要記得釋放
  if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdTRUE) {
    String stepChose = "";
    String eventChose = "";
    int eventIndexChose = -1;
    if ((*D_QueryParameter).containsKey("step")) {
      stepChose = (*D_QueryParameter)["step"].as<String>();
      if ((*D_QueryParameter).containsKey("event")) {
        eventChose = (*D_QueryParameter)["event"].as<String>();
        if ((*D_QueryParameter).containsKey("index")) {
          eventIndexChose = (*D_QueryParameter)["index"].as<String>().toInt();
        }
      }
    }
    String TargetName = D_PathParameter->as<JsonObject>()["name"];
    String FullFilePath = "/pipelines/"+TargetName+".json";
    if (SD.exists(FullFilePath)) {
      if (Machine_Ctrl.LOAD__ACTION_V2(FullFilePath, stepChose, eventChose, eventIndexChose)) {
        Machine_Ctrl.SetLog(
          5,
          "即將執行流程",
          "流程設定讀取成功",
          NULL, client, false
        );
        //! 這邊不釋放互斥鎖，交由後續執行釋放
      } else {
        Machine_Ctrl.SetLog(
          1,
          "流程設定失敗",
          "檔案讀取失敗",
          NULL, client, false
        );
        xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
      }
    } else {
      Machine_Ctrl.SetLog(
        1,
        "流程設定失敗",
        "找不到流程設定檔案: " + TargetName+".json",
        NULL, client, false
      );
      xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    }
  }
  else {
    Machine_Ctrl.SetLog(
      1,
      "儀器忙碌中，請稍後再試",
      "",
      NULL, client, false
    );
  }






  
}



void ws_TestPipeline(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("websocket API", "Test Pipeline, Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  if (Machine_Ctrl.TASK__NOW_ACTION != NULL) {
    Machine_Ctrl.SetLog(
      2,
      "測試流程設定失敗",
      "儀器忙碌中，請稍後再試",
      NULL, client
    );
  }
  else if (D_pipeline.containsKey(TargetName)) {
    //? 流程設定
    DynamicJsonDocument PipelineItem(1000000);
    JsonObject PipelineJSONItem = BuildPipelineJSONItem(TargetName, D_pipeline[TargetName].as<JsonObject>());
    PipelineItem.set(PipelineJSONItem);
    PipelineItem["data_type"] = String("TEST");
    // String settingString;
    // serializeJson(PipelineItem,settingString);
    // PipelineItem.clear();
    // Machine_Ctrl.LOAD__ACTION(settingString);
    Machine_Ctrl.LOAD__ACTION(PipelineItem.as<JsonObject>());
    Machine_Ctrl.RUN__LOADED_ACTION();
  } else {
    Machine_Ctrl.SetLog(
      1,
      "測試流程設定失敗",
      "找不到流程設定ID: " + TargetName,
      NULL, client
    );
  }

}

void ws_GetPipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  String TargetName = D_PathParameter->as<JsonObject>()["name"];
  ESP_LOGD("Websocket API", "Pipeline Name: %s", TargetName.c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  if (D_pipeline.containsKey(TargetName)) {
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("查詢流程設定完畢");
    D_baseInfoJSON["action"]["target"].set("Pipeline");
    D_baseInfoJSON["action"]["method"].set("Update");
    D_baseInfoJSON["parameter"][TargetName].set(D_pipeline[TargetName]);
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->binary(returnString);
  } else {
    Machine_Ctrl.SetLog(
      1,
      "查詢流程設定失敗",
      "找不到流程設定ID: " + TargetName,
      NULL, client
    );
  }
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
    if (D_oldConfig["desp"].as<String>() != D_newConfig["desp"].as<String>()) {
      D_oldConfig["desp"] = D_newConfig["desp"].as<String>();
    }
    if (D_oldConfig["pool"].as<String>() != D_newConfig["pool"].as<String>()) {
      D_oldConfig["pool"] = D_newConfig["pool"].as<String>();
    }
    D_oldConfig.remove("steps");
    D_oldConfig["steps"].set(D_newConfig["steps"]);
    
    D_baseInfoJSON["parameter"][TargetName].set(D_oldConfig);
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("更新流程設定完畢");
    D_baseInfoJSON["action"]["target"].set("Pipeline");
    D_baseInfoJSON["action"]["method"].set("Update");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);

    Machine_Ctrl.SetLog(
      5,
      "更新流程設定完畢",
      "流程名稱: " + D_oldConfig["title"].as<String>(),
      server, NULL
    );

    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "更新流程設定失敗",
      "找不到流程設定: " + TargetName,
      NULL, client
    );
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
    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("刪除流程設定完畢");
    D_baseInfoJSON["action"]["method"] = "Delete";
    D_baseInfoJSON["action"]["target"] = "Pipeline";
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);

    Machine_Ctrl.SetLog(
      5,
      "刪除流程設定完畢",
      "流程設定名稱: " + TargetName,
      server, NULL
    );

    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  } else {
    Machine_Ctrl.SetLog(
      1,
      "刪除流程設定失敗",
      "找不到流程設定: " + TargetName,
      NULL, client
    );
  }
}

void ws_GetAllPipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_pipeline = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pipeline"];
  D_baseInfoJSON["action"]["status"].set("OK");
  D_baseInfoJSON["action"]["message"].set("查詢所有流程設定完畢");
  D_baseInfoJSON["action"]["target"].set("Pipeline");
  D_baseInfoJSON["action"]["method"].set("Update");
  D_baseInfoJSON["parameter"].set(D_pipeline);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->binary(returnString);
}

void ws_AddNewPipelineInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_FormData->as<JsonObject>();
  if (!D_newConfig.containsKey("title") | !D_newConfig.containsKey("pool")) {
    Machine_Ctrl.SetLog(
      1,
      "新增流程設定失敗",
      "title 參數與 pool 參數為必要項目",
      NULL, client
    );
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
    D_pipeline[NewIDString]["desp"].set(D_newConfig["desp"].as<String>());
    D_pipeline[NewIDString]["pool"].set(D_newConfig["pool"].as<String>());
    D_pipeline[NewIDString].createNestedArray("steps");

    D_baseInfoJSON["parameter"][NewIDString].set(D_pipeline[NewIDString]);

    D_baseInfoJSON["action"]["status"].set("OK");
    D_baseInfoJSON["action"]["message"].set("新增流程設定完畢");
    D_baseInfoJSON["action"]["target"].set("Pipeline");
    D_baseInfoJSON["action"]["method"].set("New");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    server->binaryAll(returnString);

    Machine_Ctrl.SetLog(
      5,
      "新增流程設定: "+D_pipeline[NewIDString]["title"].as<String>(),
      "說明: " + D_pipeline[NewIDString]["desp"].as<String>(),
      server, NULL
    );

    String RewriteConfigResult = Machine_Ctrl.ReWriteDeviceSetting();
    if (RewriteConfigResult == "BUSY") {
      Machine_Ctrl.SetLog(
        1,
        "機器讀寫忙碌中", "請稍後再試",
        NULL, client
      );
    }
  }
}

//! 儀器控制
void ws_v2_RunPwmMotor(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdTRUE) {
    DynamicJsonDocument tempFile(10000);
    //? 作法為了對其正式執行的流程, 若要測試儀器會先生成測試用的temp檔案
    int motorIndex = (*D_QueryParameter)["index"].as<int>();
    int motorStatus = (*D_QueryParameter)["status"].as<int>();
    tempFile["title"].set("手動觸發-伺服馬達測試");
    tempFile["desp"].set("");

    //? events 設定
    tempFile["events"]["test"]["title"].set("伺服馬達轉動");
    tempFile["events"]["test"]["desp"].set("伺服馬達轉動");
    JsonArray eventList = tempFile["events"]["test"].createNestedArray("event");
    DynamicJsonDocument eventObj(1000);
    JsonArray pwmMotorList = eventObj.createNestedArray("pwm_motor_list");
    DynamicJsonDocument indexObj(800);
    indexObj["index"].set(motorIndex);
    indexObj["status"].set(motorStatus);
    pwmMotorList.add(indexObj);
    eventList.add(eventObj);

    //? steps_group 設定
    tempFile["steps_group"]["test"]["title"].set("伺服馬達轉動");
    tempFile["steps_group"]["test"]["desp"].set("伺服馬達轉動");
    JsonArray stepsList = tempFile["steps_group"]["test"].createNestedArray("steps");
    stepsList.add("test");

    //? pipline 設定
    JsonArray piplineList = tempFile.createNestedArray("pipline");
    JsonArray piplineSetList = piplineList.createNestedArray();
    piplineSetList.add("test");
    DynamicJsonDocument emptyObject(1000);
    JsonArray emptyList = emptyObject.createNestedArray();
    piplineSetList.add(emptyList);


    // serializeJsonPretty(tempFile, Serial);
    File tempFileItem = SD.open("/pipelines/__temp__.json", FILE_WRITE);
    serializeJson(tempFile, tempFileItem);
    tempFileItem.close();
    if (Machine_Ctrl.LOAD__ACTION_V2("/pipelines/__temp__.json")) {
      Machine_Ctrl.SetLog(
        5,
        "即將執行流程",
        "流程設定讀取成功",
        NULL, client, false
      );
    } else {
      Machine_Ctrl.SetLog(
        1,
        "流程設定失敗",
        "檔案讀取失敗",
        NULL, client, false
      );
      xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    }
  }

  else {
    Machine_Ctrl.SetLog(
      1,
      "儀器忙碌中，請稍後再試",
      "",
      NULL, client, false
    );
  }
}

void ws_v2_RunPeristalticMotor(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdTRUE) {
    DynamicJsonDocument tempFile(10000);
    //? 作法為了對其正式執行的流程, 若要測試儀器會先生成測試用的temp檔案
    int motorIndex = (*D_QueryParameter)["index"].as<int>();
    int motorStatus = (*D_QueryParameter)["status"].as<int>();
    double motorTime = (*D_QueryParameter)["time"].as<String>().toDouble();
    tempFile["title"].set("手動觸發-蠕動馬達測試");
    tempFile["desp"].set("index: "+String(motorIndex) +", status: "+String(motorStatus) +", time: "+String(motorTime));

    //? events 設定
    tempFile["events"]["test"]["title"].set("蠕動馬達轉動");
    tempFile["events"]["test"]["desp"].set("蠕動馬達轉動");
    JsonArray eventList = tempFile["events"]["test"].createNestedArray("event");
    DynamicJsonDocument eventObj(1000);
    JsonArray pwmMotorList = eventObj.createNestedArray("peristaltic_motor_list");
    DynamicJsonDocument indexObj(800);
    indexObj["index"].set(motorIndex);
    indexObj["status"].set(motorStatus);
    indexObj["time"].set(motorTime);
    indexObj["until"].set("-");
    indexObj["failType"].set("-");
    indexObj["failAction"].set("-");

    pwmMotorList.add(indexObj);
    eventList.add(eventObj);

    //? steps_group 設定
    tempFile["steps_group"]["test"]["title"].set("蠕動馬達轉動");
    tempFile["steps_group"]["test"]["desp"].set("蠕動馬達轉動");
    JsonArray stepsList = tempFile["steps_group"]["test"].createNestedArray("steps");
    stepsList.add("test");

    //? pipline 設定
    JsonArray piplineList = tempFile.createNestedArray("pipline");
    JsonArray piplineSetList = piplineList.createNestedArray();
    piplineSetList.add("test");
    DynamicJsonDocument emptyObject(1000);
    JsonArray emptyList = emptyObject.createNestedArray();
    piplineSetList.add(emptyList);


    // serializeJsonPretty(tempFile, Serial);
    File tempFileItem = SD.open("/pipelines/__temp__.json", FILE_WRITE);
    serializeJson(tempFile, tempFileItem);
    tempFileItem.close();
    if (Machine_Ctrl.LOAD__ACTION_V2("/pipelines/__temp__.json")) {
      Machine_Ctrl.SetLog(
        5,
        "即將執行流程",
        "流程設定讀取成功",
        NULL, client, false
      );
    } else {
      Machine_Ctrl.SetLog(
        1,
        "流程設定失敗",
        "檔案讀取失敗",
        NULL, client, false
      );
      xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    }
  }

  else {
    Machine_Ctrl.SetLog(
      1,
      "儀器忙碌中，請稍後再試",
      "",
      NULL, client, false
    );
  }
}


#endif