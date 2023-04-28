#include "Wifi_Ctrl.h"
#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"
#include <SPIFFS.h>
#include <esp_log.h>
#include <TimeLib.h>   
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <NTPClient.h>

#include <sstream>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <regex>

#include "../../Machine_Ctrl/src/Machine_Ctrl.h"

extern SMachine_Ctrl Machine_Ctrl;

AsyncWebServer asyncServer(80);
AsyncWebSocket ws("/ws");
const long  gmtOffset_sec = 3600*8; // GMT+8
const int   daylightOffset_sec = 0; // DST+0
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", gmtOffset_sec, daylightOffset_sec);
const char* LOG_TAG_WIFI = "WIFI";

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


////////////////////////////////////////////////////
// For WebSocketEvent
////////////////////////////////////////////////////

DynamicJsonDocument CWIFI_Ctrler::GetBaseWSReturnData(String MessageString)
{
  DynamicJsonDocument json_doc = Machine_Ctrl.MachineInfo.GetDeviceInfo();
  json_doc["cmd"].set(MessageString);
  json_doc["wifi"].set(Machine_Ctrl.BackendServer.GetWifiInfo());
  json_doc["device_status"].set(Machine_Ctrl.GetEventStatus());
  time_t nowTime = now();
  char datetimeChar[30];
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  json_doc["time"] = datetimeChar;
  return json_doc;
} 

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
    Machine_Ctrl.UpdateAllPoolsDataRandom();
    DynamicJsonDocument json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData("WS_EVT_CONNECT");
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
    client->text(returnString);
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    char* swData = ((char *)data);
    ESP_LOGD(LOG_TAG_WIFI, "Get message: %s", swData);
    String MessageString = String(swData);
    int index = MessageString.indexOf('?');
    String Message_CMD, Message_Parameter;
    
    if (index >= 0) {
      Message_CMD = MessageString.substring(0,index);
      Message_Parameter = MessageString.substring(index+1,MessageString.length());
    } else {
      Message_CMD = MessageString;
      Message_Parameter = "";
    }
    ESP_LOGD(LOG_TAG_WIFI, "Message_CMD: %s", Message_CMD.c_str());
    ESP_LOGD(LOG_TAG_WIFI, "Message_Parameter: %s", Message_Parameter.c_str());
    const std::string const_str(Message_Parameter.c_str());
    DynamicJsonDocument parameters = urlParamsToJSON(const_str);

    DynamicJsonDocument json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData(MessageString);
    
    if (Message_CMD==String("PING")) {
      json_doc["parameter"]["Message"].set("PONG");
      json_doc["message"].set("OK");
      String returnString;
      serializeJsonPretty(json_doc, returnString);
      client->text(returnString);
    }
    else if (Message_CMD==String("GetLatestInformation")) {
      Machine_Ctrl.UpdateAllPoolsDataRandom();
      if (Message_Parameter == "") {
        // 新增參數
        json_doc["parameter"].set(Machine_Ctrl.poolsCtrl.GetAllPoolsBaseInfo());
        json_doc["message"].set("OK");
        // 發出訊息
        String returnString;
        serializeJsonPretty(json_doc, returnString);
        client->text(returnString);
      } else {
        String poolChose = parameters["pool"] | "";
        if (poolChose.length() != 0) {
          // 新增參數
          json_doc["parameter"].set(Machine_Ctrl.poolsCtrl.GetPoolInfo(poolChose));
          json_doc["message"].set("OK");
          // 發出訊息 
          String returnString;
          serializeJsonPretty(json_doc, returnString);
          client->text(returnString);
        } else {
          // 新增參數
          json_doc["parameter"]["WaringMessage"].set("Wrong parameter");
          json_doc["message"].set("FAIL");
          // 發出訊息 
          String returnString;
          serializeJsonPretty(json_doc, returnString);
          client->text(returnString);
        }
      }
    }
    else if (Message_CMD==String("Set_Parameter")) {
      String NewTimeIntervals = parameters["NewTimeIntervals"] | "";
      // 排程時間設定
      String changeMessageSave = "";
      if (NewTimeIntervals != "") {
        std::string str(NewTimeIntervals.c_str());
        std::regex pattern("\\d\\d:\\d\\d");
        std::smatch matches;
        changeMessageSave += "Change TimeIntervals: "+NewTimeIntervals+"\r\n";
        Machine_Ctrl.MachineInfo.MachineInfo.time_interval->clear();
        while (std::regex_search(str, matches, pattern)) {
          for (auto match : matches) {
            Machine_Ctrl.MachineInfo.MachineInfo.time_interval->createNestedObject(String(match.str().c_str()));
          }
          str = matches.suffix().str();
        }
      }
      String NewDevno = parameters["Devno"] | "";
      if (NewDevno != "") {
        changeMessageSave += "Change Devno:"+Machine_Ctrl.MachineInfo.MachineInfo.device_no+" -> "+NewDevno+"\r\n";
        Machine_Ctrl.MachineInfo.MachineInfo.device_no = NewDevno;
      }
      String Mode = parameters["Mode"] | "";
      if ( Mode == "Mode_Master" | Mode == "Mode_Slave" ) {
        changeMessageSave += "Change Mode:"+Machine_Ctrl.MachineInfo.MachineInfo.mode+" -> "+Mode+"\r\n";
        Machine_Ctrl.MachineInfo.MachineInfo.mode = Mode;
      }
      
      Machine_Ctrl.spiffs.ReWriteMachineSettingFile(Machine_Ctrl.MachineInfo.MachineInfo);
      // 重新獲得參數設定
      json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData(MessageString);
      // 新增參數
      json_doc["parameter"]["Message"] = changeMessageSave;
      json_doc["message"].set("OK");
      // 發出訊息 
      String returnString;
      serializeJsonPretty(json_doc, returnString);
      ws.textAll(returnString);
    }
    else if (Message_CMD==String("Get_StepSetting")) {
      json_doc["parameter"].set(*(Machine_Ctrl.spiffs.DeviceSetting));
      json_doc["message"].set("OK");
      String returnString;
      serializeJsonPretty(json_doc, returnString);
      client->text(returnString);
    } else if (Message_CMD==String("Action")) {
      String action = parameters["action"] | "";
      if (action == "loadStep") {
        String stepName = parameters["stepName"] | "";
        Machine_Ctrl.LoadStepToRunHistoryItem(stepName,"Web");
        json_doc["message"].set("OK");
      }
      else if (action == "runStep") {
        Machine_Ctrl.RUN__History();
        json_doc["message"].set("OK");
      }
      else if (action == "stopAllTask") {
        Machine_Ctrl.STOP_AllTask();
        json_doc["message"].set("OK");
      }
      else if (action == "resumeAllTask") {
        Machine_Ctrl.RESUME_AllTask();
        json_doc["message"].set("OK");
      }
      String returnString;
      serializeJsonPretty(json_doc, returnString);
      ws.textAll(returnString);
    }
    json_doc.clear();
  }
}


