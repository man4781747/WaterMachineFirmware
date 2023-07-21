#ifndef SPIFFS_Ctrl_H
#define SPIFFS_Ctrl_H

#include <Arduino.h>

#include <ArduinoJson.h>

class SPIFFS_Ctrl
{
  public:
    SPIFFS_Ctrl(void){};
    void INIT_SPIFFS();
    
    void CreateFile(String FilePath);
    void CreateFolder(String FolderPath);

    DynamicJsonDocument* LoadDeviceBaseInfo();
    void ReWriteDeviceBaseInfo();
    DynamicJsonDocument *DeviceBaseInfo = new DynamicJsonDocument(500);
    String DeviceBaseInfoFilePath = "/config/device_base_config.json";
    
    DynamicJsonDocument* LoadWiFiConfig();
    void ReWriteWiFiConfig();
    DynamicJsonDocument *WifiConfig = new DynamicJsonDocument(10000);
    String WiFiConfigFilePath = "/config/wifi_config.json";

    DynamicJsonDocument* GetDeviceSetting();
    DynamicJsonDocument *DeviceSetting = new DynamicJsonDocument(300000);
  private:
};

#endif