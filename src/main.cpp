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

  Machine_Ctrl.BackendServer.ConnectToWifi("IDWATER","56651588");
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.BackendServer.ServerStart();


  // JsonObject TestData = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>();

  // Machine_Ctrl.RUN__PWMMotorEvent(
  //   TestData["event_group"]["Clear_MixRoom_Path"]["event"][0]["pwm_motor_list"].as<JsonArray>()
  // );

  // Machine_Ctrl.poolsCtrl.addNewPool("pool-1");
  // Machine_Ctrl.poolsCtrl.addNewPool("pool-2");
  // Machine_Ctrl.poolsCtrl.addNewPool("pool-3");
  // Machine_Ctrl.poolsCtrl.addNewPool("pool-4");

  Machine_Ctrl.INIT_SW_Moter();
  // Machine_Ctrl.INIT_Peristaltic_Moter();
  // Machine_Ctrl.INIT_UpdateEventGroupSetting();
  // Machine_Ctrl.INIT_UpdateStepGroupSetting();

  // Machine_Ctrl.LoadNewSettings("TEST", "ME");
  // Serial.println(
  //   Machine_Ctrl.GetRunHistoryDetailString()
  // );

  // Machine_Ctrl.Build_SwitchMotorScan();
  // Machine_Ctrl.Build_PeristalticMotorScan();

  // Machine_Ctrl.LoadStepToRunHistoryItem("TEST","ME");

  // Machine_Ctrl.RUN__History();

  // delay(3000);
  // Machine_Ctrl.STOP_AllTask();

  // delay(10000);
  // Machine_Ctrl.RESUME_AllTask();

  // while (Machine_Ctrl.TASK__History != NULL) {

    
  //   delay(10);
    
  // }
}

void loop() {
  // ESP_LOGW("loop","loop");
  // JsonObject TestData = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>();
  // Machine_Ctrl.RUN__PWMMotorEvent(
  //   TestData["event_group"]["Clear_MixRoom_Path"]["event"][0]["pwm_motor_list"].as<JsonArray>()
  // );

  // JsonObject TestData = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>();
  // Machine_Ctrl.RUN__PeristalticMotorEvent(
  //   TestData["event_group"]["Clear_MixRoom_Path"]["event"][1]["peristaltic_motor_list"].as<JsonArray>()
  // );


  // Machine_Ctrl.RUN__EventGroup(TestData["event_group"]["Clear_MixRoom_Path"]);

  // JsonObject DeviceSetting = Machine_Ctrl.spiffs.DeviceSetting->as<JsonObject>();
  // JsonObject D_history =  DeviceSetting["run_history"];
  // String printString;
  // serializeJsonPretty(D_history, printString);
  // Serial.println(printString);
  // delay(3);


  // Machine_Ctrl.LoadStepToRunHistoryItem("TEST","ME");

  // Machine_Ctrl.RUN__History();

  // while (Machine_Ctrl.TASK__History != NULL) {
  //   delay(10);
  // }

  delay(999000);
}

