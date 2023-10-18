// 如果要更改 log level，請去這邊找
// #include <sdkconfig.h>
// #undef CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL
// #define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL 5
// #undef ARDUHAL_LOG_LEVEL
// #define ARDUHAL_LOG_LEVEL 5

#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>
#include <ArduinoJson.h>
#include "CalcFunction.h"
#include "Machine_Ctrl/src/Machine_Ctrl.h"

//TODO oled暫時這樣寫死
// #include <Wire.h> 
// #include <Adafruit_GFX.h>
// #include "Adafruit_SH1106.h"
// #include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
#include "../lib/QRCode/src/qrcode.h"
//TODO oled暫時這樣寫死

//TODO
#include <vector>
#include <map>
//TODO


#include <SD.h>
// #include <Adafruit_Sensor.h>
// #include <DHT.h>
// #include <DHT_U.h>
#include <HTTPClient.h>

// #include "Adafruit_MCP9808.h"
// Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
#include "DFRobot_MCP9808.h"
// DFRobot_MCP9808_I2C mcp9808(&Machine_Ctrl.WireOne, 0x18);
// DFRobot_MCP9808_I2C mcp9808_other(&Machine_Ctrl.WireOne, 0x19);

const char* FIRMWARE_VERSION = "V3.23.1003.0";

void scanI2C();

void setup() {
  Serial.begin(115200);
  
  pinMode(16, OUTPUT);
  digitalWrite(16, HIGH);
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);

  ESP_LOGD("", "儀器啟動，韌體版本為: %s", FIRMWARE_VERSION);
  Machine_Ctrl.PrintOnScreen("Rebooting...");
  Machine_Ctrl.PrintOnScreen("Load SD");
  ESP_LOGD("", "準備讀取SD卡內資訊");
  Machine_Ctrl.INIT_SD_And_LoadConfigs();
  Machine_Ctrl.PrintOnScreen("Load SPIFFS");
  ESP_LOGD("", "準備讀取SPIFFS內資訊");
  Machine_Ctrl.INIT_SPIFFS_And_LoadConfigs();
  
  Machine_Ctrl.PrintOnScreen("Load Configs");
  ESP_LOGD("", "準備更新最新的各池感測器資料");
  Machine_Ctrl.INIT_PoolData();

  Machine_Ctrl.PrintOnScreen("INIT Motors");
  ESP_LOGD("", "準備初始化蠕動馬達控制模組");
  Machine_Ctrl.peristalticMotorsCtrl.INIT_Motors(42,41,40,2);
  ESP_LOGD("", "準備初始化伺服馬達控制模組(PCA9685)");
  Machine_Ctrl.motorCtrl.INIT_Motors(Machine_Ctrl.WireOne);
  ESP_LOGD("", "停止所有馬達動作");
  Machine_Ctrl.StopDeviceAndINIT();

  Machine_Ctrl.PrintOnScreen("Load Logs");
  ESP_LOGD("", "準備讀取log資訊");
  Machine_Ctrl.SD__LoadOldLogs();

  ESP_LOGD("", "準備使用WIFI連線");
  Machine_Ctrl.PrintOnScreen("Connect Wifi");
  Machine_Ctrl.BackendServer.ConnectToWifi();
  Machine_Ctrl.PrintOnScreen("Update Time");
  ESP_LOGD("", "準備更新儀器時間");
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.PrintOnScreen("API Server Start");
  ESP_LOGD("", "準備啟動Server");
  Machine_Ctrl.BackendServer.ServerStart();
  Machine_Ctrl.ShowIPAndQRCodeOnOled();
  Machine_Ctrl.SetLog(3, "機器開機", "");
  ESP_LOGD("", "儀器啟動完畢!");
  digitalWrite(16, LOW);
  digitalWrite(17, LOW);
  
  delay(1000);
}

void loop() {
  vTaskDelay(9999999/portTICK_PERIOD_MS);
}

void scanI2C(){
  byte error, address;
  int devices = 0;
  for (address = 1; address < 127; address++) {
    Machine_Ctrl.WireOne.beginTransmission(address);
    error = Machine_Ctrl.WireOne.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);

      devices++;
    }
  }
  if (devices == 0) {
    Serial.println("No I2C devices found");
  }
}