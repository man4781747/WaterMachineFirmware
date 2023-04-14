#include "Wifi_Ctrl.h"
#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"
#include <SPIFFS.h>
#include <esp_log.h>
#include <TimeLib.h>   
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <NTPClient.h>

#include "../../Machine_Base_info/src/Machine_Base_info.h"
#include "../../Motor_Ctrl/src/Motor_Ctrl.h"
#include "../../Machine_Ctrl/src/Machine_Ctrl.h"

AsyncWebServer asyncServer(80);
AsyncWebSocket ws("/ws");

const long  gmtOffset_sec = 3600*8; // GMT+8
const int   daylightOffset_sec = 0; // DST+0
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", gmtOffset_sec, daylightOffset_sec);


extern Machine_Info MachineInfo;
extern Motor_Ctrl motorCtrl;
extern SMachine_Ctrl Machine_Ctrl;
const char* LOG_TAG_WIFI = "WIFI";

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
    Machine_Ctrl.UpdateAllPoolsDataRandom();
    DynamicJsonDocument json_doc = Machine_Ctrl.GetDeviceInfos();
  
    json_doc["messageType"] = "WS_EVT_CONNECT";
    json_doc["CMDType"] = "AllData";
    time_t nowTime = now();
    char datetimeChar[30];
    sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
      year(nowTime), month(nowTime), day(nowTime),
      hour(nowTime), minute(nowTime), second(nowTime)
    );

    json_doc["time"] = datetimeChar;
    void* json_output = malloc(6000);
    serializeJsonPretty(json_doc, json_output, 6000);
    String returnString = String((char*)json_output);
    client->text(returnString);
    free(json_output);
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  } else if (type == WS_EVT_DATA) {

    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    char* swData = ((char *)data);
    ESP_LOGD(LOG_TAG_WIFI, "Get message: %s", swData);
    DynamicJsonDocument json_doc(1024);
    deserializeJson(json_doc, swData);
    const char* MessageType = json_doc["MessageType"];
    ESP_LOGD(LOG_TAG_WIFI, "MessageType: %s", MessageType);
    if (strcmp(MessageType, "ChangeMotorAngle") == 0) {
      int newAngle = json_doc["AngleSet"];
      const char* MotorID = json_doc["MotorID"];
      ESP_LOGD(LOG_TAG_WIFI, "ChangeMotor %s to %d", MotorID, newAngle);
      // if (strcmp(MotorID, "m_01") == 0) {
      //   motorCtrl.servo_m1.write(newAngle);
      // } else if (strcmp(MotorID, "m_02") == 0) {
      //   motorCtrl.servo_m2.write(newAngle);
      // }
      StaticJsonDocument<1024> return_json_doc;
      char json_output[1024];
      DeserializationError json_error;

      return_json_doc["messageType"] = "SomeOneChangeMotorAngle";
      return_json_doc["data"]["newAngle"] = newAngle;
      return_json_doc["data"]["MotorID"] = MotorID;
      serializeJson(return_json_doc, json_output);
      ws.textAll(json_output);
    } else if (strcmp(MessageType, "GetMachineInfo") == 0) {
      Machine_Ctrl.UpdateAllPoolsDataRandom();
      DynamicJsonDocument json_doc = Machine_Ctrl.GetDeviceInfos();
      json_doc["messageType"] = "WS_EVT_DATA";
      time_t nowTime = now();
      char datetimeChar[30];
      sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
        year(nowTime), month(nowTime), day(nowTime),
        hour(nowTime), minute(nowTime), second(nowTime)
      );
      json_doc["time"] = datetimeChar;
      json_doc["CMDType"] = "AllData";
      void* json_output = malloc(6000);
      serializeJsonPretty(json_doc, json_output, 6000);
      String returnString = String((char*)json_output);
      client->text(returnString);
      free(json_output);
    }
  }
}

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
  String ServerIP = WiFi.localIP().toString();
  ESP_LOGI(LOG_TAG_WIFI,"Server IP: %s", ServerIP.c_str());
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
  setTime(timeClient.getEpochTime());
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

void CWIFI_Ctrler::UploadNewData()
{
  Machine_Ctrl.UpdateAllPoolsDataRandom();
  DynamicJsonDocument json_doc = Machine_Ctrl.GetDeviceInfos();

  json_doc["messageType"] = "Loop";
  json_doc["CMDType"] = "AllData";
  time_t nowTime = now();
  char datetimeChar[30];
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  json_doc["time"] = datetimeChar;
  void* json_output = malloc(6000);
  serializeJsonPretty(json_doc, json_output, 6000);
  String returnString = String((char*)json_output);
  ws.textAll(returnString);
  free(json_output);
}
