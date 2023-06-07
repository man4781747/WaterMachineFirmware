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

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <HTTPClient.h>

#include <Adafruit_AS7341.h>
Adafruit_AS7341 as7341;
void SEN0364Test();

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

int Sensor_1_PIN = 19;
int Sensor_2_PIN = 20;
int Sensor_3_PIN = 21;
int Sensor_4_PIN = 22;

#define DHTTYPE    DHT22
#define DHTPIN 2
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;
/////////////////////////

const char* LOG_TAG = "MAIN";
SMachine_Ctrl Machine_Ctrl;

const char* FIRMWARE_VERSION = "V2.23.52.0";




void setup() {
  Serial.begin(115200);
  Serial.println("START");

  Machine_Ctrl.INIT_SPIFFS_config();
  Machine_Ctrl.INIT_I2C_Wires();
  Machine_Ctrl.INIT_PoolData();
  Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();

  Machine_Ctrl.peristalticMotorsCtrl.INIT_Motors(42,41,40,2);

  Machine_Ctrl.motorCtrl.INIT_Motors(Machine_Ctrl.WireOne);
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
  // pinMode(Sensor_1_PIN, OUTPUT);
  // pinMode(Sensor_2_PIN, OUTPUT);
  // pinMode(Sensor_3_PIN, OUTPUT);
  // pinMode(Sensor_4_PIN, OUTPUT);

  // if (!as7341.begin((uint8_t)57U, &(Machine_Ctrl.WireOne))){
  //   Serial.println("Could not find AS7341");
  //   while (1) { delay(10); }
  // }
  // Serial.println("Setup Atime, ASTEP, Gain");
  // as7341.setATIME(100);
  // as7341.setASTEP(999);
  // as7341.setGain(AS7341_GAIN_256X);

  // dht.begin();

  // sensor_t sensor;
  // dht.temperature().getSensor(&sensor);
  // Serial.println(F("------------------------------------"));
  // Serial.println(F("Temperature Sensor"));
  // Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  // Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  // Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  // Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  // Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  // Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  // Serial.println(F("------------------------------------"));
  // // Print humidity sensor details.
  // dht.humidity().getSensor(&sensor);
  // Serial.println(F("Humidity Sensor"));
  // Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  // Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  // Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  // Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  // Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  // Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  // Serial.println(F("------------------------------------"));
  // delayMS = sensor.min_delay / 1000;
}

