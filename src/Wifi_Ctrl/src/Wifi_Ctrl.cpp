#include "Wifi_Ctrl.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"
#include <ArduinoOTA.h> 
#include <SPIFFS.h>
#include <esp_log.h>
#include <TimeLib.h>   
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <esp_system.h>

#include <Update.h>
#include <HTTPClient.h>

#include <sstream>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <regex>
#include <map>
#include "esp_task_wdt.h"


#include "../../Machine_Ctrl/src/Machine_Ctrl.h"

extern const char* FIRMWARE_VERSION;
extern SMachine_Ctrl Machine_Ctrl;

AsyncWebServer asyncServer(80);
AsyncWebSocket ws("/ws");
const long  gmtOffset_sec = 3600*8; // GMT+8
const int   daylightOffset_sec = 0; // DST+0
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", gmtOffset_sec, daylightOffset_sec);
const char* LOG_TAG_WIFI = "WIFI";

uint8_t *oteUpdateFileBuffer;
size_t oteUpdateFileBufferLen;

enum OTAByFileStatus {
  OTAByFileStatusNO,
  OTAByFileStatusING,
  OTAByFileStatusSUCCESS,
  OTAByFileStatusFAIL
};
OTAByFileStatus nowOTAByFileStatus = OTAByFileStatus::OTAByFileStatusNO;

////////////////////////////////////////////////////
// For 處理資料
////////////////////////////////////////////////////

DynamicJsonDocument urlParamsToJSON(const std::string& urlParams) {
  std::stringstream ss(urlParams);
  std::string item;
  std::unordered_map<std::string, std::string> paramMap;
  while (std::getline(ss, item, '&')) {
    std::stringstream ss2(item);
    std::string key, value;
    std::getline(ss2, key, '=');
    std::getline(ss2, value, '=');
    paramMap[key] = value;
  }
  DynamicJsonDocument json_doc(1000);
  for (auto& it : paramMap) {
    json_doc[it.first].set(it.second);
  }
  return json_doc;
}

void SendHTTPesponse(AsyncWebServerRequest *request, AsyncWebServerResponse* response) {
  AsyncWebHeader* h = request->getHeader("Host");
  Serial.printf("HOST %s\n", h->value().c_str());
  JsonObject WifiConfigJSON = Machine_Ctrl.spiffs.WifiConfig->as<JsonObject>();
  response->addHeader("Set-Cookie", "deviceMode=LocalHost");
  response->addHeader("X-device-mode", "LocalHost");
  request->send(response);
}

////////////////////////////////////////////////////
// For WebSocketEvent
////////////////////////////////////////////////////

DynamicJsonDocument CWIFI_Ctrler::GetBaseWSReturnData(String MessageString)
{
  DynamicJsonDocument BaseWSReturnData(500000);
  BaseWSReturnData.set(*(Machine_Ctrl.spiffs.DeviceBaseInfo));
  // JsonObject json_doc = Machine_Ctrl.spiffs.DeviceBaseInfo->as<JsonObject>();
  BaseWSReturnData["FIRMWARE_VERSION"].set(FIRMWARE_VERSION);
  BaseWSReturnData["cmd"].set(MessageString);
  BaseWSReturnData["wifi"].set(Machine_Ctrl.BackendServer.GetWifiInfo());
  // BaseWSReturnData["device_status"].set(Machine_Ctrl.GetEventStatus());
  time_t nowTime = now();
  char datetimeChar[30];
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  BaseWSReturnData["time"] = datetimeChar;
  return BaseWSReturnData;
} 


