#ifndef SPIFFS_Ctrl_H
#define SPIFFS_Ctrl_H

#include <Arduino.h>

#include <ArduinoJson.h>

#include <Machine_Base_info.h>

class SPIFFS_Ctrl
{
  public:
    SPIFFS_Ctrl(void){};
    void INIT_SPIFFS();
    
    void CreateFile(String FilePath);
    void CreateFolder(String FolderPath);

    Machine_Info LoadMachineSetting();
    void ReWriteMachineSettingFile(MachineInfo_t MachineInfo_);

    DynamicJsonDocument* GetDeviceSetting();
    void ReWriteDeviceSetting();
    DynamicJsonDocument *DeviceSetting = new DynamicJsonDocument(100000);

  private:
};

#endif