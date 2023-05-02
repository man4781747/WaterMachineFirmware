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

  private:
    void setStaticAPIs();
    void setAPIs();
    void createWebServer();

};

#endif