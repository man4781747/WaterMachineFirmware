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
#include <regex>

#include "../lib/LTR_329ALS_01/src/LTR_329ALS_01.h"

#include "Machine_Ctrl/src/Machine_Ctrl.h"

const char* LOG_TAG = "MAIN";
SMachine_Ctrl Machine_Ctrl;


void setup() {
  Serial.begin(115200);
  Serial.println("START");

  Machine_Ctrl.INIT_SPIFFS_config();
  Machine_Ctrl.INIT_I2C_Wires();
  // Machine_Ctrl.LTR_329ALS_01_Ctrler.ALS_Contr_Config.ALS_Gain = ALS_Gain::Gain_96X;

  Machine_Ctrl.BackendServer.ConnectToWifi();
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.BackendServer.ServerStart();
  
}

void loop() {
  // ALS_01_Data_t testValue = Machine_Ctrl.LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  delay(10000);
  // ArduinoOTA.handle();
}

