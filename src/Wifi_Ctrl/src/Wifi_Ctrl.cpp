#include "Wifi_Ctrl.h"
#include <codecvt>
#include <WiFi.h>
#include <SPI.h>
#include "AsyncTCP.h"
#include <ArduinoOTA.h> 
#include <SPIFFS.h>
#include <esp_log.h>
#include <TimeLib.h>   
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <NTPClient.h>
#include <SD.h>
#include <esp_system.h>
// #include <Ethernet.h>
// #include <AsyncWebServer_ESP32_W5500.h>
// #include <AsyncWebServer_Ethernet.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <HTTPClient.h>

#include <sstream>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <regex>
#include <map>
#include <vector>
#include "esp_task_wdt.h"
#include "StorgeSystemExternalFunction.h"


#include "../../Machine_Ctrl/src/Machine_Ctrl.h"
#include "models.h"
#include "urls.h"



extern const char* FIRMWARE_VERSION;
extern SMachine_Ctrl Machine_Ctrl;

AsyncWebServer asyncServer(80);
AsyncWebSocket ws("/ws");

// EthernetServer ethernet_server(80);

extern byte ethernet_mac;
extern IPAddress ethernet_ip;
// IPAddress ethernet_gateway(192, 168, 20, 1);



const long  gmtOffset_sec = 3600*8; // GMT+8
const int   daylightOffset_sec = 0; // DST+0
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", gmtOffset_sec, daylightOffset_sec);
const char* LOG_TAG_WIFI = "WIFI";

uint8_t *oteUpdateFileBuffer;
size_t oteUpdateFileBufferLen;

uint8_t *newConfigUpdateFileBuffer;
size_t newConfigUpdateFileBufferLen;


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

std::string remove_non_utf8(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
  std::wstring wstr = conv.from_bytes(str);
  std::wstring::iterator it = std::remove_if(wstr.begin(), wstr.end(), [](wchar_t c) {
      return (c < 0 || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF));
  });
  wstr.erase(it, wstr.end());
  return conv.to_bytes(wstr);
}

