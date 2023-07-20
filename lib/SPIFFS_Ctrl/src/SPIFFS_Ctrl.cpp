#include "SPIFFS_Ctrl.h"

#include <esp_log.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

const char* LOG_TAG_SPIFFS = "SPIFFS";

void SPIFFS_Ctrl::INIT_SPIFFS()
{
  if(!SPIFFS.begin(true)){
    ESP_LOGE(LOG_TAG_SPIFFS, "An Error has occurred while mounting SPIFFS");
    for (;;) {
      delay(5000);
    }
  }
  SPIFFS.remove("/otaTempFile.bin");
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


/**
 * @brief 獲得儀器設定
 * 
 * @return Machine_Info 
 */
DynamicJsonDocument* SPIFFS_Ctrl::LoadDeviceBaseInfo()
{
  if (!SPIFFS.exists(DeviceBaseInfoFilePath)) {
    CreateFile(DeviceBaseInfoFilePath);
    ESP_LOGW(LOG_TAG_SPIFFS, "Can't open %s in SPIFFS ! Rebuild it !", DeviceBaseInfoFilePath.c_str());
    JsonObject DeviceBaseInfoJSON = DeviceBaseInfo->as<JsonObject>();
    DeviceBaseInfoJSON["device_no"].set("xxxxxx");
    DeviceBaseInfoJSON["mode"].set("Mode_Slave");
    String fileString;
    serializeJsonPretty(DeviceBaseInfoJSON, fileString);
    File file = SPIFFS.open(DeviceBaseInfoFilePath, FILE_WRITE);
    file.print(fileString);
    file.close();
  } else {
    File file = SPIFFS.open(DeviceBaseInfoFilePath, FILE_READ);
    String FileContent = file.readString();
    // Serial.println(FileContent);
    file.close();
    if (FileContent.length() != 0) {
      DeserializationError error = deserializeJson(*DeviceBaseInfo, FileContent);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
      }
    }
  }
  return DeviceBaseInfo;
}


void SPIFFS_Ctrl::ReWriteDeviceBaseInfo()
{
  String DeviceBaseInfoString;
  serializeJsonPretty(*DeviceBaseInfo, DeviceBaseInfoString);
  File file = SPIFFS.open(DeviceBaseInfoFilePath, FILE_WRITE);
  file.print(DeviceBaseInfoString);
  file.close();
}


DynamicJsonDocument* SPIFFS_Ctrl::LoadWiFiConfig()
{
  if (!SPIFFS.exists(WiFiConfigFilePath)) {
    CreateFile(WiFiConfigFilePath);
    ESP_LOGW(LOG_TAG_SPIFFS, "Can't open %s in SPIFFS ! Rebuild it !", WiFiConfigFilePath.c_str());
    JsonObject WifiConfigJSON = WifiConfig->as<JsonObject>();
    WifiConfigJSON["AP_IP"].set("127.0.0.1");
    WifiConfigJSON["AP_gateway"].set("127.0.0.1");
    WifiConfigJSON["AP_subnet_mask"].set("255.255.255.0");

    WifiConfigJSON["AP_Name"].set("AkiraTest");
    WifiConfigJSON["AP_Password"].set("12345678");

    WifiConfigJSON["remote_IP"].set("127.0.0.1");
    WifiConfigJSON["remote_Name"].set("xxxxx");
    WifiConfigJSON["remote_Password"].set("12345678");
    String fileString;
    serializeJsonPretty(*WifiConfig, fileString);
    File file = SPIFFS.open(WiFiConfigFilePath, FILE_WRITE);
    file.print(fileString);
    file.close();
  } else {
    File file = SPIFFS.open(WiFiConfigFilePath, FILE_READ);
    String FileContent = file.readString();
    file.close();
    Serial.println(FileContent);
    if (FileContent.length() != 0) {
      DeserializationError error = deserializeJson(*WifiConfig, FileContent);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
      }
    }
  }
  return WifiConfig;
}

void SPIFFS_Ctrl::ReWriteWiFiConfig()
{
  String WiFiConfigString;
  serializeJsonPretty(*WifiConfig, WiFiConfigString);
  File file = SPIFFS.open(WiFiConfigFilePath, FILE_WRITE);
  file.print(WiFiConfigString);
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
    if (FileContent.length() != 0) {
      DeserializationError error = deserializeJson(*DeviceSetting, FileContent);
      if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.f_str());
      }
    } else {
      File fileTemp = SPIFFS.open("/config/config_temp.json", FILE_READ);
      String tempFileContent = fileTemp.readString();
      fileTemp.close();
      // Serial.printf("temp file length: %d\r\n", tempFileContent.length());
      // Serial.printf("temp file content: %s\r\n", tempFileContent.c_str());
      deserializeJson(*DeviceSetting, tempFileContent);
    }

  }
  return DeviceSetting;
}

