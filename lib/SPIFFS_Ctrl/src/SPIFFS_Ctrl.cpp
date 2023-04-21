#include "SPIFFS_Ctrl.h"

#include <esp_log.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include <Machine_Base_info.h>

extern Machine_Info MachineInfo;

const char* LOG_TAG_SPIFFS = "SPIFFS";

void SPIFFS_Ctrl::INIT_SPIFFS()
{
  if(!SPIFFS.begin(true)){
    ESP_LOGE(LOG_TAG_SPIFFS, "An Error has occurred while mounting SPIFFS");
    for (;;) {
      delay(5000);
    }
  }
}

/**
 * @brief 獲得儀器設定
 * 
 * @return Machine_Info 
 */
Machine_Info SPIFFS_Ctrl::LoadMachineSetting()
{
  Machine_Info MachineInfo;

  if (!SPIFFS.exists("/config/base_config.json")) {
    CreateFile("/config/base_config.json");
    ESP_LOGW(LOG_TAG_SPIFFS, "Can't open /config/base_config.json in SPIFFS ! Rebuild it !");
    MachineInfo.MachineInfo.device_no = "Wxxxxx";
    File file = SPIFFS.open("/config/base_config.json", FILE_WRITE);
    file.print(MachineInfo.GetDeviceInfoString());
    file.close();
  } else {
    File file = SPIFFS.open("/config/base_config.json", FILE_READ);
    String FileContent = file.readString();
    file.close();
    ESP_LOGE(LOG_TAG_SPIFFS, "config: %s",FileContent.c_str());
    DynamicJsonDocument json_doc(10000);
    DeserializationError error = deserializeJson(json_doc, FileContent);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
    }
    MachineInfo.LoadInfoByJSONItem(json_doc);

    json_doc.clear();
  }
  return MachineInfo;
}

/**
 * @brief 如果不存在，建立檔案
 * 
 * @param FilePath 
 */
void SPIFFS_Ctrl::CreateFile(String FilePath){
  ESP_LOGI(LOG_TAG_SPIFFS, "CreateFile: %s", FilePath.c_str());
  String folderPath = FilePath.substring(0,FilePath.lastIndexOf('/')+1);
  if (folderPath.indexOf('/') == 0){
    folderPath = folderPath.substring(1,folderPath.length());
  }
  String folderPathStack = "";
  int index;
  int length;
  while (1){
    index = folderPath.indexOf('/');
    if (index <0) {
      break;
    }
    folderPathStack = folderPathStack + "/" + folderPath.substring(0,index);
    folderPath = folderPath.substring(index+1,folderPath.length());
    if (SPIFFS.exists(folderPathStack) == false){
      ESP_LOGD(LOG_TAG_SPIFFS, "Create folder: %s", folderPathStack.c_str());
      SPIFFS.mkdir(folderPathStack);
    }
  }
  if (SPIFFS.exists(FilePath) == false){
    File selectedFile = SPIFFS.open(FilePath, FILE_WRITE);
    selectedFile.close();
  }
}

/**
 * @brief 如果不存在，建立資料夾
 * 
 * @param FolderPath 
 */
void SPIFFS_Ctrl::CreateFolder(String FolderPath){
  ESP_LOGI(LOG_TAG_SPIFFS, "CreateFile: %s", FolderPath.c_str());
  String folderPath = FolderPath.substring(0,FolderPath.lastIndexOf('/')+1);
  if (folderPath.indexOf('/') == 0){
    folderPath = folderPath.substring(1,folderPath.length());
  }
  String folderPathStack = "";
  int index;
  int length;
  while (1){
    index = folderPath.indexOf('/');
    if (index <0) {
      break;
    }
    folderPathStack = folderPathStack + "/" + folderPath.substring(0,index);
    folderPath = folderPath.substring(index+1,folderPath.length());
    if (SPIFFS.exists(folderPathStack) == false){
      ESP_LOGD(LOG_TAG_SPIFFS, "Create folder: %s", folderPathStack.c_str());
      SPIFFS.mkdir(folderPathStack);
    }
  }
  if (SPIFFS.exists(FolderPath) == false){
    ESP_LOGD(LOG_TAG_SPIFFS, "Create folder: %s", FolderPath.c_str());
    SPIFFS.mkdir(FolderPath);
  }
}


void SPIFFS_Ctrl::ReWriteMachineSettingFile(MachineInfo_t MachineInfo_)
{
  DynamicJsonDocument json_doc(10000);
  json_doc["device_no"].set(MachineInfo_.device_no);
  json_doc["FIRMWARE_VERSION"].set(MachineInfo_.FIRMWARE_VERSION);
  json_doc["mode"].set(MachineInfo_.mode);
  json_doc["time_interval"].set(*MachineInfo_.time_interval);
  void* json_output = malloc(10000);
  serializeJsonPretty(json_doc, json_output, 6000);
  String returnString = String((char*)json_output);
  json_doc.clear();
  free(json_output);
  ESP_LOGD(LOG_TAG_SPIFFS, "New config content: %s", returnString.c_str());
  CreateFile("/config/base_config.json");
  File file = SPIFFS.open("/config/base_config.json", FILE_WRITE);
  file.print(returnString);
  file.close();
}

DynamicJsonDocument* SPIFFS_Ctrl::GetDeviceSetting()
{
  if (!SPIFFS.exists("/config/event_config.json")) {
    CreateFile("/config/event_config.json");
    ESP_LOGW(LOG_TAG_SPIFFS, "Can't open /config/event_config.json in SPIFFS ! Rebuild it !");
    String fileString;
    serializeJsonPretty(*DeviceSetting, fileString);
    File file = SPIFFS.open("/config/event_config.json", FILE_WRITE);
    file.print(fileString);
    file.close();
  } else {
    File file = SPIFFS.open("/config/event_config.json", FILE_READ);
    String FileContent = file.readString();
    file.close();
    DeserializationError error = deserializeJson(*DeviceSetting, FileContent);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.f_str());
    }
  }
  return DeviceSetting;
}