String removeNonUTF8Characters(const String& input) {
  // 正则表达式模式，匹配非 UTF-8 字符
  std::regex pattern("[^\\x20-\\x9F]+");
  
  std::string inputString_STD = std::string(input.c_str());
  // 使用空字符串替换非 UTF-8 字符
  std::string result = std::regex_replace(inputString_STD, pattern, "");
  
  return String(result.c_str());
}

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
  BaseWSReturnData.set(*(Machine_Ctrl.JSON__DeviceBaseInfo));
  // time_t nowTime = now();
  // char datetimeChar[30];
  // sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
  //   year(nowTime), month(nowTime), day(nowTime),
  //   hour(nowTime), minute(nowTime), second(nowTime)
  // );
  BaseWSReturnData["firmware_version"].set(FIRMWARE_VERSION);

  int startIndex = MessageString.indexOf("/", 0);
  startIndex = MessageString.indexOf("/", startIndex+1);
  int endIndex = MessageString.indexOf("/", startIndex+1);
  if (startIndex == -1) {
    BaseWSReturnData["cmd"].set(MessageString);
  } else if (endIndex == -1) {
    BaseWSReturnData["cmd"].set(MessageString.substring(startIndex+1, MessageString.length()));
  } else {
    BaseWSReturnData["cmd"].set(MessageString.substring(startIndex+1, endIndex));
  }
  BaseWSReturnData["internet"].set(Machine_Ctrl.BackendServer.GetWifiInfo());
  BaseWSReturnData["time"].set(Machine_Ctrl.GetNowTimeString());
  BaseWSReturnData["utc"] = "+8";
  BaseWSReturnData["action"]["message"].set("看到這行代表API設定時忘記設定本項目了，請通知工程師修正，謝謝");
  BaseWSReturnData["action"]["status"].set("看到這行代表API設定時忘記設定本項目了，請通知工程師修正，謝謝");
  BaseWSReturnData.createNestedObject("parameter");
  BaseWSReturnData["cmd_detail"].set(MessageString);

  if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdFALSE) {
    BaseWSReturnData["device_status"].set("Busy");
  }
  else {
    xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    BaseWSReturnData["device_status"].set("Idle");
  }
  // serializeJsonPretty(BaseWSReturnData, Serial);
  // BaseWSReturnData["device_status"].set(Machine_Ctrl.GetEventStatus());
  return BaseWSReturnData;
} 

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
    client->binary(returnString);
  }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
    DynamicJsonDocument D_baseInfo = Machine_Ctrl.BackendServer.GetBaseWSReturnData("");
    D_baseInfo["cmd"].set("CONNECTED");
    D_baseInfo["action"]["message"].set("OK");
    String returnString;
    serializeJsonPretty(D_baseInfo, returnString);
    client->binary(returnString);
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  } else if (type == WS_EVT_DATA) {
    // ESP_LOGV("ws", "收到訊息");
    vTaskDelay(10/portTICK_PERIOD_MS);
    // String MessageString= String(remove_non_utf8(std::string((char *)data)).c_str());
    String MessageString = String((char *)data);
    int totalLen = MessageString.length();

    size_t length = 0;
    for (const uint8_t* p = data; *p; ++p) {
        ++length;
    }

    // ESP_LOGV("ws", "total len: %d, data* len: %d, String len: %d", len, length, totalLen);
    MessageString = MessageString.substring(0, len);
    int commaIndex = MessageString.indexOf("]");
    String METHOD = MessageString.substring(1, commaIndex);

    bool dataFail = false;
    if (METHOD == "PATCH" or METHOD == "POST") {
      int paraLen = MessageString.length();
      if (paraLen == 0) {
        dataFail = true;
      } else {
        char lastChar = MessageString.charAt(paraLen-1);
        if (lastChar != '\n') {
          dataFail = true;
        } else {
          MessageString = MessageString.substring(commaIndex + 1, paraLen);
        }
      }
    } else {
      MessageString = MessageString.substring(commaIndex + 1);
    }

    commaIndex = MessageString.indexOf("?");
    String Message_CMD = MessageString.substring(0, commaIndex);
    String QueryParameter = MessageString.substring(commaIndex + 1);
    DynamicJsonDocument D_QueryParameter = urlParamsToJSON(QueryParameter.c_str());
    DynamicJsonDocument D_FormData(10000);
    String dataString = D_QueryParameter["data"].as<String>();
    DeserializationError error = deserializeJson(D_FormData, dataString);

    if (error or dataFail) {
      DynamicJsonDocument D_errorbaseInfo = Machine_Ctrl.BackendServer.GetBaseWSReturnData("["+METHOD+"]"+Message_CMD);
      D_errorbaseInfo["message"] = "FAIL";
      D_errorbaseInfo["parameter"]["message"] = "API帶有的Data解析錯誤，參數格式錯誤?";
      String ErrorMessage;
      serializeJsonPretty(D_errorbaseInfo, ErrorMessage);
      client->binary(ErrorMessage);
      D_errorbaseInfo.clear();
    }
    else {
      D_QueryParameter.remove("data");
      std::string Message_CMD_std = std::string(Message_CMD.c_str());
      std::string METHOD_std = std::string(METHOD.c_str());
      DynamicJsonDocument D_baseInfo = Machine_Ctrl.BackendServer.GetBaseWSReturnData("["+String(METHOD_std.c_str())+"]"+String(Message_CMD_std.c_str()));
      
      bool IsFind = false;
      for (auto it = Machine_Ctrl.BackendServer.websocketApiSetting.rbegin(); it != Machine_Ctrl.BackendServer.websocketApiSetting.rend(); ++it) {
        std::regex reg(it->first.c_str());
        std::smatch matches;
        if (std::regex_match(Message_CMD_std, matches, reg)) {
          std::map<int, String> UrlParameter;
          IsFind = true;
          if (it->second.count(METHOD_std)) {
            DynamicJsonDocument D_PathParameter(1000);
            int pathParameterIndex = -1;
            for (auto matches_it = matches.begin(); matches_it != matches.end(); ++matches_it) {
              if (pathParameterIndex == -1) {
                pathParameterIndex++;
                continue;
              }
              if ((int)(it->second[METHOD_std]->pathParameterKeyMapList.size()) <= pathParameterIndex) {
                break;
              }
              D_PathParameter[it->second[METHOD_std]->pathParameterKeyMapList[pathParameterIndex]] = String(matches_it->str().c_str());
            }
            it->second[METHOD_std]->func(server, client, &D_baseInfo, &D_PathParameter, &D_QueryParameter, &D_FormData);
          } else {
            ESP_LOGE(LOG_TAG_WIFI, "API %s 並無設定 METHOD: %s", Message_CMD_std.c_str(), METHOD_std.c_str());
            D_baseInfo["message"] = "FAIL";
            D_baseInfo["parameter"]["message"] = "Not allow: "+String(METHOD_std.c_str());
            String returnString;
            serializeJsonPretty(D_baseInfo, returnString);
            client->binary(returnString);
          }
          break;
        }
      }
      if (!IsFind) {
        ESP_LOGE(LOG_TAG_WIFI, "找不到API: %s", Message_CMD_std.c_str());
        D_baseInfo["parameter"]["message"] = "找不到API: "+String(Message_CMD_std.c_str());
        D_baseInfo["action"]["status"] = "FAIL";
        D_baseInfo["action"]["message"] = "找不到API: "+String(Message_CMD_std.c_str());
        String returnString;
        serializeJsonPretty(D_baseInfo, returnString);
        client->binary(returnString);
      }
      D_baseInfo.clear();
    }
  }
}


////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

