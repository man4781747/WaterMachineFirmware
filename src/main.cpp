// 如果要更改 log level，請去這邊找
// #include <sdkconfig.h>
// #undef CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL
// #define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL 5
// #undef ARDUHAL_LOG_LEVEL
// #define ARDUHAL_LOG_LEVEL 5


#include <Arduino.h>
// #include <Wire.h>
#include <esp_log.h>
#include <ArduinoJson.h>


#include "Machine_Ctrl/src/Machine_Ctrl.h"

const char* LOG_TAG = "MAIN";
SMachine_Ctrl Machine_Ctrl;

int test = 90;

void setup() {
  Serial.begin(115200);

  Machine_Ctrl.INIT_SPIFFS_config();


  // String FileContent;
  // serializeJsonPretty(*(Machine_Ctrl.DeviceSetting), FileContent);
  // Serial.println(FileContent);


  Machine_Ctrl.BackendServer.ConnectToWifi("IDWATER","56651588");
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.BackendServer.ServerStart();

  Machine_Ctrl.poolsCtrl.addNewPool("pool-1");
  Machine_Ctrl.poolsCtrl.addNewPool("pool-2");
  Machine_Ctrl.poolsCtrl.addNewPool("pool-3");
  Machine_Ctrl.poolsCtrl.addNewPool("pool-4");

  Machine_Ctrl.INIT_SW_Moter();
  Machine_Ctrl.Build_SwitchMotorScan();

  Machine_Ctrl.INIT_Peristaltic_Moter();
  Machine_Ctrl.Build_PeristalticMotorScan();

  Machine_Ctrl.INIT_UpdateEventGroupSetting();
  Machine_Ctrl.INIT_UpdateStepGroupSetting();
  Machine_Ctrl.GetAllEventSetting();
  Machine_Ctrl.GetAllStepSetting();
}

void loop() {
  delay(60000);
}

