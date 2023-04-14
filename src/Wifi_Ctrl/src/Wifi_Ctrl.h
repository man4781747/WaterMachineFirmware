#ifndef WIFI_CTRLER_H
#define WIFI_CTRLER_H

#include <Arduino.h>
#include <ArduinoJson.h>

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


    void UploadNewData();

  private:
    void setStaticAPIs();
    void createWebServer();
};

#endif