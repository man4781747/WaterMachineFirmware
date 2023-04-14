#include "SPIFFS_Ctrl.h"

#include <esp_log.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "../../Machine_Base_info/src/Machine_Base_info.h"

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
  LoadMachineSetting();
}


void SPIFFS_Ctrl::LoadMachineSetting()
{
  if (!SPIFFS.exists("/config")) {
    SPIFFS.mkdir("/config");
  }
  if (!SPIFFS.exists("/config/base_config.json")) {
    ESP_LOGW(LOG_TAG_SPIFFS, "Can't open /config/base_config.json in SPIFFS ! Rebuild it !");
    MachineInfo.device_no = "Wxxxxx";
    File file = SPIFFS.open("/config/base_config.json", FILE_WRITE);
    file.print(MachineInfo.GetInfoJSONString());
  } else {
    File file = SPIFFS.open("/config/base_config.json", FILE_READ);
    StaticJsonDocument<200> json_doc;
    deserializeJson(json_doc, file.readString());
    MachineInfo.LoadInfoByJSONItem(json_doc);
  }

}

String SPIFFS_Ctrl::GetMotorSetting()
{
  if (!SPIFFS.exists("/config")) {
    SPIFFS.mkdir("/config");
  }
  if (!SPIFFS.exists("/config/motor_config.json")) {
    ESP_LOGW(LOG_TAG_SPIFFS, "Can't open /config/motor_config.json in SPIFFS ! Rebuild it !");
    MachineInfo.device_no = "Wxxxxx";
    File file = SPIFFS.open("/config/motor_config.json", FILE_WRITE);
    file.print(MachineInfo.GetInfoJSONString());
  } else {
    File file = SPIFFS.open("/config/motor_config.json", FILE_READ);
    StaticJsonDocument<200> json_doc;
    deserializeJson(json_doc, file.readString());
    MachineInfo.LoadInfoByJSONItem(json_doc);
  }

}