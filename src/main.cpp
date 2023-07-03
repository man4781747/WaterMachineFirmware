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

#include "CalcFunction.h"

#include "Machine_Ctrl/src/Machine_Ctrl.h"

//TODO oled暫時這樣寫死
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//TODO oled暫時這樣寫死

#include <SD.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <HTTPClient.h>

// #include "Adafruit_MCP9808.h"
// Adafruit_MCP9808 tempsensor = Adafruit_MCP9808();
#include "DFRobot_MCP9808.h"
DFRobot_MCP9808_I2C mcp9808(&Machine_Ctrl.WireOne, 0x18);
DFRobot_MCP9808_I2C mcp9808_other(&Machine_Ctrl.WireOne, 0x19);

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


const char* LOG_TAG = "MAIN";
SMachine_Ctrl Machine_Ctrl;

const char* FIRMWARE_VERSION = "V2.23.71.0";

//TODO oled暫時這樣寫死

Adafruit_SSD1306 display(128, 64, &Machine_Ctrl.WireOne, -1);
// Adafruit_SH1106 display(Machine_Ctrl.WireOne_SDA, Machine_Ctrl.WireOne_SCL);
//TODO oled暫時這樣寫死

void scanI2C();

void setup() {
  pinMode(39, PULLUP);
  
  Serial.begin(115200);
  Serial.println("START");
  // Serial.printf("Total heap: %d\n", ESP.getHeapSize());
  // Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  // Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
  // Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());
  Machine_Ctrl.INIT_SPIFFS_config();
  Machine_Ctrl.INIT_I2C_Wires();
  Machine_Ctrl.INIT_PoolData();
  
  Machine_Ctrl.peristalticMotorsCtrl.INIT_Motors(42,41,40,2);

  Machine_Ctrl.motorCtrl.INIT_Motors(Machine_Ctrl.WireOne);

  Machine_Ctrl.StopDeviceAndINIT();
  Machine_Ctrl.INIT_SD_Card();

  Machine_Ctrl.BackendServer.ConnectToWifi();
  Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
  Machine_Ctrl.BackendServer.ServerStart();

  //TODO oled暫時這樣寫死
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setRotation(0);
  display.setTextColor(WHITE);  
  display.setCursor(0, 0);
  display.printf("%s",Machine_Ctrl.BackendServer.IP.c_str());
  display.setCursor(0, 16);
  display.printf("Ver: %s",FIRMWARE_VERSION);
  display.display();
  //TODO oled暫時這樣寫死

  // if (!SD.begin(8)) {
  //   Serial.println("initialization failed!");
  //   while (1);
  // }
  // Serial.println("initialization done.");


  //TODO oled暫時這樣寫死

  // mcp9808.begin();
  // mcp9808_other.begin();
  // tempsensor.begin(0x18);
  // tempsensor.setResolution(3);

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
  // pinMode(14, INPUT);
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
}