void ws_LocalWiFiConfigChange(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap){
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_data->as<JsonObject>();
  JsonObject WifiConfigJSON = Machine_Ctrl.spiffs.WifiConfig->as<JsonObject>();
  String returnMessageString = "";
  for (JsonPair newConfigItem : D_newConfig) {
    if (WifiConfigJSON["AP"][newConfigItem.key()]) {
      if (WifiConfigJSON["AP"][newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        ESP_LOGI(LOG_TAG_WIFI, "參數: %s 變更 %s -> %s", newConfigItem.key().c_str(), 
          WifiConfigJSON["AP"][newConfigItem.key()].as<String>().c_str(), 
          newConfigItem.value().as<String>().c_str()
        );
        WifiConfigJSON["AP"][newConfigItem.key()] = newConfigItem.value().as<String>();
        returnMessageString += String(newConfigItem.key().c_str())+": "+WifiConfigJSON["AP"][newConfigItem.key()].as<String>()+"->"+newConfigItem.value().as<String>()+"\n";
      }
    } else {
      ESP_LOGI(LOG_TAG_WIFI, "找不到對應的參數: %s", newConfigItem.key().c_str());
    }
  }
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["parameter"]["message"].set(returnMessageString);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  ws.textAll(returnString);

}

void ws_GetWiFiConfig(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap){
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject WifiConfigJSON = Machine_Ctrl.spiffs.WifiConfig->as<JsonObject>();
  D_baseInfoJSON["parameter"].set(*(Machine_Ctrl.spiffs.WifiConfig));
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["action"].set("UpdateWiFiConfig");
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}


void ws_GetDeviceBaseInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["parameter"]["message"].set("OK");
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_UpdateDeviceBaseInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_newConfig = D_data->as<JsonObject>();
  JsonObject D_DeviceBaseInfo = Machine_Ctrl.spiffs.DeviceBaseInfo->as<JsonObject>();
  String returnMessageString = "";
  for (JsonPair newConfigItem : D_newConfig) {
    if (D_DeviceBaseInfo[newConfigItem.key()]) {
      if (D_DeviceBaseInfo[newConfigItem.key()].as<String>() != newConfigItem.value().as<String>()) {
        ESP_LOGI(LOG_TAG_WIFI, "參數: %s 變更 %s -> %s", newConfigItem.key().c_str(), 
          D_DeviceBaseInfo[newConfigItem.key()].as<String>().c_str(), 
          newConfigItem.value().as<String>().c_str()
        );
        D_DeviceBaseInfo[newConfigItem.key()] = newConfigItem.value().as<String>();
        returnMessageString += String(newConfigItem.key().c_str())+": "+D_DeviceBaseInfo[newConfigItem.key()].as<String>()+"->"+newConfigItem.value().as<String>()+"\n";
      }
    } else {
      ESP_LOGI(LOG_TAG_WIFI, "找不到對應的參數: %s", newConfigItem.key().c_str());
    }
  }
  Machine_Ctrl.spiffs.ReWriteDeviceBaseInfo();
  DynamicJsonDocument D_newBaseInfo = Machine_Ctrl.BackendServer.GetBaseWSReturnData("[PATCH]/api/BaseInfo");
  JsonObject D_baseInfoJSON = D_newBaseInfo.as<JsonObject>();
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["parameter"]["message"].set(returnMessageString);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  ws.textAll(returnString);
}


