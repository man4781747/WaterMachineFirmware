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
#include <ArduinoJson.h>

// #include "../lib/LTR_329ALS_01/src/LTR_329ALS_01.h"

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
// #include <Adafruit_PWMServoDriver.h>
#include <HTTPClient.h>

uint16_t CH0_Buff [30];
uint16_t CH1_Buff [30];
DynamicJsonDocument PostData(10000);
ALS_01_Data_t test;
HTTPClient http;
String PostString;
time_t nowTime;
char datetimeChar[30];
/**
 * @brief 計算平均值
 * 
 * @param x 
 * @param len 
 * @return double 
 */
double average(const uint16_t* x, int len)
{
  uint32_t sum = 0;
  for (int i = 0; i < len; i++) {
    sum += x[i];
  }
  double average = static_cast<double>(sum) / len;
  return average;
}

/**
 * @brief 獲得方差
 * 
 * @param x 
 * @param len 
 * @return double 
 */
double variance(const uint16_t* x, int len)
{
  double sum = 0;
  double avg = average(x, len);
  for (int i = 0; i < len; i++) {
    double diff = static_cast<double>(x[i]) - avg;
    sum += diff * diff;
  }
  double variance = static_cast<double>(sum) / len;
  return variance;
}

/**
 * @brief 得到標準差
 * 
 * @param x 
 * @param len 
 * @return double 
 */
double standardDev(const uint16_t* x, int len)
{
  double var = variance(x, len);
  if (var == 0.) {
    return 0.;
  }
  return sqrt(var);
}

/**
 * @brief 獲得過濾後的平均數值
 * 
 * @param x 
 * @param len 
 * @return double 
 * 
 * @note 計算標準差時，有可能遇到標準差算出來為0的狀況，
 * 此時平均值就為答案
 */
double afterFilterValue(const uint16_t* x, int len)
{
  double standard = standardDev(x, len);
  double avg = average(x, len);

  if (standard == 0.) {
    return avg;
  }
  uint32_t sum = 0;
  double sumLen = 0.;

  for (int i = 0; i < len; i++) {
    if (abs((double)x[i]-avg) < standard*2) {
      sum += x[i];
      sumLen += 1;
    }
  }
  return static_cast<double>(sum)/sumLen;
}

void sensorTest(int Index);
long oneMinSave = 0;
long fiveMinSave = 0;
long tenMinSave = 0;
long TwntyFiveMinSave = 0;
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


  // digitalWrite(Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.stcpPin, LOW);
  // shiftOut(
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.dataPin, 
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.shcpPin, 
  //   LSBFIRST, 0b11110011
  // );
  // digitalWrite(Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.stcpPin, HIGH);

}

void loop() {
  
  // delay(1000);
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

  // if ( now()/(1*60) != oneMinSave) {
  //   oneMinSave = now()/(1*60);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
  //   sensorTest(0);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
  // }


  // if ( now()/(5*60) != fiveMinSave) {
  //   fiveMinSave = now()/(5*60);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(1);
  //   sensorTest(1);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
  // }

  // if ( now()/(10*60) != tenMinSave) {
  //   tenMinSave = now()/(10*60);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(2);
  //   sensorTest(2);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
  // }

  // if ( now()/(25*60) != TwntyFiveMinSave) {
  //   TwntyFiveMinSave = now()/(25*60);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(3);
  //   sensorTest(3);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
  // }

  // ArduinoOTA.handle();

}

