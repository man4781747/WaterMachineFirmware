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
// #include <Adafruit_GFX.h>
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

void testWeb(int index, int type, String desp);

const char* FIRMWARE_VERSION = "V2.23.73.2";

void scanI2C();


void setup() {
  Serial.begin(115200);
  Machine_Ctrl.INIT_SD_Card();
  Machine_Ctrl.INIT_SPIFFS_config();
  Machine_Ctrl.LoadPiplineConfig();
  Machine_Ctrl.INIT_I2C_Wires();
  Machine_Ctrl.INIT_PoolData();
  
  Machine_Ctrl.peristalticMotorsCtrl.INIT_Motors(42,41,40,2);

  Machine_Ctrl.motorCtrl.INIT_Motors(Machine_Ctrl.WireOne);

  Machine_Ctrl.StopDeviceAndINIT();
 
  Machine_Ctrl.LoadOldLogs();

  Machine_Ctrl.BackendServer.ConnectToWifi();
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.BackendServer.ServerStart();
  Machine_Ctrl.ShowIPAndQRCodeOnOled();
  Machine_Ctrl.SetLog(3, "機器開機", "");
  
  //TODO
  // DynamicJsonDocument pipelineConfig(65525);
  // File testContent = SD.open("/pipelines/test_1.json", FILE_READ);
  // deserializeJson((*Machine_Ctrl.pipelineConfig), testContent.readString());
  // testContent.close();
  // // serializeJsonPretty(testConfig["pipline"], Serial);

  // //? pipelineConfig: 整個流程運行中都要存在的變數
  // JsonArray piplineArray = (*Machine_Ctrl.pipelineConfig)["pipline"].as<JsonArray>();

  // //? 將原本的pipeline流程設定轉換成後面Task好追蹤的格式
  // DynamicJsonDocument piplineSave(65525);
  // for (JsonArray singlePiplineArray : piplineArray) {
  //   // serializeJson(singlePiplineArray, Serial);
  //   // Serial.println();
  //   String ThisNodeNameString = singlePiplineArray[0].as<String>();
  //   // ESP_LOGI("", "處裡流成: %s", ThisNodeNameString.c_str());
  //   if (!piplineSave.containsKey(ThisNodeNameString)) {
  //     piplineSave[ThisNodeNameString]["Name"] = ThisNodeNameString;
  //     piplineSave[ThisNodeNameString].createNestedObject("childList");
  //     piplineSave[ThisNodeNameString].createNestedObject("parentList");
  //   }
  //   for (String piplineChildName :  singlePiplineArray[1].as<JsonArray>()) {
  //     if (!piplineSave.containsKey(piplineChildName)) {
  //       piplineSave[piplineChildName]["Name"] = piplineChildName;
  //       piplineSave[piplineChildName].createNestedObject("childList");
  //       piplineSave[piplineChildName].createNestedObject("parentList");
  //     }

  //     if (!piplineSave[ThisNodeNameString]["childList"].containsKey(piplineChildName)){
  //       piplineSave[ThisNodeNameString]["childList"].createNestedObject(piplineChildName);
  //       // ESP_LOGI("", "%s 新增一個 child: %s", ThisNodeNameString.c_str(), piplineChildName.c_str());
  //     }

  //     if (!piplineSave[piplineChildName]["parentList"].containsKey(ThisNodeNameString)){
  //       piplineSave[piplineChildName]["parentList"].createNestedObject(ThisNodeNameString);
  //       // ESP_LOGI("", "%s 新增一個 parent: %s", piplineChildName.c_str(), ThisNodeNameString.c_str());
  //     }
  //   }
  // }
  // (*Machine_Ctrl.pipelineConfig)["pipline"].set(piplineSave);
  // //? 將 steps_group 內的資料多加上key值: RESULT 來讓後續Task可以判斷流程是否正確執行
  // JsonObject stepsGroup = (*Machine_Ctrl.pipelineConfig)["steps_group"].as<JsonObject>();
  // for (JsonPair stepsGroupItem : stepsGroup) {
  //   String stepsGroupName = String(stepsGroupItem.key().c_str());
  //   (*Machine_Ctrl.pipelineConfig)["steps_group"][stepsGroupName]["RESULT"].set("WAIT");
  // };
  // for (JsonPair stepsGroupItem : stepsGroup) {
  //   String stepsGroupName = String(stepsGroupItem.key().c_str());
  //   Machine_Ctrl.AddNewPiplelineFlowTask(stepsGroupName);
  // };
  //TODO

  
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(7);
  delay(1000);
}