void ws_GetStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String StepName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD(LOG_TAG_WIFI, "Step Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  if (D_steps_group.containsKey(StepName)) {
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"].set("UpdateOneStep");
    D_baseInfoJSON["parameter"][StepName].set(D_steps_group[StepName]);
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Step: " + StepName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetAllStepInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["steps_group"];
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["action"].set("UpdateAllStep");
  D_baseInfoJSON["parameter"].set(D_steps_group);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String EventName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD(LOG_TAG_WIFI, "Event Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  if (D_event_group.containsKey(EventName)) {
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"].set("UpdateOneEvent");
    D_baseInfoJSON["parameter"][EventName].set(D_event_group[EventName]);
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到Evevnt: " + EventName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetAllEventInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_event_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["event_group"];
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["action"].set("UpdateAllEvent");
  D_baseInfoJSON["parameter"].set(D_event_group);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD(LOG_TAG_WIFI, "GetPeristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"].set("UpdateOnePeristalticMotor");
    D_baseInfoJSON["parameter"][TargetName].set(D_steps_group[TargetName]);
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到蠕動馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetAllPeristalticMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_peristaltic_motor = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["peristaltic_motor"];
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["action"].set("UpdateAllPeristalticMotor");
  D_baseInfoJSON["parameter"].set(D_peristaltic_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetPwmMotorInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  String TargetName = String((*UrlParaMap)[1].c_str());
  ESP_LOGD(LOG_TAG_WIFI, "GetPeristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["action"].set("UpdateOnePwmMotor");
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
  ESP_LOGD(LOG_TAG_WIFI, "GetPeristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
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
    D_baseInfoJSON["action"].set("UpdateOnePwmMotor");
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
  ESP_LOGD(LOG_TAG_WIFI, "GetPeristaltic Motor Name: %s", (*UrlParaMap)[1].c_str());
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_steps_group = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>()["pwm_motor"];
  if (D_steps_group.containsKey(TargetName)) {
    D_steps_group.remove(TargetName);
    D_baseInfoJSON["message"].set("OK");
    D_baseInfoJSON["parameter"]["delete_id"] = TargetName;
    D_baseInfoJSON["action"].set("DeleteOnePwmMotor");
  } else {
    D_baseInfoJSON["message"].set("FAIL");
    D_baseInfoJSON["parameter"]["message"] = "找不到伺服馬達: " + TargetName;
  }
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
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
      D_baseInfoJSON["action"].set("UpdateOnePwmMotor");
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
  D_baseInfoJSON["action"].set("UpdateAllPwmMotor");
  D_baseInfoJSON["parameter"].set(D_pwm_motor);
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

void ws_GetWiFiStatusInfo(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  D_baseInfoJSON["parameter"].set(Machine_Ctrl.BackendServer.GetWifiInfo());
  D_baseInfoJSON["message"].set("OK");
  D_baseInfoJSON["action"].set("UpdateWifiInfo");
  String returnString;
  serializeJsonPretty(D_baseInfoJSON, returnString);
  client->text(returnString);
}

//

void UpdateByFileTask(void* parameter)
{
  for (;;) {
    // File OTAtemp = SPIFFS.open("/OTAtemp.bin", FILE_READ);
    ESP_LOGI("UpdateByFileTask", "更新韌體測試: %d", oteUpdateFileBufferLen);
    esp_task_wdt_init(30,0);
    Update.begin(oteUpdateFileBufferLen);

    size_t written = Update.write(oteUpdateFileBuffer, oteUpdateFileBufferLen);
    ESP_LOGI("UpdateByFileTask", "寫入完成");
    if (Update.end()) {
      nowOTAByFileStatus = OTAByFileStatus::OTAByFileStatusSUCCESS;
      ESP_LOGI("UpdateByFileTask", "更新韌體成功");
    } else {
      ESP_LOGI("UpdateByFileTask", "更新韌體失敗");
      nowOTAByFileStatus = OTAByFileStatus::OTAByFileStatusFAIL;
    }
    esp_task_wdt_init(5,0);
    vTaskDelay(1000);
    vTaskDelete(NULL);
  }
}


void UpdateTask(void* parameter)
{
  for (;;) {
    char* URL = (char*) parameter;
    DynamicJsonDocument D_baseInfo = Machine_Ctrl.BackendServer.GetBaseWSReturnData("[PATCH]OAT Update");
    JsonObject D_baseInfoJSON = D_baseInfo.as<JsonObject>();
    HTTPClient http;
    http.begin(String(URL));
    int http_code = http.GET();
    ESP_LOGI("OTAUpdateTask", "http_code: %d", http_code);
    if (http_code == HTTP_CODE_OK) {
      size_t size = http.getSize();
      Stream *dataStream = http.getStreamPtr();

      ESP_LOGI("OTAUpdateTask", "MALLOC");
      uint8_t* binData = (uint8_t*)malloc(size);
      ESP_LOGI("OTAUpdateTask", "readBytes");
      dataStream->readBytes(binData, size);
      ESP_LOGI("OTAUpdateTask", "Update being");
      Update.begin(size);
      ESP_LOGI("OTAUpdateTask", "Update.write");
      size_t written = Update.write(binData, size);
      ESP_LOGI("OTAUpdateTask", "Free");
      free(binData);

      Serial.printf("written: %d\n", written);
      if (written == size) {
        if (Update.end()) {
          Serial.println("OTA success!");
          D_baseInfoJSON["parameter"]["Message"].set("OTA更新成功，機器即將重啟");
          D_baseInfoJSON["message"].set("OK");
          String returnString;
          serializeJsonPretty(D_baseInfoJSON, returnString);
          ws.textAll(returnString);
          delay(1000);
          ESP.restart();
        } else {
          Update.printError(Serial);
          D_baseInfoJSON["parameter"]["Message"].set("OTA更新失敗: "+String(Update.errorString()));
          D_baseInfoJSON["message"].set("FAIL");
          String returnString;
          serializeJsonPretty(D_baseInfoJSON, returnString);
          ws.textAll(returnString);
        }
      } else {
        D_baseInfoJSON["parameter"]["Message"].set("OTA更新失敗:檔案下載不完全");
        D_baseInfoJSON["message"].set("FAIL");
        String returnString;
        serializeJsonPretty(D_baseInfoJSON, returnString);
        ws.textAll(returnString);
      }
    }
    vTaskDelay(1000);
    vTaskDelete(NULL);
  }
}

void ws_OTAByURL(AsyncWebSocket *server, AsyncWebSocketClient *client, DynamicJsonDocument *D_baseInfo, DynamicJsonDocument *D_data, std::map<int, String>* UrlParaMap)
{
  JsonObject D_baseInfoJSON = D_baseInfo->as<JsonObject>();
  JsonObject D_newConfig = D_data->as<JsonObject>();
  if (D_newConfig.containsKey("URL")) {
    xTaskCreate(
      UpdateTask, "UpdateTask",
      10000, (void *)(D_newConfig["URL"].as<String>()).c_str(), 1, NULL
    );
  } else {
    D_baseInfoJSON["parameter"]["Message"].set("缺少參數: URL");
    D_baseInfoJSON["message"].set("FAIL");
    String returnString;
    serializeJsonPretty(D_baseInfoJSON, returnString);
    client->text(returnString);
  }
}


void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
    Machine_Ctrl.UpdateAllPoolsDataRandom();
    DynamicJsonDocument json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData("WS_EVT_CONNECT");
    // json_doc["parameter"].set(Machine_Ctrl.poolsCtrl.GetAllPoolsBaseInfo());

    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );
    json_doc["time"] = datetimeChar;
    String returnString;
    serializeJsonPretty(json_doc, returnString);
    client->text(returnString);
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  } else if (type == WS_EVT_DATA) {
    ESP_LOGD(LOG_TAG_WIFI, "WS_EVT_DATA");
    String MessageString = String(((char *)data));
    ESP_LOGD(LOG_TAG_WIFI, "Get Message String");
    
    int commaIndex = MessageString.indexOf("]");
    String METHOD = MessageString.substring(1, commaIndex);
    MessageString = MessageString.substring(commaIndex + 1);
    
    commaIndex = MessageString.indexOf("?");
    String Message_CMD = MessageString.substring(0, commaIndex);
    String Data = MessageString.substring(commaIndex + 1);

    std::string Message_CMD_std = std::string(Message_CMD.c_str());
    std::string METHOD_std = std::string(METHOD.c_str());
    std::string Data_std = std::string(Data.c_str());
    ESP_LOGD(LOG_TAG_WIFI, "Message_API: %s", Message_CMD.c_str());
    ESP_LOGD(LOG_TAG_WIFI, "Message_METHOD: %s", METHOD.c_str());
    ESP_LOGD(LOG_TAG_WIFI, "Message_Data: %s", Data.c_str());

    DynamicJsonDocument D_baseInfo = Machine_Ctrl.BackendServer.GetBaseWSReturnData("["+String(METHOD_std.c_str())+"]"+String(Message_CMD_std.c_str()));
    
    bool IsFind = false;
    for (auto it = Machine_Ctrl.BackendServer.websocketApiSetting.begin(); it != Machine_Ctrl.BackendServer.websocketApiSetting.end(); ++it) {
      std::regex reg(it->first.c_str());
      std::smatch matches;
      if (std::regex_match(Message_CMD_std, matches, reg)) {
        std::map<int, String> UrlParameter;
        int paraIndex = 0;
        IsFind = true;
        for (auto it = matches.begin(); it != matches.end(); ++it) {
          UrlParameter.insert(std::make_pair(paraIndex, String(it->str().c_str())));
          paraIndex++;
        }
        if (it->second.count(METHOD_std)) {
          DynamicJsonDocument D_data(10000);
          DeserializationError error = deserializeJson(D_data, String(Data_std.c_str()));
          if (error) {
            ESP_LOGI(LOG_TAG_WIFI, "解析錯誤: %s", error.f_str());
            D_baseInfo["message"] = "FAIL";
            D_baseInfo["parameter"]["message"] = String(error.f_str());
            String returnString;
            serializeJsonPretty(D_baseInfo, returnString);
            client->text(returnString);
          } else {
            it->second[METHOD_std]->func(server, client, &D_baseInfo, &D_data, &UrlParameter);
          }

        } else {
          ESP_LOGE(LOG_TAG_WIFI, "API %s 並無設定 METHOD: %s", Message_CMD_std.c_str(), METHOD_std.c_str());
            D_baseInfo["message"] = "FAIL";
            D_baseInfo["parameter"]["message"] = "Not allow: "+String(METHOD_std.c_str());
        }
      }
    }
    if (!IsFind) {
      ESP_LOGE(LOG_TAG_WIFI, "找不到API: %s", Message_CMD_std.c_str());
      D_baseInfo["parameter"]["message"] = "Cnat find: "+String(Message_CMD_std.c_str());
    }
    D_baseInfo.clear();
  }
}


////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

void OTAServiceTask(void* parameter) {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA starting...");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA end!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("OTA auth failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("OTA begin failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("OTA connect failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("OTA receive failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("OTA end failed");
    }
  });
  ArduinoOTA.begin();
  for(;;){
    ArduinoOTA.handle();
    vTaskDelay(1);
  }
}

/**
 * @brief 與Wifi連線
 * 
 */
void CWIFI_Ctrler::ConnectToWifi()
{
  WiFi.mode(WIFI_AP_STA);
  JsonObject WifiConfigJSON = Machine_Ctrl.spiffs.WifiConfig->as<JsonObject>();
  ESP_LOGI(LOG_TAG_WIFI, "Start to connect to wifi");
  StartWiFiConnecter();
  CreateSoftAP();
  ConnectOtherWiFiAP(
    WifiConfigJSON["Remote"]["remote_Name"].as<String>(),
    WifiConfigJSON["Remote"]["remote_Password"].as<String>()
  );
  xTaskCreate(
    OTAServiceTask, "TASK__OTAService",
    10000, NULL, 1, NULL
  );
}

bool CWIFI_Ctrler::CreateSoftAP()
{
  ESP_LOGI(LOG_TAG_WIFI, "Create Soft AP");
  JsonObject WifiConfigJSON = Machine_Ctrl.spiffs.WifiConfig->as<JsonObject>();
  IPAddress AP_IP = IPAddress();
  IPAddress AP_gateway = IPAddress();
  IPAddress AP_subnet_mask = IPAddress();
  AP_IP.fromString(WifiConfigJSON["AP"]["AP_IP"].as<String>());
  AP_gateway.fromString(WifiConfigJSON["AP"]["AP_gateway"].as<String>());
  AP_subnet_mask.fromString(WifiConfigJSON["AP"]["AP_subnet_mask"].as<String>());
  WiFi.softAPdisconnect();
  WiFi.softAPConfig(AP_IP, AP_gateway, AP_subnet_mask);
  bool ISsuccess = WiFi.softAP(WifiConfigJSON["AP"]["AP_Name"].as<String>(),WifiConfigJSON["AP"]["AP_Password"].as<String>());

  ESP_LOGI(LOG_TAG_WIFI,"AP IP:\t%s", WiFi.softAPIP().toString().c_str());
  ESP_LOGI(LOG_TAG_WIFI,"AP MAC:\t%s", WiFi.softAPmacAddress().c_str());
  return ISsuccess;
}

void CWIFI_Ctrler::ConnectOtherWiFiAP(String SSID, String PW)
{
  WiFi.disconnect();
  WiFi.begin(SSID.c_str(),PW.c_str());
  time_t connectTimeout = now();
  while (!WiFi.isConnected() & now()-connectTimeout < 5) {
    delay(500);
  }
  if (!WiFi.isConnected()) {
    WiFi.disconnect();
    ESP_LOGE(LOG_TAG_WIFI,"Cant connect to WIFI");
    WiFi.scanNetworks(true);
  } else {
    IP = WiFi.localIP().toString();
    rssi = WiFi.RSSI();
    mac_address = WiFi.macAddress();
    ESP_LOGI(LOG_TAG_WIFI,"Server IP: %s", IP.c_str());
    ESP_LOGI(LOG_TAG_WIFI,"Server RSSI: %d", rssi);
    ESP_LOGI(LOG_TAG_WIFI,"Server MAC: %s", mac_address.c_str());
  }
}

/**
 * @brief 使用Wifi的時間更新MCU的時間
 * 
 */
void CWIFI_Ctrler::UpdateMachineTimerByNTP()
{
  ESP_LOGI(LOG_TAG_WIFI, "Try to update MCU timer");
  timeClient.begin();
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  configTime(28800, 0, "pool.ntp.org");
  setTime((time_t)timeClient.getEpochTime());
  ESP_LOGI(LOG_TAG_WIFI, "Time: %s", timeClient.getFormattedTime().c_str());
}

/**
 * @brief 執行後端Server服務
 * 
 */
void CWIFI_Ctrler::ServerStart()
{
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  ws.onEvent(onWebSocketEvent);
  asyncServer.addHandler(&ws);
  asyncServer.begin();
  createWebServer();
}

void CWIFI_Ctrler::createWebServer()
{
  setStaticAPIs();
  setAPIs();
}

void CWIFI_Ctrler::setStaticAPIs()
{
  asyncServer.serveStatic("/static/SPIFFS/",SPIFFS,"/");
  asyncServer.serveStatic("/",SPIFFS,"/").setDefaultFile("index.html");
}

void CWIFI_Ctrler::setAPIs()
{

  AddWebsocketAPI("/api/BaseInfo", "GET", &ws_GetDeviceBaseInfo);
  AddWebsocketAPI("/api/BaseInfo", "PATCH", &ws_UpdateDeviceBaseInfo);

  AddWebsocketAPI("/api/Step/(.*)", "GET", &ws_GetStepInfo);
  AddWebsocketAPI("/api/Step", "GET", &ws_GetAllStepInfo);

  AddWebsocketAPI("/api/Event/(.*)", "GET", &ws_GetEventInfo);
  AddWebsocketAPI("/api/Event", "GET", &ws_GetAllEventInfo);

  AddWebsocketAPI("/api/PeristalticMotor/(.*)", "GET", &ws_GetPeristalticMotorInfo);
  AddWebsocketAPI("/api/PeristalticMotor", "GET", &ws_GetAllPeristalticMotorInfo);

  AddWebsocketAPI("/api/pwmMotor/(.*)", "GET", &ws_GetPwmMotorInfo);
  AddWebsocketAPI("/api/pwmMotor/(.*)", "PATCH", &ws_PatchPwmMotorInfo);
  AddWebsocketAPI("/api/pwmMotor/(.*)", "DELETE", &ws_DeletePwmMotorInfo);
  AddWebsocketAPI("/api/pwmMotor", "GET", &ws_GetAllPwmMotorInfo);
  AddWebsocketAPI("/api/pwmMotor", "POST", &ws_AddNewPwmMotorInfo);
  

  AddWebsocketAPI("/api/WiFi", "GET", &ws_GetWiFiStatusInfo);
  AddWebsocketAPI("/api/WiFi/Config", "GET", &ws_GetWiFiConfig);
  AddWebsocketAPI("/api/WiFi/Local", "PATCH", &ws_LocalWiFiConfigChange);

  AddWebsocketAPI("/api/System", "PUT", &ws_OTAByURL);
  
  
  asyncServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/web/index.html", "text/html");
    SendHTTPesponse(request, response);
  });

  asyncServer.on("/api/wifi/Connect", HTTP_POST, [&](AsyncWebServerRequest *request){
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "OK");
    SendHTTPesponse(request, response);
  });
  asyncServer.on("/api/wifi/GetSSIDList", HTTP_GET, [&](AsyncWebServerRequest *request){
    String returnString;
    serializeJsonPretty(*(Machine_Ctrl.BackendServer.SSIDList), returnString);
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", returnString);
    SendHTTPesponse(request, response);
  });
  
  asyncServer.on("/api/System/OTA", HTTP_POST, 
    [&](AsyncWebServerRequest *request)
    {
      nowOTAByFileStatus = OTAByFileStatus::OTAByFileStatusING;
      ESP_LOGI("POST", "TESTTEST");
      xTaskCreatePinnedToCore(
        UpdateByFileTask, "UpdateByFile",
        10000, NULL, 2, NULL, 1
      );
      while (nowOTAByFileStatus == OTAByFileStatus::OTAByFileStatusING) {
        vTaskDelay(100);
      }
      free(oteUpdateFileBuffer);
      if (nowOTAByFileStatus == OTAByFileStatus::OTAByFileStatusSUCCESS) {
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "OK");
        SendHTTPesponse(request, response);
        delay(1000);
        ESP.restart();
      } else {
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "FAIL");
        SendHTTPesponse(request, response);
      }
    },
    [&](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
      File otaTempFile;
      if (index == 0) {
        oteUpdateFileBuffer = (uint8_t *)malloc(request->contentLength());
      }
      memcpy(oteUpdateFileBuffer + index, data, len);
      if (final) {
        Serial.printf("檔案 %s 接收完成， len: %d ，共 %d/%d bytes\n", filename.c_str(), len ,index + len, request->contentLength());
        oteUpdateFileBufferLen = index + len;
        // File OTAtemp = SPIFFS.open("/OTAtemp.bin", FILE_WRITE);
        // OTAtemp.write(oteUpdateFileBuffer ,index + len);
        // OTAtemp.close();
      } 
      else {
        Serial.printf("檔案 %s 正在傳輸， len: %d ，目前已接收 %d/%d bytes\n", filename.c_str(), len, index + len, request->contentLength());
      }
    }
  );

}


