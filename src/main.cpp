// 如果要更改 log level，請去這邊找
// #include <sdkconfig.h>
// #undef CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL
// #define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL 5
// #undef ARDUHAL_LOG_LEVEL
// #define ARDUHAL_LOG_LEVEL 5

// ArduinoJson.h
// #define JSON_MAX_DEPTH 40


#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>
#define ARDUINOJSON_DEFAULT_NESTING_LIMIT 40
#include <ArduinoJson.h>

#include "../lib/LTR_329ALS_01/src/LTR_329ALS_01.h"

#include "Machine_Ctrl/src/Machine_Ctrl.h"

/////////////////
// #include <Ethernet.h>
// #include <Ethernet2.h>
// EthernetServer server(80);
// byte ethernet_mac[] = {
//   0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
// };
// IPAddress ethernet_ip(192, 168, 20, 222);
// IPAddress gateway(192, 168, 20, 1);//宜蘭一場前場
// IPAddress subnet(255, 255, 255, 0); //
// EthernetClient client;
// EthernetServer server(80);
#include <Adafruit_PWMServoDriver.h>
/////////////////////////

const char* LOG_TAG = "MAIN";
SMachine_Ctrl Machine_Ctrl;

const char* FIRMWARE_VERSION = "V2.23.52.0";




void setup() {
  Serial.begin(115200);
  Serial.println("START");

  Machine_Ctrl.INIT_SPIFFS_config();
  Machine_Ctrl.INIT_I2C_Wires();
  Machine_Ctrl.motorCtrl.INIT_Motors(Machine_Ctrl.WireOne);
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
  Machine_Ctrl.peristalticMotorsCtrl.INIT_Motors(42,41,40,2);
  Machine_Ctrl.BackendServer.ConnectToWifi();
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.BackendServer.ServerStart();

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
  // ALS_01_Data_t testValue = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println("==== 1 ====");
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_2X);
  // testValue = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_4X);
  // testValue = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_8X);
  // testValue = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_48X);
  // testValue = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // testValue = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();




  // Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();


  // for (int i=0;i<100;i++) {
  //   Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(2, PeristalticMotorStatus::FORWARD);
  //   Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(7, PeristalticMotorStatus::REVERSR);
  //   Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
  //   delay(50);
  //   Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
  // }

  // Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
  // Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(2, PeristalticMotorStatus::REVERSR);
  // Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(7, PeristalticMotorStatus::FORWARD);
  // Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
  // delay(2000);
  // Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();

  // Ethernet.init(9);
  // Ethernet.begin(ethernet_mac, ethernet_ip);
  // Serial.println(Ethernet.hardwareStatus());
  // if (Ethernet.hardwareStatus() == EthernetNoHardware) {
  //   Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
  //   while (true) {
  //     delay(1); // do nothing, no point running without Ethernet hardware
  //   }
  // }
  // if (Ethernet.linkStatus() == LinkOFF) {
  //   Serial.println("Ethernet cable is not connected.");
  // }
  // Serial.println("------------------");
  // Serial.println(Ethernet.localIP());
  // Serial.println("------------------");
}

void loop() {
  // byte error, address;
  // int devices = 0;
  // for (address = 1; address < 127; address++) {
  //   // Serial.printf("test %d\n",address);
  //   Machine_Ctrl.WireOne.beginTransmission(address);
  //   error = Machine_Ctrl.WireOne.endTransmission();
  //   if (error == 0) {
  //     Serial.print("I2C device found at address 0x");
  //     if (address < 16) {
  //       Serial.print("0");
  //     }
  //     Serial.println(address, HEX);

  //     devices++;
  //   }
  // }
  // if (devices == 0) {
  //   Serial.println("No I2C devices found");
  // }
  // delay(1000);
  // if (client.connect("192.168.20.27", 5566)) {
  //   Serial.println("connected");
  //   client.println("GET / HTTP/1.1");//模擬池
  //   client.println("Host: 192.168.20.222:80");//模擬池
  //   client.println("Connection: close");//原版
  //   client.println();
  // } else {
  //   Serial.println("Connection failed");
  // }

  // while (client.connected()) {
  //   if (client.available()) {
  //     char c = client.read();
  //     Serial.print(c);
  //   }
  // }
  // client.stop();


  // delay(1000);

  // EthernetClient client = server.available();
  // if (client) {
  //   Serial.println("New client connected");
  //   while (client.connected()) {
  //     if (client.available()) {
  //       char c = client.read();
  //       Serial.print(c);
  //     }
  //   }
  //   client.stop();
  //   Serial.println("Client disconnected");
  // }



  // digitalWrite(STCP, LOW);
  // shiftOut(DATA, SHCP, LSBFIRST, 0b11000000);
  // digitalWrite(STCP, HIGH);
  // delay(1000);
  // Machine_Ctrl.LTR_329ALS_01_Ctrler.ALS_Contr_Config.ALS_Gain = ALS_Gain::Gain_48X;
  // ALS_01_Data_t testValue = Machine_Ctrl.LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println("==== 1 ====");
  // Serial.println(testValue.CH_0);
  // Serial.println(testValue.CH_1);
  // digitalWrite(STCP, LOW);
  // shiftOut(DATA, SHCP, LSBFIRST, 0b00000000);
  // digitalWrite(STCP, HIGH);
  delay(1000);
  // digitalWrite(STCP, LOW);
  // shiftOut(DATA, SHCP, LSBFIRST, 0b00110000);
  // digitalWrite(STCP, HIGH);
  // delay(3000);
  // Machine_Ctrl.LTR_329ALS_01_Ctrler.ALS_Contr_Config.ALS_Gain = ALS_Gain::Gain_96X;
  // ALS_01_Data_t testValue_2 = Machine_Ctrl.LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.println("==== 2 ====");
  // Serial.println(testValue_2.CH_0);
  // Serial.println(testValue_2.CH_1);




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