////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

/**
 * @brief 與Wifi連線
 * 
 */
void CWIFI_Ctrler::ConnectToWifi(const char* ssidAP,const char* passwordAP)
{
  ESP_LOGI(LOG_TAG_WIFI, "Start to connect to wifi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidAP,passwordAP);
  ESP_LOGI(LOG_TAG_WIFI, "Waiting for WiFi connect");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  IP = WiFi.localIP().toString();
  rssi = WiFi.RSSI();
  mac_address = WiFi.macAddress();
  ESP_LOGI(LOG_TAG_WIFI,"Server IP: %s", IP.c_str());
  ESP_LOGI(LOG_TAG_WIFI,"Server RSSI: %d", rssi);
  ESP_LOGI(LOG_TAG_WIFI,"Server MAC: %s", mac_address.c_str());
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
  // time_t now = time(nullptr);
  // char datetime[23];
  // sprintf(datetime, "%04d-%02d-%02dT%02d:%02d:%02d+08", year(now), month(now), day(now), hour(now), minute(now), second(now));
  // Serial.println(datetime);
  configTime(28800, 0, "pool.ntp.org");
  setTime((time_t)timeClient.getEpochTime());

  // now = time(nullptr);
  // sprintf(datetime, "%04d-%02d-%02dT%02d:%02d:%02d+08", year(now), month(now), day(now), hour(now), minute(now), second(now));
  // Serial.println(datetime);
  // now()
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
}

void CWIFI_Ctrler::setStaticAPIs()
{
  asyncServer.serveStatic("/static/SPIFFS/",SPIFFS,"/");
  asyncServer.serveStatic("/",SPIFFS,"/").setDefaultFile("index.html");
}


////////////////////////////////////////////////////
// For 互動相關
////////////////////////////////////////////////////

DynamicJsonDocument CWIFI_Ctrler::GetWifiInfo()
{
  DynamicJsonDocument json_doc(300);
  JsonVariant json_obj = json_doc.to<JsonVariant>();
  json_doc["ip"].set(IP);
  json_doc["rssi"].set(rssi);
  json_doc["mac_address"].set(mac_address);
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
