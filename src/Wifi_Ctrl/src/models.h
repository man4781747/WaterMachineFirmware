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



void ws_v2_RunPipeline(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_PathParameter, DynamicJsonDocument *D_QueryParameter, DynamicJsonDocument *D_FormData)
{
  //? 先判斷儀器是否空閒
  //! 注意，這邊流程只有失敗時會釋放互斥鎖，但如果收到訓則會將互斥鎖鎖起來，要記得再其他流程釋放他
  //! 如果執行失敗，要記得釋放
  if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdTRUE) {
    DynamicJsonDocument singlePipelineSetting(10000);

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
    singlePipelineSetting["FullFilePath"].set(FullFilePath);
    singlePipelineSetting["TargetName"].set(TargetName);
    singlePipelineSetting["stepChose"].set(stepChose);
    singlePipelineSetting["eventChose"].set(eventChose);
    singlePipelineSetting["eventIndexChose"].set(eventIndexChose);

    (*Machine_Ctrl.pipelineStack).clear();
    (*Machine_Ctrl.pipelineStack).add(singlePipelineSetting);
    
    Machine_Ctrl.LOAD__ACTION_V2(Machine_Ctrl.pipelineStack);

    // if (SD.exists(FullFilePath)) {
    //   if (Machine_Ctrl.LOAD__ACTION_V2(FullFilePath, stepChose, eventChose, eventIndexChose)) {
    //     Machine_Ctrl.SetLog(
    //       5,
    //       "即將執行流程",
    //       "流程設定讀取成功",
    //       NULL, client, false
    //     );
    //     //! 這邊不釋放互斥鎖，交由後續執行釋放
    //   } else {
    //     Machine_Ctrl.SetLog(
    //       1,
    //       "流程設定失敗",
    //       "檔案讀取失敗",
    //       NULL, client, false
    //     );
    //     xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    //   }
    // } else {
    //   Machine_Ctrl.SetLog(
    //     1,
    //     "流程設定失敗",
    //     "找不到流程設定檔案: " + TargetName+".json",
    //     NULL, client, false
    //   );
    //   xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    // }
 
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

    DynamicJsonDocument singlePipelineSetting(10000);
    String TargetName = "__temp__.json";
    String FullFilePath = "/pipelines/__temp__.json";
    singlePipelineSetting["FullFilePath"].set(FullFilePath);
    singlePipelineSetting["TargetName"].set(TargetName);
    singlePipelineSetting["stepChose"].set("");
    singlePipelineSetting["eventChose"].set("");
    singlePipelineSetting["eventIndexChose"].set(-1);

    (*Machine_Ctrl.pipelineStack).clear();
    (*Machine_Ctrl.pipelineStack).add(singlePipelineSetting);
    
    Machine_Ctrl.LOAD__ACTION_V2(Machine_Ctrl.pipelineStack);
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
    File tempFileItem = SD.open("/pipelines/__temp__.json", FILE_WRITE);
    serializeJson(tempFile, tempFileItem);
    tempFileItem.close();
    DynamicJsonDocument singlePipelineSetting(10000);
    String TargetName = "__temp__.json";
    String FullFilePath = "/pipelines/__temp__.json";
    singlePipelineSetting["FullFilePath"].set(FullFilePath);
    singlePipelineSetting["TargetName"].set(TargetName);
    singlePipelineSetting["stepChose"].set("");
    singlePipelineSetting["eventChose"].set("");
    singlePipelineSetting["eventIndexChose"].set(-1);

    (*Machine_Ctrl.pipelineStack).clear();
    (*Machine_Ctrl.pipelineStack).add(singlePipelineSetting);
    
    Machine_Ctrl.LOAD__ACTION_V2(Machine_Ctrl.pipelineStack);
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