void loop() {

  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%s, %d,%d,%.2f\n", Machine_Ctrl.GetDatetimeString().c_str(), test.CH_0, test.CH_1,mcp9808_other.getTemperature());
  // uint16_t CH_Buff [30];
  // uint16_t Temp_Buff[30];
  // for (int i=0;i<30;i++) {
  //   test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //   CH_Buff[i] = test.CH_0;
  //   // Temp_Buff[i] = (uint16_t)(mcp9808_other.getTemperature());
  // }

  // Serial.printf("%s, %.2f,%.2f\n", 
  //   Machine_Ctrl.GetDatetimeString().c_str(), 
  //   afterFilterValue(CH_Buff,30), 
  //   mcp9808_other.getTemperature()
  //   // afterFilterValue(Temp_Buff,30)
  // );

  // for (int i=151;i<156;i=i+1) {
  //   Machine_Ctrl.WireOne.beginTransmission(0x2F);
  //   Machine_Ctrl.WireOne.write(0b00000000);
  //   Machine_Ctrl.WireOne.write(i);
  //   Machine_Ctrl.WireOne.endTransmission();
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_48X);
  //   test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //   Serial.printf("%s, %d,%d,%d,%.2f\n", Machine_Ctrl.GetDatetimeString().c_str(), i, test.CH_0, test.CH_1,mcp9808_other.getTemperature());
    // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
    // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();

    // Serial.printf("%d,%d,%.2f\n", test.CH_0, test.CH_1,mcp9808_other.getTemperature());
    // Serial.printf("%d,%d,%.2f\n", test.CH_0, test.CH_1,mcp9808_other.getTemperature());
    // mcp9808_other.wakeUpMode();
    // mcp9808_other.setResolution(RESOLUTION_0_0625);

  // }
  // Machine_Ctrl.GetDatetimeString();
  // testWeb(0, 148, "綠光測試: 148");
  // testWeb(1, 248, "藍光測試: 248");
  // for (int indexChose = 0;indexChose<2;indexChose++) {
    // for (int typeChose = 0;typeChose<=200;typeChose=typeChose+10) {
      // testWeb(indexChose, 160);
    // }
  // }
  
  // digitalWrite(48, HIGH);
  // delay(500);
  // Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(0, PeristalticMotorStatus::FORWARD);
  // Machine_Ctrl.peristalticMotorsCtrl.RunMotor(
  //   Machine_Ctrl.peristalticMotorsCtrl.moduleDataList
  // );
  // delay(1500);
  // Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
  // digitalWrite(48, LOW);
  // scanI2C();
  // Machine_Ctrl.ShowIPAndQRCodeOnOled();
  delay(2000);
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

void testWeb(int index, int type, String desp) {
  HTTPClient http;
  DynamicJsonDocument PostData_V2(10000);
  uint16_t CH0_Buff_V2 [30];
  uint16_t CH1_Buff_V2 [30];
  uint16_t Temp_Buff[30];

  Machine_Ctrl.WireOne.beginTransmission(0x70);
  Machine_Ctrl.WireOne.write(1 << index);
  Machine_Ctrl.WireOne.endTransmission();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(index);
  delay(100);
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  Machine_Ctrl.WireOne.beginTransmission(0x2F);
  Machine_Ctrl.WireOne.write(0b00000000);
  Machine_Ctrl.WireOne.write(type);
  Machine_Ctrl.WireOne.endTransmission();
  for (int i=0;i<30;i++) {
    ALS_01_Data_t test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
    CH0_Buff_V2[i] = test.CH_0;
    CH1_Buff_V2[i] = test.CH_1;
  }
  PostData_V2["CH0"].set(afterFilterValue(CH0_Buff_V2,30));
  PostData_V2["CH1"].set(afterFilterValue(CH1_Buff_V2,30));
  //TODO 溫度測試
  DFRobot_MCP9808_I2C mcp9808(&Machine_Ctrl.WireOne, 0x18);
  DFRobot_MCP9808_I2C mcp9808_other(&Machine_Ctrl.WireOne, 0x19);



  mcp9808.wakeUpMode();
  mcp9808.setResolution(RESOLUTION_0_0625);
  mcp9808_other.wakeUpMode();
  mcp9808_other.setResolution(RESOLUTION_0_0625);
  PostData_V2["Temperature"].set(mcp9808.getTemperature()) ;
  PostData_V2["Temperature_2"].set(mcp9808_other.getTemperature()) ;
  //TODO 溫度測試
  PostData_V2["Type"].set("X96");
  PostData_V2["SensorIndex"].set(index);
  PostData_V2["Name"].set(desp);
  PostData_V2["Resistance_Level"].set(type);
  time_t nowTime = now();
  char datetimeChar[30];
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  PostData_V2["DataTime"] = datetimeChar;
  String PostString = "";
  http.begin("http://192.168.20.27:5566/data");
  http.addHeader("Content-Type", "application/json");
  serializeJsonPretty(PostData_V2, PostString);
  http.POST(PostString);
  http.end();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
}