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

// #include <Machine_Ctrl.h>
// #include <Wifi_Ctrl.h>
// #include <Machine_Base_info.h>
// #include <SPIFFS_Ctrl.h>
// #include <Motor_Ctrl.h>
#include "Machine_Ctrl/src/Machine_Ctrl.h"
// #include <Pools_Ctrl.h>
// #include "../lib/Machine_Ctrl/src/Machine_Ctrl.h"
// #include "../lib/Wifi_Ctrl/src/Wifi_Ctrl.h"
// #include "../lib/Machine_Base_info/src/Machine_Base_info.h"
// #include "../lib/SPIFFS_Ctrl/src/SPIFFS_Ctrl.h"
// #include "../lib/Motor_Ctrl/src/Motor_Ctrl.h"
// #include "../lib/Pools_Ctrl/src/Pools_Ctrl.h"

const char* LOG_TAG = "MAIN";
// SMachine_Ctrl Machine_Ctrl;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Machine_Ctrl.spiffs.INIT_SPIFFS();
  // ESP_LOGI(LOG_TAG,"Machine name : %s", Machine_Ctrl.MachineInfo.device_no.c_str());

  // Machine_Ctrl.BackendServer.ConnectToWifi("IDWATER","56651588");
  // Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  // Machine_Ctrl.BackendServer.ServerStart();

  // Machine_Ctrl.poolsCtrl.addNewPool("pool-1");
  // Machine_Ctrl.poolsCtrl.addNewPool("pool-2");
  // Machine_Ctrl.poolsCtrl.addNewPool("pool-3");
  // Machine_Ctrl.poolsCtrl.addNewPool("pool-4");

  // Machine_Ctrl.motorCtrl.INIT_Motors();
  // Machine_Ctrl.motorCtrl.AddNewMotor(15);
  // Machine_Ctrl.motorCtrl.AddNewMotor(14);
  // Machine_Ctrl.motorCtrl.AddNewMotor(13);
  // Machine_Ctrl.motorCtrl.AddNewMotor(12);
  // Machine_Ctrl.motorCtrl.AddNewMotor(11);

}

void loop() {
  // https://www.rapidtables.com/convert/number/binary-to-hex.html
  // Machine_Ctrl.PumpPoolWaterToTempTank();
  
  // Machine_Ctrl.ChangeMotorStatus(MOTOR_STATUS::ALL_CLOSE);
  // delay(2000);
  // Machine_Ctrl.ChangeMotorStatus(MOTOR_STATUS::GET_TEMP_TANK);
  // delay(2000);
  // Machine_Ctrl.ChangeMotorStatus(MOTOR_STATUS::GET_REAGENT_1);
  // delay(2000);
  // Machine_Ctrl.ChangeMotorStatus(MOTOR_STATUS::GET_REAGENT_2);
  // delay(2000);
  // Machine_Ctrl.ChangeMotorStatus(MOTOR_STATUS::GET_REAGENT_3);
  // delay(2000);
  // Machine_Ctrl.ChangeMotorStatus(MOTOR_STATUS::GET_REAGENT_4);
  // BackendServer.UploadNewData();
  delay(60000);
}