void loop() {
  // display.clearDisplay();
  // display.setCursor(0, 0);
  // display.printf("%s",Machine_Ctrl.BackendServer.IP.c_str());
  // display.setCursor(0, 16);
  // display.printf("Ver: %s",FIRMWARE_VERSION);
  // // display.setCursor(0, 24);
  // // display.printf("%s",Machine_Ctrl.GetDatetimeString().c_str());

  // display.display();


  // Serial.printf("%d", analogRead(14));
  // SEN0364Test();
  // Machine_Ctrl.peristalticMotorsCtrl.ShowNowSetting();
  // Machine_Ctrl.peristalticMotorsCtrl.OpenAllPin();
  // delay(1000);
  // Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
  // delay(1000);
  // Serial.println(0);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);

  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
  // delay(100);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_1X);
  // ALS_01_Data_t test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_2X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_4X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_8X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_48X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  // //TODO 溫度測試
  // mcp9808.wakeUpMode();
  // mcp9808.setResolution(RESOLUTION_0_0625);
  // Serial.printf("%.2f\n", mcp9808.getTemperature());
  // //TODO 溫度測試


  // Machine_Ctrl.peristalticMotorsCtrl.INIT_Motors(42,41,40,2);.

  // digitalWrite(41, LOW);
  // shiftOut(40, 42, MSBFIRST, 0b10101010);
  // shiftOut(40, 42, MSBFIRST, 0b10101010);
  // digitalWrite(41, HIGH);
  // delay(2000);
  // digitalWrite(41, LOW);
  // shiftOut(40, 42, MSBFIRST, 0b01010101);
  // shiftOut(40, 42, MSBFIRST, 0b01010101);
  // digitalWrite(41, HIGH);
  // delay(1000);
  // digitalWrite(41, LOW);
  // shiftOut(40, 42, MSBFIRST, 0b11111111);
  // shiftOut(40, 42, MSBFIRST, 0b11111111);
  // shiftOut(40, 42, MSBFIRST, 0b11111111);
  // shiftOut(40, 42, MSBFIRST, 0b11111111);
  // digitalWrite(41, HIGH);
  // pinMode(15, INPUT);
  // pinMode(7, OUTPUT);
  // digitalWrite(7, HIGH);
  delay(1000);
  // Serial.println(analogRead(15));
  // digitalWrite(7, LOW);

  // scanI2C();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
  // delay(1000);

  // // pinMode(15, INPUT);
  // // Serial.println(analogRead(15));
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
  // delay(1000);
  // for (int i=194;i<202;i++) {
  //   Machine_Ctrl.WireOne.beginTransmission(0x2F);
  //   Machine_Ctrl.WireOne.write(0b00000000);
  //   Machine_Ctrl.WireOne.write(i);
  //   Machine_Ctrl.WireOne.endTransmission();
  //   Serial.printf("%d,", i);
  //   // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_1X);
  //   // ALS_01_Data_t test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //   // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  //   // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_2X);
  //   // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //   // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  //   // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_4X);
  //   // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //   // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  //   // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_8X);
  //   // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //   // Serial.printf("%d,%d,", test.CH_0, test.CH_1);

  //   Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_48X);

  //   DynamicJsonDocument PostData_V2(10000);
  //   uint16_t CH0_Buff_V2 [30];
  //   uint16_t CH1_Buff_V2 [30];


  //   for (int i=0;i<30;i++) {
  //     test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  //     CH0_Buff_V2[i] = test.CH_0;
  //     CH1_Buff_V2[i] = test.CH_1;
  //   }
  //   PostData_V2["CH0"] = afterFilterValue(CH0_Buff_V2, 30);
  //   PostData_V2["CH1"] = afterFilterValue(CH1_Buff_V2, 30);

  //   Serial.printf("%.2f,%.2f,", PostData_V2["CH0"].as<double>(), PostData_V2["CH1"].as<double>());

  //   //TODO 溫度測試
  //   mcp9808.wakeUpMode();
  //   mcp9808.setResolution(RESOLUTION_0_0625);
  //   mcp9808_other.wakeUpMode();
  //   mcp9808_other.setResolution(RESOLUTION_0_0625);
  //   PostData_V2["Temperature"].set(mcp9808.getTemperature()) ;
  //   PostData_V2["Temperature_2"].set(mcp9808_other.getTemperature()) ;
  //   Serial.printf("%.2f,%.2f\n", PostData_V2["Temperature"].as<float>(), PostData_V2["Temperature_2"].as<float>());
  //   //TODO 溫度測試
  //   PostData_V2["Type"].set("X48");
  //   PostData_V2["SensorIndex"].set(0);
  //   PostData_V2["Name"].set("藍光測試");
  //   PostData_V2["Resistance_Level"].set(i);

  //   nowTime = now();
  //   sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
  //     year(nowTime), month(nowTime), day(nowTime),
  //     hour(nowTime), minute(nowTime), second(nowTime)
  //   );
  //   PostData_V2["DataTime"] = datetimeChar;
  //   PostString = "";
  //   http.begin("http://192.168.20.27:5566/data");
  //   http.addHeader("Content-Type", "application/json");
  //   serializeJsonPretty(PostData_V2, PostString);
  //   http.POST(PostString);
  //   http.end();

  //   delay(10);
  // }
  
  // Machine_Ctrl.WireOne.beginTransmission(0x2F);
  // Machine_Ctrl.WireOne.write(0b00000000);
  // Machine_Ctrl.WireOne.write(0b11111111);
  // Machine_Ctrl.WireOne.endTransmission();

  // Machine_Ctrl.WireOne.beginTransmission(0x2F);
  // Machine_Ctrl.WireOne.write(0b00001100);
  // Machine_Ctrl.WireOne.endTransmission();

  // Machine_Ctrl.WireOne.requestFrom(0x2F, 2);
  // uint8_t high_byte = Machine_Ctrl.WireOne.read();
  // uint8_t low_byte = Machine_Ctrl.WireOne.read();
  // for (int i = 7; i >= 0; i--) {
  //   Serial.print((high_byte >> i) & 1, BIN);
  // }
  // Serial.print(",");
  // for (int i = 7; i >= 0; i--) {
  //   Serial.print((low_byte >> i) & 1, BIN);
  // }
  // Serial.println();
  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 0);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(0);
  // delay(100);

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d\n", test.CH_0, test.CH_1);
  //TODO 溫度測試
  // if (!mcp9808.wakeUpMode()) {
  //   Serial.println("No");
  // } else {
  //   Serial.println("OK");
  // }

  // if (!mcp9808.setResolution(RESOLUTION_0_0625)) {
  //   Serial.println("OPPPS");
  // } else {
  //   Serial.println("YA");
  // }

  // Serial.println(mcp9808.getTemperature());
  //TODO 溫度測試

  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 1);
  // Machine_Ctrl.WireOne.endTransmission();
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(1);
  // delay(100);

  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("%d,%d\n", test.CH_0, test.CH_1);
  // delay(2000);
  // //TODO 溫度測試
  // if (!mcp9808.wakeUpMode()) {
  //   Serial.println("No");
  // } else {
  //   Serial.println("OK");
  // }

  // if (!mcp9808.setResolution(RESOLUTION_0_0625)) {
  //   Serial.println("OPPPS");
  // } else {
  //   Serial.println("YA");
  // }

  // Serial.println(mcp9808.getTemperature());
  // //TODO 溫度測試

  // Machine_Ctrl.WireOne.beginTransmission(0x70);
  // Machine_Ctrl.WireOne.write(1 << 1);
  // Machine_Ctrl.WireOne.endTransmission();
  // delay(100);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
  // test = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
  // Serial.printf("CH0: %d, CH1: %d\n", test.CH_0, test.CH_1);
  // delay(1000);
  // Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();

  // delay(1000*60);
  // Machine_Ctrl.UpdateAllPoolsDataRandom();
  // Machine_Ctrl.BroadcastNewPoolData("pool-1");
  // Machine_Ctrl.BroadcastNewPoolData("pool-2");
  // Machine_Ctrl.BroadcastNewPoolData("pool-3");
  // Machine_Ctrl.BroadcastNewPoolData("pool-4");
  // delay(1000*60*60*4 - 60*1000);
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



void TestSensor() {

}