////////////////////////////////////////////////////
// For WIFI Manager相關
////////////////////////////////////////////////////

void WiFiConnecter(void* parameter)
{
  JsonObject SSIDListChild =  Machine_Ctrl.BackendServer.SSIDList->createNestedObject();
  JsonObject WiFiConfig = Machine_Ctrl.spiffs.WifiConfig->as<JsonObject>();
  for (;;) {
    if (!WiFi.isConnected()) {
      int wifiScanStatus = WiFi.scanComplete();
      ESP_LOGD("WiFi Manager", "TEST: %d", wifiScanStatus);
      switch (wifiScanStatus) {
        case -1:
          break;
        case -2:
          WiFi.scanNetworks(true);
          break;
        default:
          for (size_t i = 0; i < wifiScanStatus; i++)
          {
            SSIDListChild[WiFi.SSID(i)]["RSSI"].set(WiFi.RSSI(i));
            SSIDListChild[WiFi.SSID(i)]["encryptionType"].set((int)WiFi.encryptionType(i));
          }
          break;
      } 
    }      
    else {
      SSIDListChild.clear();
    }
    WiFi.scanDelete();
    vTaskDelay(1000);
  }
}

void CWIFI_Ctrler::StartWiFiConnecter()
{
  xTaskCreate(
    WiFiConnecter, "TASK__SSIDScan",
    10000, NULL, 1, &TASK__SSIDScan
  );
}



