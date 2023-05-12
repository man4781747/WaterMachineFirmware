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

#include "../lib/LTR_329ALS_01/src/LTR_329ALS_01.h"

#include "Machine_Ctrl/src/Machine_Ctrl.h"


/////////////////
#include <regex>
#include <map>
#include <vector>

#include <HTTPClient.h>
HTTPClient http;
ALS_01_Data_t testValue;
time_t nowTime;
String postData;
DynamicJsonDocument newData(500);
char datetimeChar[30];

/////////////////////////

const char* LOG_TAG = "MAIN";
SMachine_Ctrl Machine_Ctrl;

const char* FIRMWARE_VERSION = "V2.23.52.0";


int SCHP = 18; // Clock
int STHP = 17; // Latch
int DATA = 16;

void setup() {
  Serial.begin(115200);
  Serial.println("START");

  Machine_Ctrl.INIT_SPIFFS_config();
  Machine_Ctrl.INIT_I2C_Wires();
  
  pinMode(SCHP, OUTPUT);
  pinMode(DATA, OUTPUT);  
  pinMode(STHP, OUTPUT);
  Machine_Ctrl.BackendServer.ConnectToWifi();
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.BackendServer.ServerStart();
}

void loop() {
  delay(10000);



  // digitalWrite(STHP, LOW);
  // shiftOut(DATA, SCHP, LSBFIRST, 0b11000000);
  // digitalWrite(STHP, HIGH);
  // delay(500);
  // Machine_Ctrl.LTR_329ALS_01_Ctrler.ALS_Contr_Config.ALS_Gain = ALS_Gain::Gain_96X;
  // testValue = Machine_Ctrl.LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // newData.clear();
  // postData = "";
  // newData["SensorIndex"] = "1";
  // newData["Type"] = "96X";
  // newData["CH0"] = testValue.CH_0;
  // newData["CH1"] = testValue.CH_1;
  // nowTime = now();
  // sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
  //   year(nowTime), month(nowTime), day(nowTime),
  //   hour(nowTime), minute(nowTime), second(nowTime)
  // );
  // newData["DataTime"] = datetimeChar;
  // http.begin("http://192.168.20.27:5566/data");
  // http.addHeader("Content-Type", "application/json");
  // serializeJsonPretty(newData, postData);
  // http.POST(postData);
  // delay(2500);
  // http.end();

  // digitalWrite(STHP, LOW);
  // shiftOut(DATA, SCHP, LSBFIRST, 0b00110000);
  // digitalWrite(STHP, HIGH);
  // delay(500);
  // Machine_Ctrl.LTR_329ALS_01_Ctrler.ALS_Contr_Config.ALS_Gain = ALS_Gain::Gain_96X;
  // testValue = Machine_Ctrl.LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // newData.clear();
  // postData = "";
  // newData["SensorIndex"] = "2";
  // newData["Type"] = "96X";
  // newData["CH0"] = testValue.CH_0;
  // newData["CH1"] = testValue.CH_1;
  // nowTime = now();
  // sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
  //   year(nowTime), month(nowTime), day(nowTime),
  //   hour(nowTime), minute(nowTime), second(nowTime)
  // );
  // newData["DataTime"] = datetimeChar;
  // http.begin("http://192.168.20.27:5566/data");
  // http.addHeader("Content-Type", "application/json");
  // serializeJsonPretty(newData, postData);
  // http.POST(postData);
  // delay(2500);
  // http.end();

  // ArduinoOTA.handle();

}

