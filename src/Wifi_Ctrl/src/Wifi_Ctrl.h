#ifndef WIFI_CTRLER_H
#define WIFI_CTRLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoOTA.h> 

#include "AsyncTCP.h"
#include <unordered_map>
#include <map>
#include <vector>
#include <regex>

extern AsyncWebServer asyncServer;
extern AsyncWebSocket ws;

class C_WebsocketAPI
{
  public:
    String APIPath, METHOD;
    std::vector<String> pathParameterKeyMapList;
    /**
     * @brief 
     * ws, client, returnInfoJSON, PathParameterJSON, QueryParameterJSON, FormDataJSON
     * 
     */
    void (*func)(
      AsyncWebSocket*, AsyncWebSocketClient*, 
      DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*
    );
    C_WebsocketAPI(
      String APIPath_, String METHOD_, 
      void (*func_)(
        AsyncWebSocket*, AsyncWebSocketClient*, 
        DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*
      )
    ) {
      METHOD = METHOD_;
      func = func_;
      std::regex ms_PathParameterKeyRegex(".*\\(\\<([a-zA-Z0-9_]*)\\>.*\\).*");
      std::regex ms_PathParameterReplace("\\<.*\\>");  
      std::smatch matches;
      char *token;
      char newAPIPathChar[APIPath_.length()+1];
      strcpy(newAPIPathChar, APIPath_.c_str());
      token = strtok(newAPIPathChar, "/");
      std::string reMix_APIPath = "";
      while( token != NULL ) {
        std::string newMSstring = std::string(token);
        if (std::regex_search(newMSstring, matches, ms_PathParameterKeyRegex)) {
          pathParameterKeyMapList.push_back(String(matches[1].str().c_str()));
          newMSstring = std::regex_replace(matches[0].str().c_str(), ms_PathParameterReplace, "");
        }
        if (newMSstring.length() != 0) {
          reMix_APIPath += "/" + newMSstring;
        }
        token = strtok(NULL, "/");
      }
      APIPath = String(reMix_APIPath.c_str());
      ESP_LOGI("WS_API Setting", "註冊 API: %s, METHOD: %s, PathURL長度: %d", reMix_APIPath.c_str(), METHOD.c_str(), pathParameterKeyMapList.size());
            
    };
  private:
};

/**
 * @brief 儀器WIFI設定
 * 
 */
class CWIFI_Ctrler
{
  public:
    CWIFI_Ctrler(void){};
    String IP = "NONE";
    int rssi = 0;
    String mac_address = "NONE";


    AsyncWebServer *asyncServer_ = &asyncServer;
    AsyncWebSocket *ws_ = &ws;

    ////////////////////////////////////////////////////
    // For 初始化
    ////////////////////////////////////////////////////

    void ConnectToWifi();
    bool CreateSoftAP();
    void ConnectOtherWiFiAP(String SSID, String PW="");
    void UpdateMachineTimerByNTP();
    void ServerStart();

    ////////////////////////////////////////////////////
    // For WIFI Manager相關
    ////////////////////////////////////////////////////

    TaskHandle_t TASK__SSIDScan;
    DynamicJsonDocument* SSIDList = new DynamicJsonDocument(3000);
    DynamicJsonDocument* SSIDList_Temp = new DynamicJsonDocument(3000);
    bool WifiModeOpen = true;
    void StartSTAConnectCheck();

    ////////////////////////////////////////////////////
    // For 互動相關
    ////////////////////////////////////////////////////

    DynamicJsonDocument GetSSIDList();
    void StartWiFiConnecter();

    DynamicJsonDocument GetWifiInfo();
    String GetWifiInfoString();
    DynamicJsonDocument GetBaseWSReturnData(String MessageString);

    void UploadNewData();
    


    void AddWebsocketAPI(String APIPath, String METHOD, void (*func)(AsyncWebSocket*, AsyncWebSocketClient*, DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*, DynamicJsonDocument*));
    // std::unordered_map<std::string, C_WebsocketAPI*> websocketApiSetting;
    std::map<std::string, std::unordered_map<std::string, C_WebsocketAPI*>> websocketApiSetting;

  private:
    void setStaticAPIs();
    void setAPIs();
    void createWebServer();

};
extern CWIFI_Ctrler WIFI_Ctrler;
#endif