void loop() {
  // SEN0364Test();
  delay(1000);
  // delay(delayMS);
  // // Get temperature event and print its value.
  // sensors_event_t event;
  // dht.temperature().getEvent(&event);
  // if (isnan(event.temperature)) {
  //   Serial.println(F("Error reading temperature!"));
  // }
  // else {
  //   Serial.print(F("Temperature: "));
  //   Serial.print(event.temperature);
  //   Serial.println(F("°C"));
  // }
  // // Get humidity event and print its value.
  // dht.humidity().getEvent(&event);
  // if (isnan(event.relative_humidity)) {
  //   Serial.println(F("Error reading humidity!"));
  // }
  // else {
  //   Serial.print(F("Humidity: "));
  //   Serial.print(event.relative_humidity);
  //   Serial.println(F("%"));
  // }


  // digitalWrite(Sensor_1_PIN,HIGH);
  // digitalWrite(Sensor_2_PIN,LOW);
  // digitalWrite(Sensor_3_PIN,HIGH);
  // delay(1000);

  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("CH0: %d, CH1: %d\n", test.CH_0, test.CH_1);
  // delay(1000);
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 1);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(1);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("CH0: %d, CH1: %d\n", test.CH_0, test.CH_1);
 
  // sensorTest(0);
  // digitalWrite(Sensor_1_PIN,LOW);
  // digitalWrite(Sensor_2_PIN,LOW);
  // digitalWrite(Sensor_3_PIN,LOW);
  // delay(60*1000);
  // digitalWrite(Sensor_1_PIN,LOW);
  // digitalWrite(Sensor_2_PIN,HIGH);
  // digitalWrite(Sensor_3_PIN,HIGH);
  // delay(1000);
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 1);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("CH0: %d, CH1: %d\n", test.CH_0, test.CH_1);
  // sensorTest(1);
  // digitalWrite(Sensor_1_PIN,LOW);
  // digitalWrite(Sensor_2_PIN,LOW);
  // digitalWrite(Sensor_3_PIN,LOW);
  // digitalWrite(Sensor_1_PIN,HIGH);
  // digitalWrite(Sensor_2_PIN,LOW);
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // delay(10);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("CH0: %d, CH1: %d\n", test.CH_0, test.CH_1);
  // delay(1000);


  // digitalWrite(Sensor_3_PIN,LOW);
  // digitalWrite(Sensor_2_PIN,HIGH);
  // // digitalWrite(Sensor_3_PIN,LOW);
  // // digitalWrite(Sensor_4_PIN,LOW);
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 1);
  // Machine_Ctrl.WireOne.endTransmission();
  // delay(10);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("CH0: %d, CH1: %d\n", test.CH_0, test.CH_1);
  // delay(1000);

  // delay(1000);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // delay(10);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // for (int i=0;i<30;i++) {
  //   test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //   CH0_Buff[i] = test.CH_0;
  //   CH1_Buff[i] = test.CH_1;
  // }
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("CH0: %d, CH1: %d\n", test.CH_0, test.CH_1);




  // PostData["CH0"] = afterFilterValue(CH0_Buff, 30);
  // PostData["CH1"] = afterFilterValue(CH1_Buff, 30);


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

  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(1);
  //   sensorTest(1);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();

  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(2);
  //   sensorTest(2);
  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();

  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(3);
  //   sensorTest(3);
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
  sensors_event_t event;

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
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    PostData["Temperature"].set(-1) ;
  }
  else {
    PostData["Temperature"].set((float)event.temperature) ;
  }
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
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    PostData["Temperature"].set(-1) ;
  }
  else {
    PostData["Temperature"].set((float)event.temperature) ;
  }
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
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    PostData["Temperature"].set(-1) ;
  }
  else {
    PostData["Temperature"].set((float)event.temperature) ;
  }
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
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    PostData["Temperature"].set(-1) ;
  }
  else {
    PostData["Temperature"].set((float)event.temperature) ;
  }
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
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    PostData["Temperature"].set(-1) ;
  }
  else {
    PostData["Temperature"].set((float)event.temperature) ;
  }
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
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    PostData["Temperature"].set(-1) ;
  }
  else {
    PostData["Temperature"].set((float)event.temperature) ;
  }
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


void SEN0364Test(){
  uint16_t readings[12];

  if (!as7341.readAllChannels(readings)){
    Serial.println("Error reading all channels!");
    return;
  }

  Serial.print("ADC0/F1 415nm : ");
  Serial.println(readings[0]);
  Serial.print("ADC1/F2 445nm : ");
  Serial.println(readings[1]);
  Serial.print("ADC2/F3 480nm : ");
  Serial.println(readings[2]);
  Serial.print("ADC3/F4 515nm : ");
  Serial.println(readings[3]);
  Serial.print("ADC0/F5 555nm : ");

  /* 
  // we skip the first set of duplicate clear/NIR readings
  Serial.print("ADC4/Clear-");
  Serial.println(readings[4]);
  Serial.print("ADC5/NIR-");
  Serial.println(readings[5]);
  */
  
  Serial.println(readings[6]);
  Serial.print("ADC1/F6 590nm : ");
  Serial.println(readings[7]);
  Serial.print("ADC2/F7 630nm : ");
  Serial.println(readings[8]);
  Serial.print("ADC3/F8 680nm : ");
  Serial.println(readings[9]);
  Serial.print("ADC4/Clear    : ");
  Serial.println(readings[10]);
  Serial.print("ADC5/NIR      : ");
  Serial.println(readings[11]);

  Serial.println();
}
