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


  // Machine_Ctrl.RUN_NO2_Original_Value();
  Machine_Ctrl.RUN_EVENT(
    &RUN_NO2_Original_Value
  );
}

void loop() {
  Machine_Ctrl.BackendServer.UploadNewData();
  // Machine_Ctrl.LoopTest();
  // RUN_MOTOR_GROUP PWM_TEST {
  //   "測試", "測試",
  //   new PWM_MOTOR_STATUS_SET_OBJ[17] {
  //     {PWM_POSITION_MAPPING::S_M0, test},
  //     {PWM_POSITION_MAPPING::S_M1, test},
  //     {PWM_POSITION_MAPPING::S_M2, test},
  //     {PWM_POSITION_MAPPING::S_M3, test},
  //     {PWM_POSITION_MAPPING::S_M4, test},
  //     {PWM_POSITION_MAPPING::S_M5, test},
  //     {PWM_POSITION_MAPPING::S_M6, test},
  //     {PWM_POSITION_MAPPING::S_M7, test},
  //     {PWM_POSITION_MAPPING::S_M8, test},
  //     {PWM_POSITION_MAPPING::S_PH1, test},
  //     {PWM_POSITION_MAPPING::S_PH2, test},
  //     {PWM_POSITION_MAPPING::S_L1, test},
  //     {PWM_POSITION_MAPPING::S_L2, test},
  //     {PWM_POSITION_MAPPING::S_B1, test},
  //     {PWM_POSITION_MAPPING::S_B2, test},
  //     {PWM_POSITION_MAPPING::S_B3, test},
  //     {PWM_POSITION_MAPPING::S_B4, test},
  //   }, 17,
  //   {PERISTALTIC_MOTOR_MAPPING::M4, PeristalticMotorStatus::FORWARD, 0}
  // };
  // Machine_Ctrl.SwitchPWMMotor__AND__RunPeristalticMotor(&PWM_TEST);
  // if (test == 90) {
  //   test = 180;
  // } else if (test == 180) {
  //   test = 0;
  // } else {
  //   test = 90;
  // }




  // https://www.rapidtables.com/convert/number/binary-to-hex.html
  // Machine_Ctrl.spiffs.ReWriteMachineSettingFile(
  //   Machine_Ctrl.MachineInfo.MachineInfo
  // );
  // Machine_Ctrl.PumpPoolWaterToTempTank();
  delay(60000);
}