void sensorTest(int Index){
  Machine_Ctrl.WireOne.beginTransmission(0x70);
  Machine_Ctrl.WireOne.write(1 << Index);
  Machine_Ctrl.WireOne.endTransmission();
  delay(10);
  PostData.clear();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_1X);
  for (int i=0;i<30;i++) {
    test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
    CH0_Buff[i] = test.CH_0;
    CH1_Buff[i] = test.CH_1;
  }
  PostData["CH0"] = afterFilterValue(CH0_Buff, 30);
  PostData["CH1"] = afterFilterValue(CH1_Buff, 30);
  // Serial.printf(" %.2f, %.2f\n",PostData["CH0"],PostData["CH1"]);

  nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  PostData["DataTime"] = datetimeChar;
  PostData["SensorIndex"] = String(Index + 1);
  PostData["Type"] = "1X";
  PostString = "";
  http.begin("http://192.168.20.27:5566/data");
  http.addHeader("Content-Type", "application/json");
  serializeJsonPretty(PostData, PostString);
  http.POST(PostString);
  http.end();
  ////
  PostData.clear();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_2X);
  for (int i=0;i<30;i++) {
    test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
    CH0_Buff[i] = (double)test.CH_0;
    CH1_Buff[i] = (double)test.CH_1;
  }
  PostData["CH0"] = afterFilterValue(CH0_Buff, 30);
  PostData["CH1"] = afterFilterValue(CH1_Buff, 30);
  Serial.printf("Index:1 - 2X: %.2f, %.2f\n",PostData["CH0"],PostData["CH1"]);
  nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  PostData["DataTime"] = datetimeChar;
  PostData["SensorIndex"] = String(Index + 1);
  PostData["Type"] = "2X";
  PostString = "";
  http.begin("http://192.168.20.27:5566/data");
  http.addHeader("Content-Type", "application/json");
  serializeJsonPretty(PostData, PostString);
  http.POST(PostString);
  http.end();
  ////
  PostData.clear();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_4X);
  for (int i=0;i<30;i++) {
    test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
    CH0_Buff[i] = (double)test.CH_0;
    CH1_Buff[i] = (double)test.CH_1;
  }
  PostData["CH0"] = afterFilterValue(CH0_Buff, 30);
  PostData["CH1"] = afterFilterValue(CH1_Buff, 30);
  Serial.printf("Index:1 - 4X: %.2f, %.2f\n",PostData["CH0"],PostData["CH1"]);
  nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  PostData["DataTime"] = datetimeChar;
  PostData["SensorIndex"] = String(Index + 1);
  PostData["Type"] = "4X";
  PostString = "";
  http.begin("http://192.168.20.27:5566/data");
  http.addHeader("Content-Type", "application/json");
  serializeJsonPretty(PostData, PostString);
  http.POST(PostString);
  http.end();
  ////
  PostData.clear();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_8X);
  for (int i=0;i<30;i++) {
    test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
    CH0_Buff[i] = (double)test.CH_0;
    CH1_Buff[i] = (double)test.CH_1;
  }
  PostData["CH0"] = afterFilterValue(CH0_Buff, 30);
  PostData["CH1"] = afterFilterValue(CH1_Buff, 30);
  Serial.printf("Index:1 - 8X: %.2f, %.2f\n",PostData["CH0"],PostData["CH1"]);
  nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  PostData["DataTime"] = datetimeChar;
  PostData["SensorIndex"] = String(Index + 1);
  PostData["Type"] = "8X";
  PostString = "";
  http.begin("http://192.168.20.27:5566/data");
  http.addHeader("Content-Type", "application/json");
  serializeJsonPretty(PostData, PostString);
  http.POST(PostString);
  http.end();
  ////
  PostData.clear();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_48X);
  for (int i=0;i<30;i++) {
    test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
    CH0_Buff[i] = (double)test.CH_0;
    CH1_Buff[i] = (double)test.CH_1;
  }
  PostData["CH0"] = afterFilterValue(CH0_Buff, 30);
  PostData["CH1"] = afterFilterValue(CH1_Buff, 30);
  Serial.printf("Index:1 - 48X: %.2f, %.2f\n",PostData["CH0"],PostData["CH1"]);
  nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  PostData["DataTime"] = datetimeChar;
  PostData["SensorIndex"] = String(Index + 1);
  PostData["Type"] = "48X";
  PostString = "";
  http.begin("http://192.168.20.27:5566/data");
  http.addHeader("Content-Type", "application/json");
  serializeJsonPretty(PostData, PostString);
  http.POST(PostString);
  http.end();
  ////
  PostData.clear();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  for (int i=0;i<30;i++) {
    test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
    CH0_Buff[i] = (double)test.CH_0;
    CH1_Buff[i] = (double)test.CH_1;
  }
  PostData["CH0"] = afterFilterValue(CH0_Buff, 30);
  PostData["CH1"] = afterFilterValue(CH1_Buff, 30);
  Serial.printf("Index:1 - 96X: %.2f, %.2f\n",PostData["CH0"],PostData["CH1"]);
  nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );
  PostData["DataTime"] = datetimeChar;
  PostData["SensorIndex"] = String(Index + 1);
  PostData["Type"] = "96X";
  PostString = "";
  http.begin("http://192.168.20.27:5566/data");
  http.addHeader("Content-Type", "application/json");
  serializeJsonPretty(PostData, PostString);
  http.POST(PostString);
  http.end();


}