////////////////////////////////////////////////////
// For 互動相關
////////////////////////////////////////////////////

DynamicJsonDocument CWIFI_Ctrler::GetSSIDList()
{
  DynamicJsonDocument SSIDList(1000);
  int16_t n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i){
    SSIDList[WiFi.SSID(i)] = WiFi.RSSI(i);
  }
  return SSIDList;
}


// void ScanSSIDList(void* parameter) {
//   for (;;) {
//     Machine_Ctrl.BackendServer.SSIDList_Temp->clear();
//     JsonObject SSIDList = Machine_Ctrl.BackendServer.SSIDList_Temp->as<JsonObject>();
//     int16_t n = WiFi.scanNetworks();
//     for (int i = 0; i < n; ++i){
//       SSIDList[WiFi.SSID(i)] = WiFi.RSSI(i);
//     }
//     Machine_Ctrl.BackendServer.SSIDList->clear();
//     Machine_Ctrl.BackendServer.SSIDList->set(SSIDList);
//     vTaskDelay(100);
//   }
// }
// void CWIFI_Ctrler::StartScanSSIDList()
// {
//   xTaskCreate(
//     ScanSSIDList, "TASK__SSIDScan",
//     10000, NULL, 1, &TASK__SSIDScan
//   );
// }




DynamicJsonDocument CWIFI_Ctrler::GetWifiInfo()
{
  JsonObject WifiConfigJSON = Machine_Ctrl.spiffs.WifiConfig->as<JsonObject>();
  DynamicJsonDocument json_doc(3000);
  JsonVariant json_obj = json_doc.to<JsonVariant>();
  WiFi.softAPgetHostname();
  
  json_doc["Local"]["name"] = WifiConfigJSON["AP"]["AP_Name"].as<String>();
  json_doc["Local"]["ip"] = WifiConfigJSON["AP"]["AP_IP"].as<String>();
  json_doc["Local"]["gateway"] = WifiConfigJSON["AP"]["AP_gateway"].as<String>();
  json_doc["Local"]["subnet_mask"] = WifiConfigJSON["AP"]["AP_subnet_mask"].as<String>();

  json_doc["Remote"]["ip"].set(IP);
  json_doc["Remote"]["rssi"].set(rssi);
  json_doc["Remote"]["mac_address"].set(mac_address);
  return json_doc;
};

