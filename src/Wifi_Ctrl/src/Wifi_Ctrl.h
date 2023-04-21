#ifndef WIFI_CTRLER_H
#define WIFI_CTRLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include "AsyncTCP.h"

extern AsyncWebServer asyncServer;
extern AsyncWebSocket ws;

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

    void ConnectToWifi(const char* ssidAP,const char* passwordAP);
    void UpdateMachineTimerByNTP();
    void ServerStart();


    ////////////////////////////////////////////////////
    // For 互動相關
    ////////////////////////////////////////////////////

    DynamicJsonDocument GetWifiInfo();
    String GetWifiInfoString();
    DynamicJsonDocument GetBaseWSReturnData(String MessageString);

    void UploadNewData();

    ////////////////////////////////////////////////////
    // For 互動相關
    ////////////////////////////////////////////////////



  private:
    void setStaticAPIs();
    void createWebServer();
};

#endif