void WiFiConnectChecker(void* parameter) {
  ESP_LOGW("WiFiConnectChecker","開始偵測WiFi是否斷線");
  for (;;) {
    vTaskDelay(60000/portTICK_PERIOD_MS);
    ESP_LOGW("WiFiConnectChecker","開始檢查網路是否斷線");
    if (!WiFi.isConnected()) {
      ESP_LOGW("WiFiConnectChecker","偵測到WiFi斷線，嘗試重新連接");
      WiFi.disconnect();
      WiFi.begin(
        (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Name"].as<String>().c_str(),
        (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Password"].as<String>().c_str()
      );
      // lasUpdatetime = -9999;
    }
    // vTaskDelay(50000/portTICK_PERIOD_MS);
  }
}


void OTAServiceTask(void* parameter) {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA starting...");
    Machine_Ctrl.SetLog(3, "儀器遠端更新中", "", Machine_Ctrl.BackendServer.ws_, NULL);
  });
  ArduinoOTA.onEnd([]() {
    Machine_Ctrl.SetLog(3, "儀器遠端更新成功，即將重開機", "", Machine_Ctrl.BackendServer.ws_, NULL);
    Serial.println("\nOTA end!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Machine_Ctrl.SetLog(1, "儀器遠端更新失敗", "OTA auth failed", Machine_Ctrl.BackendServer.ws_, NULL);
      Serial.println("OTA auth failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Machine_Ctrl.SetLog(1, "儀器遠端更新失敗", "OTA begin failed", Machine_Ctrl.BackendServer.ws_, NULL);
      Serial.println("OTA begin failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Machine_Ctrl.SetLog(1, "儀器遠端更新失敗", "OTA connect failed", Machine_Ctrl.BackendServer.ws_, NULL);
      Serial.println("OTA connect failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Machine_Ctrl.SetLog(1, "儀器遠端更新失敗", "OTA receive failed", Machine_Ctrl.BackendServer.ws_, NULL);
      Serial.println("OTA receive failed");
    } else if (error == OTA_END_ERROR) {
      Machine_Ctrl.SetLog(1, "儀器遠端更新失敗", "OTA end failed", Machine_Ctrl.BackendServer.ws_, NULL);
      Serial.println("OTA end failed");
    }
  });
  ArduinoOTA.begin();
  for(;;){
    ArduinoOTA.handle();
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}


/**
 * @brief 與Wifi連線
 * 
 */



void CWIFI_Ctrler::ConnectToWifi()
{
  WiFi.mode(WIFI_AP_STA);
  ESP_LOGI(LOG_TAG_WIFI, "Start to connect to wifi");
  CreateSoftAP();
  // WiFi.disconnect();
  // WiFi.begin(
  //   (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Name"].as<String>().c_str(),
  //   (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Password"].as<String>().c_str()
  // );
  // int failCount = 0;
  // int reTryCount = 0;
  // while (!WiFi.isConnected()) {
  //   Machine_Ctrl.PrintOnScreen("Try Connect Wifi\nRetry: "+String(reTryCount));
  //   if (failCount > 15) {
  //     failCount = 0;
  //     reTryCount++;
  //     WiFi.disconnect();
  //     WiFi.begin(
  //       (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Name"].as<String>().c_str(),
  //       (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Password"].as<String>().c_str()
  //     );
  //   }
  //   vTaskDelay(1000/portTICK_PERIOD_MS);
  //   failCount++;
  // }
  WiFi.begin(
    (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Name"].as<String>().c_str(),
    (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Password"].as<String>().c_str()
  );
  WiFi.setAutoReconnect(true);
  xTaskCreate(
    OTAServiceTask, "TASK__OTAService",
    10000, NULL, 1, NULL
  );
}

bool CWIFI_Ctrler::CreateSoftAP()
{
  ESP_LOGI(LOG_TAG_WIFI, "Create Soft AP");
  JsonObject WifiConfigJSON = (*Machine_Ctrl.JSON__WifiConfig).as<JsonObject>();
  IPAddress AP_IP = IPAddress();
  IPAddress AP_gateway = IPAddress();
  IPAddress AP_subnet_mask = IPAddress();
  AP_IP.fromString(WifiConfigJSON["AP"]["AP_IP"].as<String>());
  AP_gateway.fromString(WifiConfigJSON["AP"]["AP_gateway"].as<String>());
  AP_subnet_mask.fromString(WifiConfigJSON["AP"]["AP_subnet_mask"].as<String>());
  WiFi.softAPdisconnect();
  WiFi.softAPConfig(AP_IP, AP_gateway, AP_subnet_mask);
  bool ISsuccess = WiFi.softAP(WifiConfigJSON["AP"]["AP_Name"].as<String>(),WifiConfigJSON["AP"]["AP_Password"].as<String>());
  ESP_LOGI(LOG_TAG_WIFI,"AP Name:\t%s", WifiConfigJSON["AP"]["AP_Name"].as<String>().c_str());
  ESP_LOGI(LOG_TAG_WIFI,"AP PW:\t%s", WifiConfigJSON["AP"]["AP_Password"].as<String>().c_str());
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
  int errorCount = 0;
  while(!timeClient.update()) {
    timeClient.forceUpdate();
    vTaskDelay(100);
    errorCount++;
    if (errorCount > 1000) {
      ESP.restart();
    }
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
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "*");
  ws.onEvent(onWebSocketEvent);
  asyncServer.addHandler(&ws);
  asyncServer.begin();
  createWebServer();
}

void CWIFI_Ctrler::StartSTAConnectCheck()
{
  xTaskCreate(
    WiFiConnectChecker, "WiFiChecker",
    10000, NULL, 2, &TASK__STAConnectCheck
  );
}

void CWIFI_Ctrler::createWebServer()
{
  setStaticAPIs();
  setAPIs();
}

void CWIFI_Ctrler::setStaticAPIs()
{
  asyncServer.serveStatic("/static/SPIFFS/",SPIFFS,"/");
  asyncServer.serveStatic("/static/SD/",SD,"/");
  asyncServer.serveStatic("/",SPIFFS,"/").setDefaultFile("index.html");
  
}

void CWIFI_Ctrler::setAPIs()
{
  CWIFI_Ctrler *This = this;
  setAPI(*This);

  asyncServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    AsyncWebServerResponse* response = request->beginResponse(SPIFFS, "/web/index.html", "text/html");
    SendHTTPesponse(request, response);
  });

  asyncServer.on("/api/pipeline/config", HTTP_POST, 
    [&](AsyncWebServerRequest *request)
    { 
      free(newConfigUpdateFileBuffer);
      // String FileContent = String(newConfigUpdateFileBuffer ,newConfigUpdateFileBufferLen);
      // free(newConfigUpdateFileBuffer);
      // DynamicJsonDocument NewDeviceSetting(300000);
      // DeserializationError error = deserializeJson(NewDeviceSetting, FileContent);
      // if (error) {
      //   Serial.print("deserializeJson() failed: ");
      //   Serial.println(error.f_str());
      //   AsyncWebServerResponse* response = request->beginResponse(400, "application/json", "{\"Result\":\"FAIL\"}");
      //   SendHTTPesponse(request, response);
      // }
      // else {
      //   AsyncWebServerResponse* response = request->beginResponse(200, "application/json", FileContent);
      //   SendHTTPesponse(request, response);
      // }
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "{\"Result\":\"OK\"}");
      SendHTTPesponse(request, response);
    },
    [&](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
      if (index == 0) {
        newConfigUpdateFileBuffer = (uint8_t *)malloc(request->contentLength());
      }
      memcpy(newConfigUpdateFileBuffer + index, data, len);
      if (final) {
        Serial.printf("檔案 %s 接收完成， len: %d ，共 %d/%d bytes\n", filename.c_str(), len ,index + len, request->contentLength());
        newConfigUpdateFileBufferLen = index + len;
        if (!SD.exists("/pipelines")) {
          SD.mkdir("/pipelines");
        }
        File configTempFile;
        configTempFile = SD.open("/pipelines/"+filename, FILE_WRITE);
        configTempFile.write(newConfigUpdateFileBuffer ,index + len);
        configTempFile.close();
        Serial.printf("檔案更新完成\n", filename.c_str());
        Machine_Ctrl.SD__UpdatePipelineConfigList();
      } 
      else {
        Serial.printf("檔案 %s 正在傳輸， len: %d ，目前已接收 %d/%d bytes\n", filename.c_str(), len, index + len, request->contentLength());
      }
    }
  );

  //? 排程設定相關API
  asyncServer.on("/api/schedule", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson(*Machine_Ctrl.JSON__ScheduleConfig, RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );
  asyncServer.on("^\\/api\\/schedule\\/([a-zA-Z0-9_.-]+)$", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      String keyName = request->pathArg(0);
      if ((*Machine_Ctrl.JSON__ScheduleConfig).containsKey(keyName)) {
        response = request->beginResponse(200, "application/json", (*Machine_Ctrl.JSON__ScheduleConfig)[keyName]);
      } 
      else {
        response = request->beginResponse(500, "application/json", "{\"Result\":\"Can't Find: "+keyName+"\"}");
      }
      SendHTTPesponse(request, response);
    }
  );
  //? 接收前端上傳的設定更新
  //? FormData強制一定要有 Param: "content", 型態: String, 格式: JSON
  asyncServer.on("^\\/api\\/schedule\\/([a-zA-Z0-9_.-]+)$", HTTP_PATCH,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      String keyName = request->pathArg(0);
      if (request->hasArg("content")) {
        String content = request->getParam("content", true)->value();
        DynamicJsonDocument JSON__content(1000);
        DeserializationError error = deserializeJson(JSON__content, content);
        if (error) {
          ESP_LOGE("schedule更新", "JOSN解析失敗,停止更新排程設定檔內容", error.c_str());
          response = request->beginResponse(500, "application/json", "{\"Result\":\"更新失敗,content所需型態: String, 格式: JSON\"}");
        }
        else {
          (*Machine_Ctrl.JSON__ScheduleConfig)[keyName].set(JSON__content);
          Machine_Ctrl.SD__ReWriteScheduleConfig();
          response = request->beginResponse(200, "application/json", (*Machine_Ctrl.JSON__ScheduleConfig)[keyName]);
        }
      }
      else {
        response = request->beginResponse(500, "application/json", "{\"Result\":\"缺少para: 'content',型態: String, 格式: JSON\"}");
      }
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("^\\/api\\/schedule\\/([a-zA-Z0-9_.-]+)$", HTTP_DELETE,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      String keyName = request->pathArg(0);
      if ((*Machine_Ctrl.JSON__ScheduleConfig).containsKey(keyName)) {
        (*Machine_Ctrl.JSON__ScheduleConfig).remove(keyName);
        Machine_Ctrl.SD__ReWriteScheduleConfig();
        response = request->beginResponse(200, "application/json", "{\"Result\":\"刪除成功\"}");
      } 
      else {
        response = request->beginResponse(500, "application/json", "{\"Result\":\"Can't Find: "+keyName+"\"}");
      }
      SendHTTPesponse(request, response);
    }
  );

  //LOG系統

  //? default的log api是抓取最新100筆log
  asyncServer.on("/api/logs", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      DynamicJsonDocument ReturnData(100000);
      String StartTime = "1900-01-01 00:00:00";
      String EndTime = "2900-01-01 00:00:00";
      String LevelList = "1,2,3,4,5,6";
      if (request->hasParam("st")) {
        StartTime = request->getParam("st")->value();
      }
      if (request->hasParam("et")) {
        EndTime = request->getParam("et")->value();
      }
      if (request->hasParam("lv")) {
        LevelList = request->getParam("lv")->value();
      }

      // String SQL_String = "SELECT * FROM logs DESC LIMIT 1000";

      String SQL_String = "SELECT time, title, desp, level FROM logs WHERE level in (";
      SQL_String += LevelList;
      SQL_String += ") AND time BETWEEN '";
      SQL_String += StartTime;
      SQL_String += "' AND '";
      SQL_String += EndTime;
      SQL_String += "' LIMIT 1000";
      // String SQL_String = "SELECT * FROM logs WHERE time BETWEEN '";
      // SQL_String += StartTime;
      // SQL_String += "' AND '";
      // SQL_String += EndTime;
      // SQL_String += "' LIMIT 1000";

      // String SQL_String = "SELECT * FROM logs WHERE level in (";
      // SQL_String += LevelList;
      // SQL_String += ") ORDER BY time DESC LIMIT 1000";

      Machine_Ctrl.db_exec( Machine_Ctrl.DB_Log, SQL_String, &ReturnData);
      String RetuenString;
      serializeJson(ReturnData, RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("^\\/api\\/logs\\/([a-zA-Z0-9_.-]+)$", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      String keyName = request->pathArg(0);
      if ((*Machine_Ctrl.JSON__ScheduleConfig).containsKey(keyName)) {
        response = request->beginResponse(200, "application/json", (*Machine_Ctrl.JSON__ScheduleConfig)[keyName]);
      } 
      else {
        response = request->beginResponse(500, "application/json", "{\"Result\":\"Can't Find: "+keyName+"\"}");
      }
      SendHTTPesponse(request, response);
    }
  );

  // 感測器資廖系統

  //? default的log api是抓取最新100筆log
  asyncServer.on("/api/sensor", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      DynamicJsonDocument ReturnData(100000);
      String StartTime = "1900-01-01 00:00:00";
      String EndTime = "2900-01-01 00:00:00";
      String PoolList = "'pool-1','pool-2','pool-3','pool-4'";
      String ValueNameList = "'pH','NO2','NH4'";
      if (request->hasParam("st")) {
        StartTime = request->getParam("st")->value();
      }
      if (request->hasParam("et")) {
        EndTime = request->getParam("et")->value();
      }
      if (request->hasParam("pl")) {
        PoolList = request->getParam("pl")->value();
      }
      if (request->hasParam("name")) {
        ValueNameList = request->getParam("name")->value();
      }

      String SQL_String = "SELECT * FROM sensor WHERE pool in (";
      SQL_String += PoolList;
      SQL_String += ") AND value_name in (";
      SQL_String += ValueNameList;
      SQL_String += ") AND time BETWEEN '";
      SQL_String += StartTime;
      SQL_String += "' AND '";
      SQL_String += EndTime;
      SQL_String += "' ORDER BY time DESC LIMIT 1000";
      Machine_Ctrl.db_exec(Machine_Ctrl.DB_Sensor,SQL_String, &ReturnData);


      // if (request->hasParam("st") & request->hasParam("et")) {
      //   String StartTime = request->getParam("st")->value();
      //   String EndTime = request->getParam("et")->value();
      //   // Machine_Ctrl.db_exec(Machine_Ctrl.DB_Sensor, "SELECT * FROM sensor WHERE time >= "+StartTime+" AND time <="+EndTime+" ORDER BY time DESC LIMIT 100", &ReturnData);
      //   Machine_Ctrl.db_exec(Machine_Ctrl.DB_Sensor, "SELECT * FROM sensor WHERE time BETWEEN '"+StartTime+" 00:00:00' AND '"+EndTime+" 00:00:00' ORDER BY time DESC LIMIT 100", &ReturnData);
      // }
      // else {
      //   Machine_Ctrl.db_exec(Machine_Ctrl.DB_Sensor, "SELECT * FROM sensor DESC LIMIT 100", &ReturnData);
      // }
      String RetuenString;
      serializeJson(ReturnData, RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/sensor", HTTP_DELETE,
    [&](AsyncWebServerRequest *request)
    { 

      DynamicJsonDocument ReturnData(100000);
      if (request->hasParam("st") & request->hasParam("et")) {
        String StartTime = request->getParam("st")->value();
        String EndTime = request->getParam("et")->value();
        // Machine_Ctrl.db_exec(Machine_Ctrl.DB_Sensor, "SELECT * FROM sensor WHERE time >= "+StartTime+" AND time <="+EndTime+" ORDER BY time DESC LIMIT 100", &ReturnData);
        Machine_Ctrl.db_exec(Machine_Ctrl.DB_Sensor, "SELECT * FROM sensor WHERE time BETWEEN '"+StartTime+" 00:00:00' AND '"+EndTime+" 00:00:00' ORDER BY time DESC LIMIT 100", &ReturnData);
      }
      else {
        Machine_Ctrl.db_exec(Machine_Ctrl.DB_Sensor, "SELECT * FROM sensor ORDER BY id DESC LIMIT 1000", &ReturnData);
      }
      String RetuenString;
      serializeJson(ReturnData, RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/config/device_config", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson((*Machine_Ctrl.JSON__DeviceConfig), RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/config/spectrophotometer_config", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson((*Machine_Ctrl.JSON__SpectrophotometerConfig), RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/config/PHmeter_config", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson((*Machine_Ctrl.JSON__PHmeterConfig), RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("^\\/api\\/pool_config\\/([a-zA-Z0-9_.-]+)$", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      String keyName = request->pathArg(0);

      for (JsonVariant poolConfig : (*Machine_Ctrl.JSON__PoolConfig).as<JsonArray>()) {
        JsonObject poolConfig_Obj = poolConfig.as<JsonObject>();
        if (poolConfig_Obj["id"].as<String>() == keyName) {
          String ReturnString;
          serializeJson(poolConfig_Obj, ReturnString);
          response = request->beginResponse(200, "application/json", ReturnString);
          SendHTTPesponse(request, response);
        }
      }
      response = request->beginResponse(500, "application/json", "{\"Result\":\"Can't Find: "+keyName+"\"}");
      SendHTTPesponse(request, response);
    }
  );
  asyncServer.on("^\\/api\\/pool_config\\/([a-zA-Z0-9_.-]+)$", HTTP_PATCH,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      if (!request->hasArg("content")) {
        response = request->beginResponse(500, "application/json", "{\"Result\":\"缺少para: 'content',型態: String, 格式: JSON\"}");
        SendHTTPesponse(request, response);
      }
      String keyName = request->pathArg(0);
      for (JsonVariant poolConfig : (*Machine_Ctrl.JSON__PoolConfig).as<JsonArray>()) {
        JsonObject poolConfig_Obj = poolConfig.as<JsonObject>();
        if (poolConfig_Obj["id"].as<String>() == keyName) {
          String content = request->getParam("content", true)->value();
          DynamicJsonDocument JSON__content(1000);
          DeserializationError error = deserializeJson(JSON__content, content);
          if (error) {
            ESP_LOGE("schedule更新", "JOSN解析失敗,停止更新排程設定檔內容", error.c_str());
            response = request->beginResponse(500, "application/json", "{\"Result\":\"更新失敗,content所需型態: String, 格式: JSON\"}");
            SendHTTPesponse(request, response);
          }
          if (JSON__content.containsKey("external_mapping")) {
            poolConfig_Obj["external_mapping"] = JSON__content["external_mapping"].as<String>();
          }
          if (JSON__content.containsKey("title")) {
            poolConfig_Obj["title"] = JSON__content["title"].as<String>();
          }
          if (JSON__content.containsKey("desp")) {
            poolConfig_Obj["desp"] = JSON__content["desp"].as<String>();
          }
          ExFile_WriteJsonFile(SD, Machine_Ctrl.FilePath__SD__PoolConfig, *Machine_Ctrl.JSON__PoolConfig);
          String ReturnString;
          serializeJson(poolConfig_Obj, ReturnString);
          response = request->beginResponse(200, "application/json", ReturnString);
          SendHTTPesponse(request, response);
        }
      }
      response = request->beginResponse(500, "application/json", "{\"Result\":\"Can't Find: "+keyName+"\"}");
      SendHTTPesponse(request, response);
    }
  );


  asyncServer.on("/api/pool_config", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson((*Machine_Ctrl.JSON__PoolConfig), RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );
  asyncServer.on("/api/pool_config", HTTP_PATCH,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      if (!request->hasArg("content")) {
        response = request->beginResponse(500, "application/json", "{\"Result\":\"缺少para: 'content',型態: String, 格式: JSON\"}");
        SendHTTPesponse(request, response);
      }
      String content = request->getParam("content", true)->value();
      DynamicJsonDocument JSON__content(1000);
      DeserializationError error = deserializeJson(JSON__content, content);
      if (error) {
        ESP_LOGE("schedule更新", "JOSN解析失敗,停止更新排程設定檔內容", error.c_str());
        response = request->beginResponse(500, "application/json", "{\"Result\":\"更新失敗,content所需型態: String, 格式: JSON\"}");
        SendHTTPesponse(request, response);
      }
      (*Machine_Ctrl.JSON__PoolConfig) = JSON__content;
      ExFile_WriteJsonFile(SD, Machine_Ctrl.FilePath__SD__PoolConfig, *Machine_Ctrl.JSON__PoolConfig);
      String ReturnString;
      serializeJson((*Machine_Ctrl.JSON__PoolConfig), ReturnString);
      response = request->beginResponse(200, "application/json", ReturnString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/config/device_config", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson((*Machine_Ctrl.JSON__DeviceConfig), RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );
  asyncServer.on("/api/config/device_config", HTTP_PATCH,
    [&](AsyncWebServerRequest *request)
    { 
      AsyncWebServerResponse* response;
      if (!request->hasArg("content")) {
        response = request->beginResponse(500, "application/json", "{\"Result\":\"缺少para: 'content',型態: String, 格式: JSON\"}");
        SendHTTPesponse(request, response);
      }
      String content = request->getParam("content", true)->value();
      DynamicJsonDocument JSON__content(1000);
      DeserializationError error = deserializeJson(JSON__content, content);
      if (error) {
        ESP_LOGE("schedule更新", "JOSN解析失敗,停止更新排程設定檔內容", error.c_str());
        response = request->beginResponse(500, "application/json", "{\"Result\":\"更新失敗,content所需型態: String, 格式: JSON\"}");
        SendHTTPesponse(request, response);
      }
      (*Machine_Ctrl.JSON__DeviceConfig) = JSON__content;
      ExFile_WriteJsonFile(SD, Machine_Ctrl.FilePath__SD__DeviceConfig, *Machine_Ctrl.JSON__DeviceConfig);
      String ReturnString;
      serializeJson((*Machine_Ctrl.JSON__DeviceConfig), ReturnString);
      response = request->beginResponse(200, "application/json", ReturnString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/config/peristaltic_motor_config", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson((*Machine_Ctrl.JSON__PeristalticMotorConfig), RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/config/pwm_motor_config", HTTP_GET,
    [&](AsyncWebServerRequest *request)
    { 
      String RetuenString;
      serializeJson((*Machine_Ctrl.JSON__PWNMotorConfig), RetuenString);
      AsyncWebServerResponse* response = request->beginResponse(200, "application/json", RetuenString);
      SendHTTPesponse(request, response);
    }
  );

  asyncServer.on("/api/config", HTTP_POST, 
    [&](AsyncWebServerRequest *request)
    { 
      String FileContent = String(newConfigUpdateFileBuffer ,newConfigUpdateFileBufferLen);
      free(newConfigUpdateFileBuffer);
      DynamicJsonDocument NewDeviceSetting(300000);
      DeserializationError error = deserializeJson(NewDeviceSetting, FileContent);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
        AsyncWebServerResponse* response = request->beginResponse(400, "application/json", "{\"Result\":\"FAIL\"}");
        SendHTTPesponse(request, response);
      }
      else {
        AsyncWebServerResponse* response = request->beginResponse(200, "application/json", FileContent);
        SendHTTPesponse(request, response);
      }

    },
    [&](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final)
    {
      if (index == 0) {
        newConfigUpdateFileBuffer = (uint8_t *)malloc(request->contentLength());
      }
      memcpy(newConfigUpdateFileBuffer + index, data, len);
      if (final) {
        Serial.printf("檔案 %s 接收完成， len: %d ，共 %d/%d bytes\n", filename.c_str(), len ,index + len, request->contentLength());
        newConfigUpdateFileBufferLen = index + len;
        if (!SD.exists("/config")) {
          SD.mkdir("/config");
        }
        File configTempFile;
        configTempFile = SD.open("/config/"+filename, FILE_WRITE);
        configTempFile.write(newConfigUpdateFileBuffer ,index + len);
        configTempFile.close();
        Machine_Ctrl.SD__UpdatePipelineConfigList();
      } 
      else {
        Serial.printf("檔案 %s 正在傳輸， len: %d ，目前已接收 %d/%d bytes\n", filename.c_str(), len, index + len, request->contentLength());
      }
    }
  );

  asyncServer.on("^\\/api\\/pipeline\\/([a-zA-Z0-9_.-]+)$", HTTP_DELETE, 
    [&](AsyncWebServerRequest *request)
    { 
      DynamicJsonDocument responeData(1000);
      AsyncWebServerResponse* response;
      String ResponeContent;
      String fileName = request->pathArg(0);
      Serial.println("/pipelines/"+fileName);
      if (SD.exists("/pipelines/"+fileName)) {
        if (SD.remove("/pipelines/"+fileName)) {
          Machine_Ctrl.SD__UpdatePipelineConfigList();
          responeData["result"].set("Delete Pipeline File: "+fileName+" Success");
          serializeJson(responeData, ResponeContent);
          response = request->beginResponse(200, "application/json", ResponeContent);
        }
        else {
          responeData["result"].set("Delete Pipeline File: "+fileName+" Fail");
          response = request->beginResponse(400, "application/json", ResponeContent);
        }
      }
      else {
        responeData["result"].set("Can't File Pipeline File: "+fileName);
        response = request->beginResponse(400, "application/json", ResponeContent);
      }
      SendHTTPesponse(request, response);
    }
  );
  asyncServer.on("/api/piplines", HTTP_GET, [&](AsyncWebServerRequest *request){
    String pipelineFilesList;
    serializeJsonPretty(*Machine_Ctrl.JSON__PipelineConfigList, pipelineFilesList);
    // serializeJsonPretty(ExFile_listDir(SD,"/pipelines"), pipelineFilesList);

    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", pipelineFilesList);
    SendHTTPesponse(request, response);
  });

  //! 獲得Sensor檔案列表
  asyncServer.on("/api/SensorHistory", HTTP_GET, [&](AsyncWebServerRequest *request){
    String SensorHistoryFilesList;
    serializeJsonPretty(ExFile_listDir(SD,"/datas"), SensorHistoryFilesList);
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", SensorHistoryFilesList);
    SendHTTPesponse(request, response);
  });



  asyncServer.on("/api/wifi", HTTP_GET, [&](AsyncWebServerRequest *request){
    String ReturnData;
    serializeJson((*Machine_Ctrl.JSON__WifiConfig)["Remote"], ReturnData);
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", ReturnData);
    SendHTTPesponse(request, response);
  });

  asyncServer.on("/api/wifi", HTTP_POST, [&](AsyncWebServerRequest *request){
    String StringReturnMessage;
    String NewRemoteName;
    String NewRemotePassword;
    DynamicJsonDocument ReturnMessage(1000);
    AsyncWebServerResponse* response;
    if (request->hasParam("remote_Name", true) & request->hasParam("remote_Password", true)) {
      NewRemoteName = request->getParam("remote_Name", true)->value();
      NewRemotePassword = request->getParam("remote_Password", true)->value();
      (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Name"].set(NewRemoteName);
      (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Password"].set(NewRemotePassword);
      Machine_Ctrl.SPIFFS__ReWriteWiFiConfig();
      WiFi.disconnect();
      WiFi.begin(
        (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Name"].as<String>().c_str(),
        (*Machine_Ctrl.JSON__WifiConfig)["Remote"]["remote_Password"].as<String>().c_str()
      );
      ReturnMessage["Message"] = "Success";
      serializeJson(ReturnMessage, StringReturnMessage);
      response = request->beginResponse(200, "application/json", StringReturnMessage);
    } else {
      ReturnMessage["Message"] = "需要參數: remote_Name, remote_Password";
      serializeJson(ReturnMessage, StringReturnMessage);
      response = request->beginResponse(500, "application/json", StringReturnMessage);
    }
    SendHTTPesponse(request, response);
  });


  asyncServer.on("/api/wifi/Connect", HTTP_POST, [&](AsyncWebServerRequest *request){
    AsyncWebServerResponse* response = request->beginResponse(200, "application/json", "OK");
    SendHTTPesponse(request, response);
  });
  asyncServer.on("/api/System/OTA", HTTP_POST, 
    [&](AsyncWebServerRequest *request)
    {
      nowOTAByFileStatus = OTAByFileStatus::OTAByFileStatusING;
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

  //! CORS 檢查用
  asyncServer.onNotFound([](AsyncWebServerRequest *request) {
    if (request->method() == HTTP_OPTIONS) {
      request->send(200);
    } else {
      request->send(404);
    }
  });
}


////////////////////////////////////////////////////
// For 互動相關
////////////////////////////////////////////////////


DynamicJsonDocument CWIFI_Ctrler::GetWifiInfo()
{
  JsonObject WifiConfigJSON = (*Machine_Ctrl.JSON__WifiConfig).as<JsonObject>();
  WifiConfigJSON["Remote"]["ip"].set(WiFi.softAPIP().toString());
  WifiConfigJSON["Remote"]["rssi"].set(WiFi.RSSI());
  WifiConfigJSON["Remote"]["mac_address"].set(WiFi.softAPmacAddress());


  // DynamicJsonDocument json_doc(3000);
  // JsonVariant json_obj = json_doc.to<JsonVariant>();
  // WiFi.softAPgetHostname();
  // json_doc["remote"]["ip"].set(IP);
  // json_doc["remote"]["rssi"].set(rssi);
  // json_doc["remote"]["mac_address"].set(mac_address);
  return WifiConfigJSON;
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

void CWIFI_Ctrler::AddWebsocketAPI(String APIPath, String METHOD, void (*func)(AsyncWebSocket*, AsyncWebSocketClient*, DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*))
{
  C_WebsocketAPI *newAPI = new C_WebsocketAPI(APIPath, METHOD, func);
  std::unordered_map<std::string, C_WebsocketAPI*> sub_map;
  if (websocketApiSetting.count(std::string(newAPI->APIPath.c_str())) == 0) {
    sub_map[std::string(newAPI->METHOD.c_str())] = newAPI;
    websocketApiSetting.insert(
      std::make_pair(std::string(newAPI->APIPath.c_str()), sub_map)
    );
  } else {
    websocketApiSetting[std::string(newAPI->APIPath.c_str())][std::string(newAPI->METHOD.c_str())] = newAPI;
  }
}