String CWIFI_Ctrler::GetWifiInfoString()
{
  void* json_output = malloc(300);
  DynamicJsonDocument json_doc = GetWifiInfo();
  serializeJsonPretty(json_doc, json_output, 10000);
  String returnString = String((char*)json_output);
  free(json_output);
  json_doc.clear();
  return returnString;
};

void CWIFI_Ctrler::UploadNewData()
{
  Serial.println("UploadNewData");
  Machine_Ctrl.UpdateAllPoolsDataRandom();
  DynamicJsonDocument json_doc = GetBaseWSReturnData("WS_EVT_CONNECT");
  json_doc["parameter"].set(Machine_Ctrl.poolsCtrl.GetAllPoolsBaseInfo());

  time_t nowTime = now();
  char datetimeChar[30];
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  json_doc["time"] = datetimeChar;
  String returnString;
  serializeJsonPretty(json_doc, returnString);
  ws.textAll(returnString);
}


void CWIFI_Ctrler::AddWebsocketAPI(String APIPath, String METHOD, void (*func)(AsyncWebSocket*, AsyncWebSocketClient*, DynamicJsonDocument*, DynamicJsonDocument*, std::map<int, String>*))
{
  C_WebsocketAPI *newAPI = new C_WebsocketAPI(APIPath, METHOD, func);
  std::unordered_map<std::string, C_WebsocketAPI*> sub_map;

  if (websocketApiSetting.count(std::string(APIPath.c_str())) == 0) {
    sub_map[std::string(METHOD.c_str())] = newAPI;
    websocketApiSetting.insert(
      std::make_pair(std::string(APIPath.c_str()), sub_map)
    );
  } else {
    websocketApiSetting[std::string(APIPath.c_str())][std::string(METHOD.c_str())] = newAPI;
  }
  
  // websocketApiSetting[std::string(APIPath.c_str())][std::string(METHOD.c_str())] = newAPI;
}
