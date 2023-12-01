#include "Machine_Ctrl.h"
#include <esp_log.h>
#include "esp_random.h"

#include <String.h>
#include "CalcFunction.h"
#include "StorgeSystemExternalFunction.h"
#include <SD.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <U8g2lib.h>
#include <INA226.h>
#include "lx_20s.h"
// #ifdef U8X8_HAVE_HW_I2C
// #ifdef WIRE_INTERFACES_COUNT
// #if WIRE_INTERFACES_COUNT > 1
// #define U8X8_HAVE_2ND_HW_I2C
// #endif
// #endif
// #endif /* U8X8_HAVE_HW_I2C */
#include <Adafruit_GFX.h>
#include "Adafruit_SH1106.h"
#include "../lib/QRCode/src/qrcode.h"

#include <ESPAsyncWebServer.h>
// #include <AsyncWebServer_ESP32_W5500.h>
#include "AsyncTCP.h"

#include <TimeLib.h>   

#include <Pools_Ctrl.h>
#include <Motor_Ctrl.h>
#include <vector>
#include <variant>
#include <functional>
#include <random>
#include <map>
#include <ctime>

#include "UUID.h"
UUID uuid;

TaskHandle_t TASK_SwitchMotorScan = NULL;
TaskHandle_t TASK_PeristalticMotorScan = NULL;

SemaphoreHandle_t MUTEX_Peristaltic_MOTOR = xSemaphoreCreateBinary();

// U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, Machine_Ctrl.WireOne_SCL, Machine_Ctrl.WireOne_SDA);
// U8G2_SSD1306_128X64_NONAME_1_2ND_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
// extern AsyncWebSocket ws;

String CreateUUID()
{
  char uuidStr[37];
  esp_fill_random(uuidStr, sizeof(uuidStr) - 1);
  uuidStr[8] = '-';
  uuidStr[13] = '-';
  uuidStr[18] = '-';
  uuidStr[23] = '-';
  uuidStr[36] = '\0';
  Serial.println(uuidStr);
  String reaultString = String(uuidStr);
  return reaultString;
}

////////////////////////////////////////////////////
// For 初始化
////////////////////////////////////////////////////

static int callback(void *data, int argc, char **argv, char **azColName){
  int i;
  JsonDocument *targetJsonObj = (JsonDocument*)data;
  if (targetJsonObj==NULL) {
    return 0;
  }
  DynamicJsonDocument oneLogData(2000);
  for (i = 0; i<argc; i++){
    String KeyName = String(azColName[i]);
    String Value = String(argv[i] ? argv[i] : "NULL");
    oneLogData[KeyName].set(Value);
  }
  (*targetJsonObj).add(oneLogData);
  return 0;
}

char *zErrMsg = 0;
int SMachine_Ctrl::db_exec(sqlite3 *db, String sql, JsonDocument *jsonData=NULL) {
  xSemaphoreTake(SQL_xMutex, portMAX_DELAY);
  if (jsonData!=NULL) {
    (*jsonData).clear();
  }
  Serial.println(sql);
  long start = micros();
  int rc = sqlite3_exec(db, sql.c_str(), callback, (void*)jsonData, &zErrMsg);
  if (rc != SQLITE_OK) {
    Serial.printf("SQL error: %s\n", zErrMsg);
    sqlite3_free(zErrMsg);
  } else {
    Serial.printf("Operation done successfully\n");
  }
  Serial.print(F("Time taken:"));
  Serial.println(micros()-start);
  // if (jsonData!=NULL) {
  //   serializeJsonPretty(*jsonData, Serial);
  // }
  xSemaphoreGive(SQL_xMutex);
  return rc;
}

double getRandomNumber(double mean, double stddev) {

  // std::random_device rd;  // 随机数种子
  // std::mt19937 gen(rd()); // 随机数引擎
  // std::normal_distribution<double> dist(mean, stddev);
  // double sample = dist(gen); 
  return mean + (double)((esp_random() % 1000)/1000.);
}
//! SPIFFS 相關

/**
 * @brief SPIFFS 初始化
 * 同時會讀取基礎資訊&參數設定檔
 * 
 */
void SMachine_Ctrl::INIT_SPIFFS_And_LoadConfigs()
{
  INIT_SPIFFS();
  SPIFFS__LoadDeviceBaseInfo();
  SPIFFS__LoadWiFiConfig();
}

bool SMachine_Ctrl::INIT_SPIFFS()
{
  if(!SPIFFS.begin(true)){
    ESP_LOGE("", "讀取SPIFFS失敗");
    return false;
  }
  ESP_LOGD("", "讀取SPIFFS成功");
  return true;
}

bool SMachine_Ctrl::SPIFFS__LoadDeviceBaseInfo()
{
  //STEP 首先判斷設定檔是否存在，如果不在就建立一個
  if (!SPIFFS.exists(FilePath__SPIFFS__DeviceBaseInfo)) {
    ESP_LOGW("", "找不到儀器基礎設定檔: %s，重建他!", FilePath__SPIFFS__DeviceBaseInfo.c_str());
    ExFile_CreateFile(SPIFFS, FilePath__SPIFFS__DeviceBaseInfo);
    (*JSON__DeviceBaseInfo)["device_no"].set("xxxxxx");
    (*JSON__DeviceBaseInfo)["mode"].set("Mode_Slave");
    return ExFile_WriteJsonFile(SPIFFS, FilePath__SPIFFS__DeviceBaseInfo, *JSON__DeviceBaseInfo);
  }
  else {
    return ExFile_LoadJsonFile(SPIFFS, FilePath__SPIFFS__DeviceBaseInfo, *JSON__DeviceBaseInfo);
  }
}

bool SMachine_Ctrl::SPIFFS__ReWriteDeviceBaseInfo()
{
  return ExFile_WriteJsonFile(SPIFFS, FilePath__SPIFFS__DeviceBaseInfo, *JSON__DeviceBaseInfo);
}

bool SMachine_Ctrl::SPIFFS__LoadWiFiConfig()
{
  //STEP 首先判斷設定檔是否存在，如果不在就建立一個
  if (!SPIFFS.exists(FilePath__SPIFFS__WiFiConfig)) {
    ESP_LOGW("", "找不到儀器基礎設定檔: %s，重建他!", FilePath__SPIFFS__WiFiConfig.c_str());
    ExFile_CreateFile(SPIFFS, FilePath__SPIFFS__WiFiConfig);
    (*JSON__WifiConfig)["AP_IP"].set("127.0.0.1");
    (*JSON__WifiConfig)["AP_gateway"].set("127.0.0.1");
    (*JSON__WifiConfig)["AP_subnet_mask"].set("255.255.255.0");
    (*JSON__WifiConfig)["AP_Name"].set("AkiraTest");
    (*JSON__WifiConfig)["AP_Password"].set("12345678");
    (*JSON__WifiConfig)["remote_IP"].set("127.0.0.1");
    (*JSON__WifiConfig)["remote_Name"].set("xxxxx");
    (*JSON__WifiConfig)["remote_Password"].set("12345678");
    return ExFile_WriteJsonFile(SPIFFS, FilePath__SPIFFS__WiFiConfig, *JSON__WifiConfig);
  }
  else {
    return ExFile_LoadJsonFile(SPIFFS, FilePath__SPIFFS__WiFiConfig, *JSON__WifiConfig);
  }
}

bool SMachine_Ctrl::SPIFFS__ReWriteWiFiConfig()
{
  return ExFile_WriteJsonFile(SPIFFS, FilePath__SPIFFS__WiFiConfig, *JSON__WifiConfig);
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! SQL相關
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

int SMachine_Ctrl::openLogDB()
{
  // SD.remove("/logDB.db");

  sqlite3_initialize();
  int rc = sqlite3_open(FilePath__SD__LogDB.c_str(), &DB_Log);
   if (rc) {
      Serial.printf("Can't open database: %s\n", sqlite3_errmsg(DB_Log));
      return rc;
   } else {
      Serial.printf("Opened database successfully\n");
      db_exec(DB_Log, "CREATE TABLE logs ( time TEXT, title TEXT, desp TEXT, level INTEGER );");
   }
   return rc;
}

int SMachine_Ctrl::openSensorDB()
{
  // SD.remove("/sensorDB.db");
  // sqlite3_initialize();
  int rc = sqlite3_open(FilePath__SD__SensorDB.c_str(), &DB_Sensor);
  if (rc) {
    Serial.printf("Can't open database: %s\n", sqlite3_errmsg(DB_Sensor));
    return rc;
  } else {
    Serial.printf("Opened database successfully\n");
    db_exec(DB_Sensor, "CREATE TABLE sensor ( time TEXT, title TEXT, pool TEXT , value_name TEXT , result REAL );");
    // db_exec(DB_Sensor, "CREATE TABLE sensor ( time TEXT, title TEXT, pool TEXT , value_name TEXT , result REAL, uuid TEXT );");
  }
  return rc;
}

void SMachine_Ctrl::InsertNewDataToDB(String time,String title, String pool, String ValueName, double result, String uuid)
{
  // String SqlString = "INSERT INTO sensor ( time, title, pool, value_name, result, uuid ) VALUES ( '";
  // SqlString += time;
  // SqlString += "' ,'";
  // SqlString += title;
  // SqlString += "' ,'";
  // SqlString += pool;
  // SqlString += "' ,'";
  // SqlString += ValueName;
  // SqlString += "' ,";
  // SqlString += String(result,2).toDouble();
  // SqlString += " ,'";
  // SqlString += Machine_Ctrl.TaskUUID;
  // SqlString += "' );";
  // db_exec(DB_Sensor, SqlString);

  String SqlString = "INSERT INTO sensor ( time, title, pool, value_name, result ) VALUES ( '";
  SqlString += time;
  SqlString += "' ,'";
  SqlString += title;
  SqlString += "' ,'";
  SqlString += pool;
  SqlString += "' ,'";
  SqlString += ValueName;
  SqlString += "' ,";
  SqlString += String(result,2).toDouble();
  SqlString += " );";
  db_exec(DB_Sensor, SqlString);

}

//! SD卡 相關

void SMachine_Ctrl::INIT_SD_And_LoadConfigs()
{
  INIT_SD_Card();
  ESP_LOGD("", "準備讀取儀器基礎設定檔");
  SD__DeviceConfig();
  ESP_LOGD("", "準備讀取光度感測器的設定檔");
  SD__LoadspectrophotometerConfig();
  ESP_LOGD("", "準備讀取PH感測器的設定檔");
  SD__LoadPHmeterConfig();
  ESP_LOGD("", "準備讀取蝦池的設定檔");
  SD__LoadPoolConfig();
  ESP_LOGD("", "準備更新當前SD卡內有的Pipeline檔案列表");
  SD__UpdatePipelineConfigList();
  ESP_LOGD("", "準備讀取排程的設定檔");
  SD__LoadScheduleConfig();
  ESP_LOGD("", "準備讀取伺服馬達的設定檔");
  SD__LoadPWNMotorConfig();
  ESP_LOGD("", "準備讀取蠕動馬達的設定檔");
  SD__LoadPeristalticMotorConfig();

}

bool SMachine_Ctrl::INIT_SD_Card()
{
  if (!SD.begin(8)) {
    ESP_LOGE("", "SD卡讀取失敗");
    vTaskDelay(1000/portTICK_PERIOD_MS);
    ESP.restart();
    return false;
  } else {
    ESP_LOGD("", "SD卡讀取成功");
    return true;
  }
}

void SMachine_Ctrl::SD__DeviceConfig()
{
  ExFile_LoadJsonFile(SD, FilePath__SD__DeviceConfig, *JSON__DeviceConfig);
}

void SMachine_Ctrl::SD__LoadspectrophotometerConfig()
{
  ExFile_LoadJsonFile(SD, FilePath__SD__SpectrophotometerConfig, *JSON__SpectrophotometerConfig);
}

void SMachine_Ctrl::SD__LoadPHmeterConfig()
{
  ExFile_LoadJsonFile(SD, FilePath__SD__PHmeterConfig, *JSON__PHmeterConfig);
}

void SMachine_Ctrl::SD__LoadPoolConfig()
{
  ExFile_LoadJsonFile(SD, FilePath__SD__PoolConfig, *JSON__PoolConfig);
}

void SMachine_Ctrl::SD__LoadPWNMotorConfig()
{
  ExFile_LoadJsonFile(SD, FilePath__SD__PWNMotorConfig, *JSON__PWNMotorConfig);
}

void SMachine_Ctrl::SD__LoadPeristalticMotorConfig()
{
  ExFile_LoadJsonFile(SD, FilePath__SD__PeristalticMotorConfig, *JSON__PeristalticMotorConfig);
}



void SMachine_Ctrl::SD__LoadScheduleConfig()
{
  ExFile_LoadJsonFile(SD, FilePath__SD__ScheduleConfig, *JSON__ScheduleConfig);
}

void SMachine_Ctrl::SD__ReWriteScheduleConfig()
{
  ExFile_WriteJsonFile(SD, FilePath__SD__ScheduleConfig, *JSON__ScheduleConfig);
}

void SMachine_Ctrl::SD__UpdatePipelineConfigList()
{
  (*JSON__PipelineConfigList).clear();
  File folder = SD.open("/pipelines");
  while (true) {
    File Entry = folder.openNextFile();
    if (! Entry) {
      break;
    }
    String FileName = String(Entry.name());
    if (FileName == "__temp__.json") {
      continue;
    }
    ESP_LOGD("", "準備讀取設定檔: %s", FileName.c_str());
    DynamicJsonDocument fileInfo(500);
    fileInfo["size"].set(Entry.size());
    fileInfo["name"].set(FileName);
    fileInfo["getLastWrite"].set(Entry.getLastWrite());
    DynamicJsonDocument fileContent(120000);
    DeserializationError error = deserializeJson(fileContent, Entry);
    Entry.close();
    ESP_LOGD("", "設定檔讀取完畢: %s", FileName.c_str());
    fileInfo["title"].set(fileContent["title"].as<String>());
    fileInfo["desp"].set(fileContent["desp"].as<String>());
    fileInfo["tag"].set(fileContent["tag"].as<JsonArray>());
    // if (error) {

    // } else {
    //   fileInfo["title"].set(fileContent["title"].as<String>());
    //   fileInfo["desp"].set(fileContent["desp"].as<String>());
    //   fileInfo["tag"].set(fileContent["tag"].as<JsonArray>());
    // }
    (*JSON__PipelineConfigList).add(fileInfo);
  }
}

void SMachine_Ctrl::SD__LoadOldLogs()
{
  db_exec(DB_Log, "SELECT * FROM logs DESC LIMIT 100", JSON__DeviceLogSave);
  // std::vector<String> logFileNameList;
  // File logsFolder = SD.open("/logs");
  // while (true) {
  //   File entry =  logsFolder.openNextFile();
  //   if (! entry) {
  //     break;
  //   }
  //   logFileNameList.push_back(String(entry.name()));
  //   entry.close();
  //   if (logFileNameList.size() > 100) {
  //     logFileNameList.erase(logFileNameList.begin());
  //   }
  // }

  // std::vector<String> logContentList;
  // for (int i=logFileNameList.size()-1;i>-1;i--) {
  //   if (logContentList.size() > 100) {
  //     break;
  //   }
  //   File logFileChose = SD.open("/logs/"+logFileNameList[i]);
  //   std::vector<String> singleLogContentList;
  //   String line = logFileChose.readStringUntil('\n');
  //   while (line.length() > 0) {
  //     singleLogContentList.push_back(line);
  //     line = logFileChose.readStringUntil('\n');
  //   }
  //   logFileChose.close();
    
  //   for (int singleLineIndex=singleLogContentList.size()-1;singleLineIndex>0;singleLineIndex--) {
  //     logContentList.push_back(singleLogContentList[singleLineIndex]);
  //     if (logContentList.size() > 100) {
  //       break;
  //     }
  //   }
  // }
  // for (int lineIndex=logContentList.size()-1;lineIndex>0;lineIndex--) {
  //   int delimiterIndex = 0;
  //   String lineChose = logContentList[lineIndex];
  //   int itemIndex = 0;
  //   int level;
  //   String title,time, desp="";
  //   while (lineChose.length() > 0) {
  //     delimiterIndex = lineChose.indexOf(",");
  //     if (itemIndex == 0) {
  //       level = lineChose.substring(0, delimiterIndex).toInt();
  //     }
  //     else if (itemIndex == 1) {
  //       time = lineChose.substring(0, delimiterIndex);
  //     }
  //     else if (itemIndex == 2) {
  //       title = lineChose.substring(0, delimiterIndex);
  //     }
  //     else if (itemIndex == 3) {
  //       desp = lineChose.substring(0, delimiterIndex);
  //     }
  //     if (delimiterIndex == -1) {
  //       break;
  //     }
  //     lineChose = lineChose.substring(delimiterIndex + 1);
  //     itemIndex++;
  //   }
  //   DynamicJsonDocument logItem(3000);
  //   logItem["level"].set(level);
  //   logItem["time"].set(time);
  //   logItem["title"].set(title);
  //   logItem["desp"].set(desp);
  //   // serializeJson(logItem, Serial);
  //   (*JSON__DeviceLogSave).add(logItem);
  //   if ((*JSON__DeviceLogSave).size() > 100) {
  //     break;
  //   }
  // }
}



void SMachine_Ctrl::INIT_I2C_Wires()
{
  WireOne.end();
  WireOne.begin(WireOne_SDA, WireOne_SCL, 100000);
}

/**
 * @brief 初始化各池的數值歷史紀錄
 * 若記錄檔存在，則讀取紀錄檔
 * 若不存在，則使用預設值
 * 
 */
bool SMachine_Ctrl::INIT_PoolData()
{
  if (SD.exists(FilePath__SD__LastSensorDataSave)) {
    if (ExFile_LoadJsonFile(SD, FilePath__SD__LastSensorDataSave, *JSON__sensorDataSave)) {
      ESP_LOGD("", "讀取最新各池感測器測量數值成功");
      return true;
    } else {
      ESP_LOGE("", "讀取最新各池感測器測量數值失敗");
    }
  }
  ESP_LOGE("", "找不到或讀取最新各池感測器測量數值失敗，重建資料庫");
  // for (int poolIndex=1;poolIndex<5;poolIndex++) {
  //   String poolName = "pool-"+String(poolIndex);
  //   (*JSON__sensorDataSave)[poolName]["PoolID"].set(poolName);
  //   (*JSON__sensorDataSave)[poolName]["PoolName"].set(D_poolItem.value().as<JsonObject>()["title"].as<String>());
  //   (*JSON__sensorDataSave)[poolName]["PoolDescription"].set(D_poolItem.value().as<JsonObject>()["desp"].as<String>());
  //   (*JSON__sensorDataSave)[poolName]["NO2_wash_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["NO2_wash_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[poolName]["NO2_test_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["NO2_test_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[poolName]["NO2"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["NO2"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[poolName]["NH4_wash_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["NH4_wash_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[poolName]["NH4_test_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["NH4_test_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[poolName]["NH4"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["NH4"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[poolName]["pH_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["pH_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[poolName]["pH"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[poolName]["pH"]["data_time"].set("1990-01-01 00:00:00");
  // }
  for (JsonVariant SinPoolInfo : (*JSON__PoolConfig).as<JsonArray>()) {
    JsonObject Array_SinPoolInfo = SinPoolInfo.as<JsonObject>();
    String PoolID = Array_SinPoolInfo["id"].as<String>();
    (*JSON__sensorDataSave)[PoolID]["PoolID"].set(PoolID);
    (*JSON__sensorDataSave)[PoolID]["PoolName"].set(Array_SinPoolInfo["title"].as<String>());
    (*JSON__sensorDataSave)[PoolID]["PoolDescription"].set(Array_SinPoolInfo["desp"].as<String>());
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NO2_wash_volt"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NO2_wash_volt"]["data_time"].set("1990-01-01 00:00:00");
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NO2_test_volt"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NO2_test_volt"]["data_time"].set("1990-01-01 00:00:00");
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NO2"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NO2"]["data_time"].set("1990-01-01 00:00:00");
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NH4_wash_volt"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NH4_wash_volt"]["data_time"].set("1990-01-01 00:00:00");
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NH4_test_volt"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NH4_test_volt"]["data_time"].set("1990-01-01 00:00:00");
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NH4"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["NH4"]["data_time"].set("1990-01-01 00:00:00");
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["pH_volt"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["pH_volt"]["data_time"].set("1990-01-01 00:00:00");
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["pH"]["Value"].set(-1.);
    (*JSON__sensorDataSave)[PoolID]["DataItem"]["pH"]["data_time"].set("1990-01-01 00:00:00");
  }
  // JsonObject D_pools = (*Machine_Ctrl.JSON__DeviceBaseInfo)["pools"].as<JsonObject>();
  // for (JsonPair D_poolItem : D_pools) {
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["PoolID"].set(D_poolItem.key());
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["PoolName"].set(D_poolItem.value().as<JsonObject>()["title"].as<String>());
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["PoolDescription"].set(D_poolItem.value().as<JsonObject>()["desp"].as<String>());
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NO2_wash_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NO2_wash_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NO2_test_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NO2_test_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NO2"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NO2"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NH4_wash_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NH4_wash_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NH4_test_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NH4_test_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NH4"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["NH4"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["pH_volt"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["pH_volt"]["data_time"].set("1990-01-01 00:00:00");
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["pH"]["Value"].set(-1.);
  //   (*JSON__sensorDataSave)[D_poolItem.key()]["pH"]["data_time"].set("1990-01-01 00:00:00");
  // }
  return false;
}

void SMachine_Ctrl::StopDeviceAndINIT()
{
  CleanAllStepTask();
  peristalticMotorsCtrl.SetAllMotorStop();
  MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
  digitalWrite(48, LOW);
  xSemaphoreGive(LOAD__ACTION_V2_xMutex);
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! OLED螢幕排程
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void OLEDCheckTask(void* parameter)
{ 
  String IPStringSave="";
  int ipList[4];
  for (;;) {
    if (WiFi.isConnected()) {
      String IpString = WiFi.localIP().toString();
      // Serial.println(IpString);
      if (IpString == IPStringSave) {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        continue;
      } else {
        IPStringSave = IpString;
      }
      String delimiter = ".";
      byte x0 = 3 + 64;
      byte y0 = 3;
      QRCode qrcode;
      uint8_t qrcodeData[qrcode_getBufferSize(3)];
      qrcode_initText(&qrcode, qrcodeData, 3, 0, ("http://" + IpString).c_str());
      int delimiterIndex = IpString.indexOf(delimiter);
      int rowChose = 0;
      while (delimiterIndex != -1) {
        ipList[rowChose] =  IpString.substring(0, delimiterIndex).toInt();
        IpString = IpString.substring(delimiterIndex+1);
        delimiterIndex = IpString.indexOf(delimiter);
        rowChose++;
      }
      ipList[3] = IpString.toInt();
      Machine_Ctrl.WireOne.end();
      U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, Machine_Ctrl.WireOne_SCL, Machine_Ctrl.WireOne_SDA);
      u8g2.begin();
      u8g2.setFont(u8g2_font_HelvetiPixelOutline_te);
      u8g2.firstPage();
      do {
        u8g2.setFont(u8g2_font_VCR_OSD_tn);
        u8g2.setColorIndex(1); // 設成白色
        for (int i = 0; i < 3; i++) {
          u8g2.drawUTF8(0, 16 * (i + 1), (String(ipList[i]) + ".").c_str());
        }
        u8g2.drawUTF8(0, 16 * (3 + 1), (String(ipList[3])).c_str());

        u8g2.drawBox(65, 0, 66, 64);
        for (uint8_t y = 0; y < qrcode.size; y++)
        {
          for (uint8_t x = 0; x < qrcode.size; x++)
          {
            if (qrcode_getModule(&qrcode, x, y))
            {
              u8g2.setColorIndex(0);
            }
            else
            {
              u8g2.setColorIndex(1);
            }
            u8g2.drawBox(x0 + x * 2, y0 + y * 2, 2, 2);
          }
        }
      } while ( u8g2.nextPage() );
      Wire.end();
      Machine_Ctrl.INIT_I2C_Wires();
    }
    else {
      if (IPStringSave == "") {
        vTaskDelay(1000/portTICK_PERIOD_MS);
        continue;
      }
      IPStringSave="";
      Machine_Ctrl.PrintOnScreen("The WiFi has not been connected yet, please wait for a while.\nIf you cannot connect for a long time, please check whether the settings are correct.");
    }
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}


void SMachine_Ctrl::BuildOLEDCheckTask() 
{
  xTaskCreate(
    OLEDCheckTask, "OLEDCheckTask",
    10000, NULL, 1, NULL
  );
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! 儀器時鐘確認Task
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void TimeCheckTask(void* parameter)
{ 
  time_t lasUpdatetime = 0;
  for (;;) {
    if (now() < 100000) {
      if (WiFi.isConnected()) {
        Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
        lasUpdatetime = now();
      }
    }
    else if (now() > lasUpdatetime + 3600) {
      Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
      lasUpdatetime = now();
    }
    vTaskDelay(5000/portTICK_PERIOD_MS);
  }
}


void SMachine_Ctrl::BuildTimeCheckTask() 
{
  xTaskCreate(
    TimeCheckTask, "TimeCheckTask",
    10000, NULL, 1, NULL
  );
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! 排程功能相關
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void ScheduleManager(void* parameter)
{ 
  for(;;) {
    ESP_LOGI("", "準備檢查是否有排程設定");
    time_t nowTime = now();
    if ( nowTime < 946656000 ) {
      ESP_LOGW("", "儀器時間有誤，暫時跳過排程設定檢查");
      if (WiFi.isConnected()) {
        // Machine_Ctrl.BackendServer.UpdateMachineTimerByNTP();
        vTaskDelay(5000/portTICK_PERIOD_MS);
      }
      vTaskDelay(1000/portTICK_PERIOD_MS);
      continue;
    }
    //? 每5分鐘檢查一次排程
    if (minute(nowTime) % 5 == 0) {
      int Weekday = weekday(nowTime);
      int Hour = hour(nowTime);
      int Minute = minute(nowTime);

      for (JsonPair ScheduleConfigChose : (*Machine_Ctrl.JSON__ScheduleConfig).as<JsonObject>()) {
        String ScheduleConfigID = String(ScheduleConfigChose.key().c_str());
        JsonObject ScheduleConfig = ScheduleConfigChose.value().as<JsonObject>();
        for (JsonVariant weekdayItem : ScheduleConfig["schedule"].as<JsonArray>()[1].as<JsonArray>()) {
          if (weekdayItem.as<int>() == Weekday) {
            for (JsonVariant hourItem : ScheduleConfig["schedule"].as<JsonArray>()[2].as<JsonArray>()) {
              if (hourItem.as<int>() == Hour) {
                for (JsonVariant minuteItem : ScheduleConfig["schedule"].as<JsonArray>()[3].as<JsonArray>()) {
                  if (minuteItem.as<int>() == Minute) {
                    if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdTRUE) {
                      ESP_LOGD("本雞排程", "執行排程瞜");
                      (*Machine_Ctrl.JSON__pipelineStack).clear();
                      int eventCount = 0;
                      for (JsonVariant poolScheduleItem : ScheduleConfig["schedule"].as<JsonArray>()[0].as<JsonArray>()) {
                        int targetIndex = poolScheduleItem.as<int>();
                        String TargetName = "pool_"+String(targetIndex+1)+"_all_data_get.json";
                        String FullFilePath = "/pipelines/"+TargetName;
                        DynamicJsonDocument singlePipelineSetting(10000);
                        singlePipelineSetting["FullFilePath"].set(FullFilePath);
                        singlePipelineSetting["TargetName"].set(TargetName);
                        singlePipelineSetting["stepChose"].set("");
                        singlePipelineSetting["eventChose"].set("");
                        singlePipelineSetting["eventIndexChose"].set(-1);
                        (*Machine_Ctrl.JSON__pipelineStack).add(singlePipelineSetting);
                        ESP_LOGD("WebSocket", " - 事件 %d", eventCount+1);
                        ESP_LOGD("WebSocket", "   - 檔案路徑:\t%s", FullFilePath.c_str());
                        ESP_LOGD("WebSocket", "   - 目標名稱:\t%s", TargetName.c_str());
                      }
                      Machine_Ctrl.LOAD__ACTION_V2(Machine_Ctrl.JSON__pipelineStack);
                    }
                    else {
                      ESP_LOGD("本雞排程", "儀器忙碌中，跳過本次排程");
                    }
                    break;
                  }
                }
                break;
              }
            }
            break;
          }
        }
      }
    }
    ESP_LOGI("", "排程檢查完畢，等待下一個檢查時段");
    vTaskDelay(1000*60*1/portTICK_PERIOD_MS);
  }
}


void SMachine_Ctrl::CreateScheduleManagerTask() 
{
  if (TASK__ScheduleManager == NULL) {
    xTaskCreate(
      ScheduleManager, "ScheduleManager",
      10000, NULL, 4, &TASK__ScheduleManager
    );
  }
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//! 流程設定相關
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

struct TaskInfo {
  char* stepsGroupName;
  int TaskThreadIndex;
  bool* TaskThreadFree;
  StackType_t* TaskThreadStackBuffer;
  StaticTask_t* TaskThreadTaskBuffer;
};


//? pipeline task詳細執行過程
void PiplelineFlowTask(void* parameter)
{ 
  //! TASK 有以下數種狀態
  //! 1. WAIT、2. NORUN、3. FAIL、4. SUCCESS、5. RUNNING、6. STOP_THIS_PIPELINE
  // char* stepsGroupName = (char*)parameter;

  TaskInfo TaskInfoItem = *((TaskInfo*)parameter);
  char* stepsGroupName = TaskInfoItem.stepsGroupName;

  String ThisPipelineTitle = (*Machine_Ctrl.JSON__pipelineConfig)["title"].as<String>();
  String stepsGroupNameString = String(stepsGroupName);
  String ThisStepGroupTitle = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["title"].as<String>();
  ESP_LOGI("", "建立 %s 的 Task", ThisStepGroupTitle.c_str());
  Machine_Ctrl.SetLog(
    3, "執行流程: "+ThisStepGroupTitle,"Pipeline: "+ThisPipelineTitle, Machine_Ctrl.BackendServer.ws_, NULL
  );

  (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("RUNNING");

  //? 這個 Task 要執行的 steps_group 的 list
  JsonArray stepsGroupArray = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["steps"].as<JsonArray>();

  //? 這個 Task 要執行的 parent list
  JsonObject parentList = (*Machine_Ctrl.JSON__pipelineConfig)["pipline"][stepsGroupNameString]["parentList"].as<JsonObject>();
  
  bool isStepFail = false;

  for (String eventChose : stepsGroupArray) {
    //? eventChose: 待執行的event名稱

    String thisEventTitle = (*Machine_Ctrl.JSON__pipelineConfig)["events"][eventChose]["title"].as<String>();

    ESP_LOGI("", " 執行: %s - %s", ThisStepGroupTitle.c_str(), thisEventTitle.c_str());
    Machine_Ctrl.SetLog(3, "執行事件: "+thisEventTitle,"流程: "+ThisStepGroupTitle, Machine_Ctrl.BackendServer.ws_, NULL);

    JsonArray eventList = (*Machine_Ctrl.JSON__pipelineConfig)["events"][eventChose]["event"].as<JsonArray>();
    String taskResult = "SUCCESS";
    for (JsonObject eventItem : eventList) {
      //! 伺服馬達控制設定
      if (eventItem.containsKey("pwm_motor_list")) {
        // digitalWrite(16, HIGH);
        // digitalWrite(17, HIGH);
        pinMode(4, OUTPUT);
        digitalWrite(4, HIGH);
        // int ServoMotorType = (*Machine_Ctrl.JSON__DeviceConfig)["ServoMotorType"].as<int>();
        int ServoMotorType = 2;
        if (ServoMotorType==2) {
          xSemaphoreTake(Machine_Ctrl.LX_20S_xMutex, portMAX_DELAY);
          Serial2.begin(115200,SERIAL_8N1, 9, 10);
          vTaskDelay(500/portTICK_PERIOD_MS);
          for (JsonObject pwmMotorItem : eventItem["pwm_motor_list"].as<JsonArray>()) {
            int targetAngValue = map(pwmMotorItem["status"].as<int>(), -30, 210, 0, 1000);
            ESP_LOGI("LOADED_ACTION","       - (LX-20S) %d 轉至 %d 度(%d)", 
              pwmMotorItem["index"].as<int>(), 
              pwmMotorItem["status"].as<int>(), targetAngValue
            );
            LX_20S_SerialServoMove(Serial2, pwmMotorItem["index"].as<int>(),targetAngValue,500);
          }
          vTaskDelay(2000/portTICK_PERIOD_MS);
          for (JsonObject pwmMotorItem : eventItem["pwm_motor_list"].as<JsonArray>()) {
            int targetAngValue = map(pwmMotorItem["status"].as<int>(), -30, 210, 0, 1000);
            int readAng = LX_20S_SerialServoReadPosition(Serial2, pwmMotorItem["index"].as<int>());
            int d_ang = targetAngValue - readAng;
            if (abs(d_ang)>15) {
              ESP_LOGE("LX-20S", "       - 伺服馬達(LX-20S) 編號 %d 並無轉到指定角度: %d，讀取到的角度為: %d",
                pwmMotorItem["index"].as<int>(), pwmMotorItem["status"].as<int>(),
                map(readAng, 0, 1000, -30, 210)
              );
            }
          }
          Serial2.end();
          xSemaphoreGive(Machine_Ctrl.LX_20S_xMutex);
        } else {
          vTaskDelay(50/portTICK_PERIOD_MS);
          Machine_Ctrl.motorCtrl.ResetPCA9685();
          for (JsonObject pwmMotorItem : eventItem["pwm_motor_list"].as<JsonArray>()) {
            // ESP_LOGI("LOADED_ACTION","       - %d 轉至 %d 度", 
            //   pwmMotorItem["index"].as<int>(), 
            //   pwmMotorItem["status"].as<int>()
            // );
            Machine_Ctrl.motorCtrl.SetMotorTo(pwmMotorItem["index"].as<int>(), pwmMotorItem["status"].as<int>());
            vTaskDelay(50/portTICK_PERIOD_MS);
          }
          vTaskDelay(2000/portTICK_PERIOD_MS);
          Machine_Ctrl.motorCtrl.ResetPCA9685();
        }
        digitalWrite(4, LOW);
        
        // digitalWrite(16, LOW);
        // digitalWrite(17, LOW);
      }
      //! 蠕動馬達控制設定
      else if (eventItem.containsKey("peristaltic_motor_list")) {
        pinMode(48, OUTPUT);
        //? endTimeCheckList: 記錄各蠕動馬達最大運行結束時間
        DynamicJsonDocument endTimeCheckList(10000);
        
        for (JsonObject peristalticMotorItem : eventItem["peristaltic_motor_list"].as<JsonArray>()) {
          int motorIndex = peristalticMotorItem["index"].as<int>();
          String motorIndexString = String(motorIndex);
          PeristalticMotorStatus ststus = peristalticMotorItem["status"].as<int>() == 1 ? PeristalticMotorStatus::FORWARD : PeristalticMotorStatus::REVERSR;
          float runTime = peristalticMotorItem["time"].as<String>().toFloat();
          
          String untilString = peristalticMotorItem["until"].as<String>();
          //? failType: 錯誤判斷的準則，目前有幾種類型
          //? 1. no: 無錯誤判斷
          //? 2. timeout: 超時
          //? 3. connect: 指定pin導通
          String failType = peristalticMotorItem["failType"].as<String>();

          //? failAction: 觸發錯誤判斷時的反應，目前有幾種類型
          //? 1. continue: 繼續執行，但最後流程Task完成後會標記為Fail
          //? 2. stepStop: 當前流程Task關閉
          //? 3. stopImmediately: 整台機器停止動作
          String failAction = peristalticMotorItem["failAction"].as<String>();

          if (endTimeCheckList.containsKey(motorIndexString)) {
            continue;
          }
          ESP_LOGI("LOADED_ACTION","       - (%d) %s 持續 %.2f 秒或是直到%s被觸發,failType: %s, failAction: %s", 
            motorIndex, 
            peristalticMotorItem["status"].as<int>()==-1 ? "正轉" : "反轉", 
            runTime,
            untilString.c_str(),  
            failType.c_str(), failAction.c_str()
          );

          endTimeCheckList[motorIndexString]["index"] = motorIndex;
          endTimeCheckList[motorIndexString]["status"] = ststus;
          endTimeCheckList[motorIndexString]["time"] = runTime;
          endTimeCheckList[motorIndexString]["failType"] = failType;
          endTimeCheckList[motorIndexString]["failAction"] = failAction;
          endTimeCheckList[motorIndexString]["finish"] = false;
          
          if (untilString == "RO") {
            endTimeCheckList[motorIndexString]["until"] = 1;
          } 
          else if (untilString == "SAMPLE") {
            endTimeCheckList[motorIndexString]["until"] = 2;
          }
          else {
            endTimeCheckList[motorIndexString]["until"] = -1;
          }
          Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(motorIndex, ststus);
          // Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
          endTimeCheckList[motorIndexString]["startTime"] = millis();
          endTimeCheckList[motorIndexString]["endTime"] = millis() + (long)(runTime*1000);
          // vTaskDelay(100/portTICK_PERIOD_MS);
        }


        Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
        digitalWrite(48, HIGH);


        //? 開始loop所有馬達狀態，來決定是否繼續執行、停下、觸發錯誤...等等等
        bool allFinish = false;
        while (allFinish == false){
          allFinish = true;
          for (const auto& endTimeCheck : endTimeCheckList.as<JsonObject>()) {
            JsonObject endTimeCheckJSON = endTimeCheck.value().as<JsonObject>();
            if (endTimeCheckJSON["finish"] == true) {
              continue;
            }
            allFinish = false;
            long endTime = endTimeCheckJSON["endTime"].as<long>();
            int until = endTimeCheckJSON["until"].as<int>();
            int motorIndex = endTimeCheckJSON["index"].as<int>();
            String thisFailType = endTimeCheckJSON["failType"].as<String>();
            String thisFailAction = endTimeCheckJSON["failAction"].as<String>();

            //? 若馬達執行時間達到最大時間
            if (millis() >= endTime) {
              //? 執行到這，代表馬達執行到最大執行時間，如果 failType 是 timeout，則代表觸發失敗判斷
              if (thisFailType == "timeout") {
                ESP_LOGE("", "蠕動馬達觸發Timeout錯誤，錯誤處裡: %s", thisFailAction.c_str());

                if (thisFailAction=="stepStop") {
                  for (const auto& motorChose : endTimeCheckList.as<JsonObject>()) {
                    //? 強制停止當前step執行的馬達
                    Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(motorIndex, PeristalticMotorStatus::STOP);
                    Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
                  }
                  endTimeCheckJSON["finish"].set(true);
                  (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("FAIL");
                  Machine_Ctrl.pipelineTaskHandleMap.erase(stepsGroupNameString);
                  free(stepsGroupName);
                  *TaskInfoItem.TaskThreadFree = true;
                  vTaskDelete(NULL);
                }
                else if (thisFailAction=="stopImmediately") {
                  ESP_LOGE("", "緊急終止儀器");
                  Machine_Ctrl.StopDeviceAndINIT();
                }
                else if (thisFailAction=="stopPipeline") {
                  ESP_LOGE("", "停止當前流程，若有下一個排隊中的流程就執行他");
                  (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("STOP_THIS_PIPELINE");
                  Machine_Ctrl.pipelineTaskHandleMap.erase(stepsGroupNameString);
                  free(stepsGroupName);
                  *TaskInfoItem.TaskThreadFree = true;
                  vTaskDelete(NULL);
                }
                //? 若非，則正常停止馬達運行
                Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(motorIndex, PeristalticMotorStatus::STOP);
                Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
                ESP_LOGV("", "蠕動馬達執行至最大時間");
                endTimeCheckJSON["finish"].set(true);
              }
              else {
                //? 若非，則正常停止馬達運行
                Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(motorIndex, PeristalticMotorStatus::STOP);
                Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
                ESP_LOGV("", "蠕動馬達執行至最大時間");
                endTimeCheckJSON["finish"].set(true);
              }
            }
            //? 若要判斷滿水浮球狀態
            else if (until != -1) {
              pinMode(until, INPUT);
              int value = digitalRead(until);
              if (value == HIGH) {
                Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(motorIndex, PeristalticMotorStatus::STOP);
                Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
                ESP_LOGV("", "浮球觸發，關閉蠕動馬達");
                //? 判斷是否有觸發錯誤
                if (thisFailType == "connect") {
                  ESP_LOGE("", "蠕動馬達觸發connect錯誤");
                  if (thisFailAction=="stepStop") {
                    for (const auto& motorChose : endTimeCheckList.as<JsonObject>()) {
                      //? 強制停止當前step執行的馬達
                      Machine_Ctrl.peristalticMotorsCtrl.SetMotorStatus(motorIndex, PeristalticMotorStatus::STOP);
                      Machine_Ctrl.peristalticMotorsCtrl.RunMotor(Machine_Ctrl.peristalticMotorsCtrl.moduleDataList);
                    }
                    endTimeCheckJSON["finish"].set(true);
                    (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("FAIL");
                    Machine_Ctrl.pipelineTaskHandleMap.erase(stepsGroupNameString);
                    free(stepsGroupName);
                    *TaskInfoItem.TaskThreadFree = true;
                    vTaskDelete(NULL);
                  }
                  else if (thisFailAction=="stopImmediately") {
                    Machine_Ctrl.StopDeviceAndINIT();
                  }
                }
                endTimeCheckJSON["finish"].set(true);
              }
            }
            vTaskDelay(100);
          }
        }
        if (Machine_Ctrl.peristalticMotorsCtrl.IsAllStop()) {
          digitalWrite(48, LOW);
        }
      }
      //! 分光光度計控制設定
      else if (eventItem.containsKey("spectrophotometer_list")) {
        int SensorVersion = (*Machine_Ctrl.JSON__DeviceConfig)["SensorVersion"].as<int>();
        if (SensorVersion == 1) {
          ESP_LOGV("", "使用I2C版本PD的光度計模組");
          for (JsonObject spectrophotometerItem : eventItem["spectrophotometer_list"].as<JsonArray>()) {
            int spectrophotometerIndex = spectrophotometerItem["index"].as<int>();
            JsonObject spectrophotometerConfigChose = (*Machine_Ctrl.JSON__SpectrophotometerConfig)[spectrophotometerIndex].as<JsonObject>();
            String spectrophotometerTitle = spectrophotometerConfigChose["title"].as<String>();
            String GainStr = spectrophotometerItem["gain"].as<String>();
            String targetChannel = spectrophotometerItem["channel"].as<String>();
            String value_name = spectrophotometerItem["value_name"].as<String>();
            String poolChose = spectrophotometerItem["pool"].as<String>();
            //? type: 光度計動作類型，當前有兩種
            //? 1. Adjustment:  調整數位電阻數值，有錯誤判斷，當數值不達標時有觸發錯誤機制
            //? 2. Measurement: 不調整電阻並量測出PPM數值
            String type = spectrophotometerItem["type"].as<String>();
            String failAction = spectrophotometerItem["failAction"].as<String>();

            //? 開啟指定index模組
            Machine_Ctrl.WireOne.beginTransmission(0x70);
            Machine_Ctrl.WireOne.write(1 << spectrophotometerIndex);
            Machine_Ctrl.WireOne.endTransmission();
            vTaskDelay(500/portTICK_PERIOD_MS);
            Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.openSensorByIndex(spectrophotometerIndex);
            vTaskDelay(500/portTICK_PERIOD_MS);

            //? 依放大倍率設定調整
            if (GainStr == "1X") {
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_1X);
            }
            else if (GainStr == "2X") {
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_2X);
            }
            else if (GainStr == "4X") {
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_4X);
            }
            else if (GainStr == "8X") {
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_8X);
            }
            else if (GainStr == "48X") {
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_48X);
            }
            else if (GainStr == "96X") {
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.SetGain(ALS_Gain::Gain_96X);
            }

            //! 調整數值
            if (type == "Adjustment") {
              int targetLevel = spectrophotometerItem["target"].as<int>();
              char logBuffer[1000];
              sprintf(
                logBuffer, 
                "(%d)%s 指定池: %s, 測量倍率: %s, 指定頻道: %s, 調整強度目標: %d, 並紀錄為: %s, 若不達標觸發錯誤行為: %s",
                spectrophotometerIndex, spectrophotometerTitle.c_str(), poolChose.c_str(),
                GainStr.c_str(), targetChannel.c_str(), targetLevel, value_name.c_str(), failAction.c_str()
              );
              Machine_Ctrl.SetLog(3, "調整光強度,並記錄A0數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
              ESP_LOGI("LOADED_ACTION","       - %s",String(logBuffer).c_str());

              ALS_01_Data_t sensorData;
              uint16_t checkBuffer_CH0 [10];
              uint16_t checkBuffer_CH1 [10];
              double checkValue_max;
              double checkValue_min;
              double checkValue_fixing;
              
              bool fixFail = true;

              //? 進行各種量測檢查，若遇到錯誤則直接break;
              for (int xxx=0;xxx<1;xxx++) {
                //STEP 檢查最暗時亮度有沒有低於目標值
                Machine_Ctrl.WireOne.beginTransmission(0x2F);
                Machine_Ctrl.WireOne.write(0b00000000);
                Machine_Ctrl.WireOne.write(0);
                Machine_Ctrl.WireOne.endTransmission();
                for (int i=0;i<10;i++) {
                  sensorData = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
                  checkBuffer_CH0[i] = sensorData.CH_0;
                  checkBuffer_CH1[i] = sensorData.CH_1;
                }
                if (targetChannel == "CH0") {
                  checkValue_min = afterFilterValue(checkBuffer_CH0, 10);
                }
                else if (targetChannel == "CH1") {
                  checkValue_min = afterFilterValue(checkBuffer_CH1, 10);
                }
                if (checkValue_min > (double)targetLevel) {
                  sprintf(logBuffer, "光度計最暗強度為: %s, 大於目標值: %d",String(checkValue_min, 2).c_str(), targetLevel);
                  Machine_Ctrl.SetLog(2, "調整光強度,並記錄A0數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
                  ESP_LOGI("LOADED_ACTION","       - %s",String(logBuffer).c_str());
                  spectrophotometerConfigChose["level"].set(0);
                  Machine_Ctrl.lastLightValue_CH0 = afterFilterValue(checkBuffer_CH0, 10);
                  Machine_Ctrl.lastLightValue_CH1 = afterFilterValue(checkBuffer_CH1, 10);
                  fixFail = false;
                  break;
                }

                //STEP 檢查最亮時亮度有沒有低於目標值
                Machine_Ctrl.WireOne.beginTransmission(0x2F);
                Machine_Ctrl.WireOne.write(0b00000000);
                Machine_Ctrl.WireOne.write(255);
                Machine_Ctrl.WireOne.endTransmission();
                for (int i=0;i<10;i++) {
                  sensorData = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
                  checkBuffer_CH0[i] = sensorData.CH_0;
                  checkBuffer_CH1[i] = sensorData.CH_1;
                }
                if (targetChannel == "CH0") {
                  checkValue_max = afterFilterValue(checkBuffer_CH0, 10);
                }
                else if (targetChannel == "CH1") {
                  checkValue_max = afterFilterValue(checkBuffer_CH1, 10);
                }
                if (checkValue_max < (double)targetLevel) {
                  sprintf(logBuffer, "光度計最亮時強度為: %s, 小於目標值: %d",String(checkValue_max, 2).c_str(), targetLevel);
                  Machine_Ctrl.SetLog(2, "調整光強度,並記錄A0數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
                  ESP_LOGI("LOADED_ACTION","       - %s",String(logBuffer).c_str());
                  spectrophotometerConfigChose["level"].set(255);
                  Machine_Ctrl.lastLightValue_CH0 = afterFilterValue(checkBuffer_CH0, 10);
                  Machine_Ctrl.lastLightValue_CH1 = afterFilterValue(checkBuffer_CH1, 10);
                  fixFail = false;
                  break;
                }
                
                //STEP 目標亮度介於最暗與最亮之間，以最亮與最暗為準，內插出最可能接近目標值的設定
                int guestValue = (int)(255*((double)targetLevel-checkValue_min)/(checkValue_max-checkValue_min));
                spectrophotometerConfigChose["level"].set(guestValue);
                Machine_Ctrl.WireOne.beginTransmission(0x2F);
                Machine_Ctrl.WireOne.write(0b00000000);
                Machine_Ctrl.WireOne.write(guestValue);
                Machine_Ctrl.WireOne.endTransmission();
                for (int i=0;i<10;i++) {
                  sensorData = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
                  checkBuffer_CH0[i] = sensorData.CH_0;
                  checkBuffer_CH1[i] = sensorData.CH_1;
                }
                if (targetChannel == "CH0") {
                  checkValue_fixing = afterFilterValue(checkBuffer_CH0, 10);
                }
                else if (targetChannel == "CH1") {
                  checkValue_fixing = afterFilterValue(checkBuffer_CH1, 10);
                }
                //? 若調整後的值小於目標，則慢慢向上調整，直到到達目標強度為止
                if (checkValue_fixing <= (double)targetLevel) {
                  while (true) {
                    if (guestValue >= 255) {break;}
                    Machine_Ctrl.WireOne.beginTransmission(0x2F);
                    Machine_Ctrl.WireOne.write(0b00000000);
                    Machine_Ctrl.WireOne.write(guestValue);
                    Machine_Ctrl.WireOne.endTransmission();
                    for (int i=0;i<10;i++) {
                      sensorData = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
                      checkBuffer_CH0[i] = sensorData.CH_0;
                      checkBuffer_CH1[i] = sensorData.CH_1;
                      Serial.printf("Adjustment\tOrigin\tLevel: %d\tCH0: %d,\tCH1: %d\n", guestValue, sensorData.CH_0, sensorData.CH_1);
                    }
                    Machine_Ctrl.lastLightValue_CH0 = afterFilterValue(checkBuffer_CH0, 10);
                    Machine_Ctrl.lastLightValue_CH1 = afterFilterValue(checkBuffer_CH1, 10);
                    Serial.printf("Adjustment\tResult\tLevel: %d\tCH0: %d,\tCH1: %d\n",guestValue, String(Machine_Ctrl.lastLightValue_CH0, 2).c_str(), String(Machine_Ctrl.lastLightValue_CH1, 2).c_str());
                    if (targetChannel == "CH0") {
                      checkValue_fixing = Machine_Ctrl.lastLightValue_CH0;
                    }
                    else if (targetChannel == "CH1") {
                      checkValue_fixing = Machine_Ctrl.lastLightValue_CH1;
                    }
                    if (checkValue_fixing >= (double)targetLevel) {
                      fixFail = false;
                      spectrophotometerConfigChose["level"].set(guestValue);
                      break;
                    }
                    guestValue++;
                  }
                }
                //? 若調整後的值大於目標，則慢慢向下調整，直到到達目標強度為止
                else {
                  while(true) {
                    if (guestValue <= 0) {break;}
                    Machine_Ctrl.WireOne.beginTransmission(0x2F);
                    Machine_Ctrl.WireOne.write(0b00000000);
                    Machine_Ctrl.WireOne.write(guestValue);
                    Machine_Ctrl.WireOne.endTransmission();
                    for (int i=0;i<10;i++) {
                      sensorData = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
                      checkBuffer_CH0[i] = sensorData.CH_0;
                      checkBuffer_CH1[i] = sensorData.CH_1;
                      Serial.printf("Adjustment\tOrigin\tLevel: %d\tCH0: %d,\tCH1: %d\n", guestValue, sensorData.CH_0, sensorData.CH_1);
                    }
                    if (targetChannel == "CH0") {
                      checkValue_fixing = afterFilterValue(checkBuffer_CH0, 10);
                    }
                    else if (targetChannel == "CH1") {
                      checkValue_fixing = afterFilterValue(checkBuffer_CH1, 10);
                    }
                    if (checkValue_fixing < (double)targetLevel) {
                      fixFail = false;
                      spectrophotometerConfigChose["level"].set(guestValue+1);
                      break;
                    }
                    Machine_Ctrl.lastLightValue_CH0 = afterFilterValue(checkBuffer_CH0, 10);
                    Machine_Ctrl.lastLightValue_CH1 = afterFilterValue(checkBuffer_CH1, 10);
                    Serial.printf("Adjustment\tResult\tLevel: %d\tCH0: %d,\tCH1: %d\n",guestValue, String(Machine_Ctrl.lastLightValue_CH0, 2).c_str(), String(Machine_Ctrl.lastLightValue_CH1, 2).c_str());
                    guestValue--;
                  }
                }
              }

              Machine_Ctrl.WireOne.beginTransmission(0x70);
              Machine_Ctrl.WireOne.write(1 << 7);
              Machine_Ctrl.WireOne.endTransmission();
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();

              double finalValue;
              if (targetChannel == "CH0") {
                finalValue = Machine_Ctrl.lastLightValue_CH0;
              }
              else if (targetChannel == "CH1") {
                finalValue = Machine_Ctrl.lastLightValue_CH1;
              }
              String DataFileFullPath = Machine_Ctrl.SensorDataFolder + Machine_Ctrl.GetDateString("") + "_data.csv";
              Machine_Ctrl.InsertNewDataToDB(
                Machine_Ctrl.GetDatetimeString(),
                spectrophotometerTitle, 
                poolChose,
                value_name,
                finalValue,
                Machine_Ctrl.TaskUUID
              );
              Machine_Ctrl.SaveSensorData_photometer(
                DataFileFullPath,Machine_Ctrl.GetDatetimeString() ,spectrophotometerTitle, poolChose, GainStr, targetChannel,
                value_name, -1, finalValue, -1
              );
              //? websocket相關的數值要限縮在小數點下第2位
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose][value_name].set(String(finalValue,2).toDouble());

              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["Value"].set(String(finalValue,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["data_time"].set(Machine_Ctrl.GetDatetimeString());

              Machine_Ctrl.ReWriteLastDataSaveFile(Machine_Ctrl.FilePath__SD__LastSensorDataSave, (*Machine_Ctrl.JSON__sensorDataSave).as<JsonObject>());
              sprintf(logBuffer, "最終取調整值:%d, CH0 強度: %s, CH1 強度: %s", 
                spectrophotometerConfigChose["level"].as<int>(), String(Machine_Ctrl.lastLightValue_CH0, 2).c_str(),
                String(Machine_Ctrl.lastLightValue_CH1, 2).c_str()
              );
              if (fixFail) {
                Machine_Ctrl.SetLog(2, "調整光強度失敗", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
                ESP_LOGI("LOADED_ACTION","       - 調整光強度失敗 %s",String(logBuffer).c_str());
                if (failAction == "stepStop") {
                  //? 當前step流程中止
                  (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("FAIL");
                  Machine_Ctrl.pipelineTaskHandleMap[stepsGroupNameString] = NULL;
                  Machine_Ctrl.pipelineTaskHandleMap.erase(stepsGroupNameString);
                  free(stepsGroupName);
                  vTaskDelete(NULL);
                }
                else if (failAction == "stopImmediately") {
                  Machine_Ctrl.StopDeviceAndINIT();
                }
                isStepFail = true;
              } else {
                Machine_Ctrl.SetLog(5, "調整光強度成功", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
                ESP_LOGI("LOADED_ACTION","       - 調整光強度成功 %s",String(logBuffer).c_str());
              }
            }
            //! 數據測量
            else if (type=="Measurement") {
              double dilution = spectrophotometerItem["dilution"].as<String>().toDouble();
              double max = spectrophotometerItem["max"].as<String>().toDouble();
              double min = spectrophotometerItem["min"].as<String>().toDouble();
              double mValue = spectrophotometerConfigChose["calibration"][0]["ret"]["m"].as<double>();
              double bValue = spectrophotometerConfigChose["calibration"][0]["ret"]["b"].as<double>();
              String TargetType = value_name.substring(0,3); 

              char logBuffer[3000];
              sprintf(
                logBuffer, 
                "(%d)%s 測量倍率: %s, 指定頻道: %s, 量測數值, 並紀錄為: %s, 若數值不介於 %s - %s 則觸發錯誤行為: %s, 最後算出PPM數值: %s",
                spectrophotometerIndex, spectrophotometerTitle.c_str(), GainStr.c_str(), targetChannel.c_str(), value_name.c_str(), 
                String(min, 2).c_str(), String(max, 2).c_str(), failAction.c_str(), TargetType.c_str()
              );
              String logString = String(logBuffer);

              ESP_LOGI("LOADED_ACTION","       - %s", logString.c_str());
              Machine_Ctrl.SetLog(3, "測量PPM數值", logString, Machine_Ctrl.BackendServer.ws_);

              ALS_01_Data_t sensorData;
              uint16_t CH0_Buff [30];
              uint16_t CH1_Buff [30];
              double CH0_result, CH1_result;
              double CH0_after, CH1_after;
              Machine_Ctrl.WireOne.beginTransmission(0x2F);
              Machine_Ctrl.WireOne.write(0b00000000);
              Machine_Ctrl.WireOne.write(spectrophotometerConfigChose["level"].as<int>());
              Machine_Ctrl.WireOne.endTransmission();
              for (int i=0;i<30;i++) {
                sensorData = Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.TakeOneValue();
                Serial.printf("Measurement\tOrigin\tLevel: %d\tCH0: %d,\tCH1: %d\n", spectrophotometerConfigChose["level"].as<int>(), sensorData.CH_0, sensorData.CH_1);
                CH0_Buff[i] = sensorData.CH_0;
                CH1_Buff[i] = sensorData.CH_1;
              }
              CH0_result = afterFilterValue(CH0_Buff, 30);
              CH1_result = afterFilterValue(CH1_Buff, 30);
              Serial.printf("Measurement\tResult\tLevel: %d\tCH0: %s,\tCH1: %s\n", spectrophotometerConfigChose["level"].as<int>(), String(CH0_result,2).c_str(), String(CH1_result,2).c_str());

              CH0_after = (-log10(CH0_result/Machine_Ctrl.lastLightValue_CH0)-bValue)/mValue * dilution;
              CH1_after = (-log10(CH1_result/Machine_Ctrl.lastLightValue_CH1)-bValue)/mValue * dilution;

              Machine_Ctrl.WireOne.beginTransmission(0x70);
              Machine_Ctrl.WireOne.write(1 << 7);
              Machine_Ctrl.WireOne.endTransmission();
              Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
              
              double failCheckValue;
              double failCheckPPM;
              if (targetChannel == "CH0") {
                failCheckValue = CH0_result;
                failCheckPPM = CH0_after;
              } else {
                failCheckValue = CH1_result;
                failCheckPPM = CH1_after;
              }
              if (failCheckPPM < 0) {
                failCheckPPM = 0;
              }
              //? websocket相關的數值要限縮在小數點下第2位
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose][value_name].set(String(failCheckValue,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose][TargetType].set(String(failCheckPPM,2).toDouble());
              sprintf(
                logBuffer, 
                "測量結果: %s ,設定值: %d, 頻道: %s, 原始數值: %s, 轉換後PPM: %s",
                TargetType.c_str(), spectrophotometerConfigChose["level"].as<int>(), targetChannel.c_str(), String(failCheckValue, 2).c_str(), String(failCheckPPM, 2).c_str()
              );
              ESP_LOGI("LOADED_ACTION","       - %s", String(logBuffer).c_str());
              Machine_Ctrl.SetLog(3, "測量PPM數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);


              String DataFileFullPath = Machine_Ctrl.SensorDataFolder + Machine_Ctrl.GetDateString("") + "_data.csv";
              Machine_Ctrl.SaveSensorData_photometer(
                DataFileFullPath,Machine_Ctrl.GetDatetimeString() ,spectrophotometerTitle, poolChose, GainStr, targetChannel,
                value_name, dilution, failCheckValue, failCheckPPM
              );
              
              Machine_Ctrl.InsertNewDataToDB(
                Machine_Ctrl.GetDatetimeString(),
                spectrophotometerTitle, 
                poolChose,
                value_name,
                failCheckValue,
                Machine_Ctrl.TaskUUID
              );
              Machine_Ctrl.InsertNewDataToDB(
                Machine_Ctrl.GetDatetimeString(),
                spectrophotometerTitle, 
                poolChose,
                TargetType,
                failCheckPPM,
                Machine_Ctrl.TaskUUID
              );


              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["Value"].set(String(failCheckValue,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["data_time"].set(Machine_Ctrl.GetDatetimeString());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][TargetType]["Value"].set(String(failCheckPPM,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][TargetType]["data_time"].set(Machine_Ctrl.GetDatetimeString());
              Machine_Ctrl.ReWriteLastDataSaveFile(Machine_Ctrl.FilePath__SD__LastSensorDataSave, (*Machine_Ctrl.JSON__sensorDataSave).as<JsonObject>());

              if (failCheckValue < min or failCheckValue > max) {
                sprintf(
                  logBuffer, 
                  "測量光度述職時發現錯誤，獲得數值: %s, 不再設定範圍內 %s - %s",
                  String(failCheckValue, 2).c_str(), String(min, 2).c_str(), String(max, 2).c_str()
                );
                ESP_LOGI("LOADED_ACTION","       - %s", String(logBuffer).c_str());
                Machine_Ctrl.SetLog(1, "測量PPM數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);

                if (failAction == "stepStop") {
                  //? 當前step流程中止
                  (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("FAIL");
                  Machine_Ctrl.pipelineTaskHandleMap[stepsGroupNameString] = NULL;
                  Machine_Ctrl.pipelineTaskHandleMap.erase(stepsGroupNameString);
                  free(stepsGroupName);
                  vTaskDelete(NULL);
                }
                else if (failAction == "stopImmediately") {
                  Machine_Ctrl.StopDeviceAndINIT();
                }
                isStepFail = true;
              }
            }
          }
        }
        else if (SensorVersion == 2) {
          ESP_LOGV("", "使用電壓式版本PD的光度計模組");
          for (JsonObject spectrophotometerItem : eventItem["spectrophotometer_list"].as<JsonArray>()) {
            int spectrophotometerIndex = spectrophotometerItem["index"].as<int>();
            JsonObject spectrophotometerConfigChose = (*Machine_Ctrl.JSON__SpectrophotometerConfig)[spectrophotometerIndex].as<JsonObject>();
            String spectrophotometerTitle = spectrophotometerConfigChose["title"].as<String>();
            String GainStr = spectrophotometerItem["gain"].as<String>();
            String targetChannel = spectrophotometerItem["channel"].as<String>();
            String value_name = spectrophotometerItem["value_name"].as<String>();
            String poolChose = spectrophotometerItem["pool"].as<String>();
            //? type: 光度計動作類型，當前有兩種
            //? 1. Adjustment:  調整數位電阻數值，有錯誤判斷，當數值不達標時有觸發錯誤機制
            //? 2. Measurement: 不調整電阻並量測出PPM數值
            String type = spectrophotometerItem["type"].as<String>();
            String failAction = spectrophotometerItem["failAction"].as<String>();

            //? spectrophotometerIndex 為 0 時，代表藍光模組
            //? spectrophotometerIndex 為 1 時，代表綠光模組
            int activePin;
            int sensorAddr;
            if (spectrophotometerIndex == 0) {
              activePin = 16;
              sensorAddr = 0x44;
            } else {
              activePin = 17;
              sensorAddr = 0x45;
              // sensorAddr = 0x4F;
            }

            if (type == "Adjustment") {
              char logBuffer[1000];
              sprintf(
                logBuffer, 
                "(%d)%s 指定池: %s, 紀錄為: %s, 若不達標觸發錯誤行為: %s",
                spectrophotometerIndex, spectrophotometerTitle.c_str(), poolChose.c_str(),
                value_name.c_str(), failAction.c_str()
              );
              Machine_Ctrl.SetLog(3, "測量初始強度", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
              ESP_LOGI("LOADED_ACTION","       - %s",String(logBuffer).c_str());

              digitalWrite(activePin, HIGH);
              vTaskDelay(500/portTICK_PERIOD_MS);
              INA226 ina226(Machine_Ctrl.WireOne);
              ina226.begin(sensorAddr);

              ina226.configure(
                INA226_AVERAGES_4, // 采样平均
                INA226_BUS_CONV_TIME_140US, //采样时间
                INA226_SHUNT_CONV_TIME_140US, //采样时间
                INA226_MODE_SHUNT_BUS_CONT // Shunt和Bus电压连续测量
              );
              ina226.calibrate(0.01, 4);
              int countNum = 100;
              double shuntvoltageBuffer_ina226 = 0;
              double busvoltageBuffer_ina226 = 0;
              for (int i=0;i<countNum;i++) {
                shuntvoltageBuffer_ina226 += (double)ina226.readShuntVoltage();
                busvoltageBuffer_ina226 += (double)ina226.readBusVoltage();
                vTaskDelay(10/portTICK_PERIOD_MS);
              }
              double shuntvoltage_ina226 = shuntvoltageBuffer_ina226/countNum;
              // Serial.printf("shuntvoltage_ina226: %.2f\n", shuntvoltage_ina226);
              double busvoltage_ina226 = busvoltageBuffer_ina226/countNum;
              // double finalValue = (busvoltage_ina226 + (shuntvoltage_ina226))*1000;
              double finalValue = busvoltage_ina226*1000;
              Serial.printf("%.2f\n", finalValue);
              digitalWrite(activePin, LOW);
              Machine_Ctrl.lastLightValue = finalValue;
              String DataFileFullPath = Machine_Ctrl.SensorDataFolder + Machine_Ctrl.GetDateString("") + "_data.csv";
              Machine_Ctrl.SaveSensorData_photometer(
                DataFileFullPath,Machine_Ctrl.GetDatetimeString() ,spectrophotometerTitle, poolChose, "-", "-",
                value_name, -1, finalValue, -1
              );
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["Value"].set(String(finalValue,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["data_time"].set(Machine_Ctrl.GetDatetimeString());
              Machine_Ctrl.ReWriteLastDataSaveFile(Machine_Ctrl.FilePath__SD__LastSensorDataSave, (*Machine_Ctrl.JSON__sensorDataSave).as<JsonObject>());

              Machine_Ctrl.InsertNewDataToDB(
                Machine_Ctrl.GetDatetimeString(),
                spectrophotometerTitle, 
                poolChose,
                value_name,
                finalValue,
                Machine_Ctrl.TaskUUID
              );

              sprintf(
                logBuffer, 
                "測量結果: %s 初始強度: %s mV",
                value_name.substring(0,3).c_str(), String(finalValue, 2).c_str()
              );
              Machine_Ctrl.SetLog(3, "測量初始強度", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
              ESP_LOGI("LOADED_ACTION","       - %s",String(logBuffer).c_str());

            }
            //! 數據測量
            else if (type=="Measurement") {
              double dilution = spectrophotometerItem["dilution"].as<String>().toDouble();
              double max = spectrophotometerItem["max"].as<String>().toDouble();
              double min = spectrophotometerItem["min"].as<String>().toDouble();
              double mValue = spectrophotometerConfigChose["calibration"][0]["ret"]["m"].as<double>();
              double bValue = spectrophotometerConfigChose["calibration"][0]["ret"]["b"].as<double>();
              String TargetType = value_name.substring(0,3); 

              char logBuffer[3000];
              sprintf(
                logBuffer, 
                "(%d)%s 量測數值, 並紀錄為: %s, 若數值不介於 %s - %s 則觸發錯誤行為: %s, 最後算出PPM數值: %s",
                spectrophotometerIndex, spectrophotometerTitle.c_str(), value_name.c_str(), 
                String(min, 2).c_str(), String(max, 2).c_str(), failAction.c_str(), TargetType.c_str()
              );
              String logString = String(logBuffer);

              ESP_LOGI("LOADED_ACTION","       - %s", logString.c_str());
              Machine_Ctrl.SetLog(3, "測量PPM數值", logString, Machine_Ctrl.BackendServer.ws_);
              digitalWrite(activePin, HIGH);
              vTaskDelay(500/portTICK_PERIOD_MS);
              INA226 ina226(Machine_Ctrl.WireOne);
              ina226.begin(sensorAddr);

              ina226.configure(
                INA226_AVERAGES_4, // 采样平均
                INA226_BUS_CONV_TIME_140US, //采样时间
                INA226_SHUNT_CONV_TIME_140US, //采样时间
                INA226_MODE_SHUNT_BUS_CONT // Shunt和Bus电压连续测量
              );
              ina226.calibrate(0.01, 4);
              int countNum = 100;
              double shuntvoltageBuffer_ina226 = 0;
              double busvoltageBuffer_ina226 = 0;
              for (int i=0;i<countNum;i++) {
                shuntvoltageBuffer_ina226 += (double)ina226.readShuntVoltage();
                busvoltageBuffer_ina226 += (double)ina226.readBusVoltage();
                vTaskDelay(10/portTICK_PERIOD_MS);
              }
              double shuntvoltage_ina226 = shuntvoltageBuffer_ina226/countNum;
              // Serial.printf("shuntvoltage_ina226: %.2f\n", shuntvoltage_ina226);
              double busvoltage_ina226 = busvoltageBuffer_ina226/countNum;
              double finalValue = (busvoltage_ina226 + (shuntvoltage_ina226))*1000;
              Serial.printf("%.2f\n", finalValue);
              digitalWrite(activePin, LOW);

              double finalValue_after = (-log10(finalValue/Machine_Ctrl.lastLightValue)-bValue)/mValue * dilution;
              if (finalValue_after < 0) {
                finalValue_after = 0;
              }

              //? websocket相關的數值要限縮在小數點下第2位
              // (*Machine_Ctrl.JSON__sensorDataSave)[poolChose][value_name].set(String(finalValue,2).toDouble());
              // (*Machine_Ctrl.JSON__sensorDataSave)[poolChose][TargetType].set(String(finalValue_after,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["Value"].set(String(finalValue,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][value_name]["data_time"].set(Machine_Ctrl.GetDatetimeString());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][TargetType]["Value"].set(String(finalValue_after,2).toDouble());
              (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"][TargetType]["data_time"].set(Machine_Ctrl.GetDatetimeString());

              sprintf(
                logBuffer, 
                "測量結果: %s, 初始強度 %s, 當前樣本數值: %s, 轉換後PPM: %s",
                TargetType.c_str(), String(Machine_Ctrl.lastLightValue, 2).c_str(), String(finalValue, 2).c_str(), String(finalValue_after, 2).c_str()
              );
              ESP_LOGI("LOADED_ACTION","       - %s", String(logBuffer).c_str());
              Machine_Ctrl.SetLog(3, "測量PPM數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);

              String DataFileFullPath = Machine_Ctrl.SensorDataFolder + Machine_Ctrl.GetDateString("") + "_data.csv";
              Machine_Ctrl.SaveSensorData_photometer(
                DataFileFullPath,Machine_Ctrl.GetDatetimeString() ,spectrophotometerTitle, poolChose, "-", "-",
                value_name, dilution, finalValue, finalValue_after
              );
              Machine_Ctrl.ReWriteLastDataSaveFile(Machine_Ctrl.FilePath__SD__LastSensorDataSave, (*Machine_Ctrl.JSON__sensorDataSave).as<JsonObject>());

              Machine_Ctrl.InsertNewDataToDB(
                Machine_Ctrl.GetDatetimeString(),
                spectrophotometerTitle, 
                poolChose,
                value_name,
                finalValue,
                Machine_Ctrl.TaskUUID
              );
              Machine_Ctrl.InsertNewDataToDB(
                Machine_Ctrl.GetDatetimeString(),
                spectrophotometerTitle, 
                poolChose,
                TargetType,
                finalValue_after,
                Machine_Ctrl.TaskUUID
              );


              if (finalValue < min or finalValue > max) {
                sprintf(
                  logBuffer, 
                  "測量光度述職時發現錯誤，獲得數值: %s, 不再設定範圍內 %s - %s",
                  String(finalValue, 2).c_str(), String(min, 2).c_str(), String(max, 2).c_str()
                );
                ESP_LOGI("LOADED_ACTION","       - %s", String(logBuffer).c_str());
                Machine_Ctrl.SetLog(1, "測量PPM數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);

                if (failAction == "stepStop") {
                  //? 當前step流程中止
                  (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("FAIL");
                  Machine_Ctrl.pipelineTaskHandleMap.erase(stepsGroupNameString);
                  free(stepsGroupName);
                  *TaskInfoItem.TaskThreadFree = true;
                  vTaskDelete(NULL);
                }
                else if (failAction == "stopImmediately") {
                  Machine_Ctrl.StopDeviceAndINIT();
                }
                isStepFail = true;
              }
            }
          }
        }
      }
      else if (eventItem.containsKey("ph_meter")) {
        // Serial.println("ph_meter");
        //TODO 目前因為水質機只會有一個PH計，因此先以寫死的方式來做
        ESP_LOGI("LOADED_ACTION","      PH計控制");
        for (JsonObject PHmeterItem : eventItem["ph_meter"].as<JsonArray>()) {
          pinMode(15, INPUT);
          pinMode(7, OUTPUT);
          digitalWrite(7, HIGH);
          vTaskDelay(1000/portTICK_PERIOD_MS);
          String poolChose = PHmeterItem["pool"].as<String>();
          uint16_t phValue[30];
          for (int i=0;i<30;i++) {
            phValue[i] = analogRead(15);
          }
          //* 原始電壓數值獲得
          double PH_RowValue = afterFilterValue(phValue, 30);
          double m = (*Machine_Ctrl.JSON__PHmeterConfig)[0]["calibration"][0]["ret"]["m"].as<String>().toDouble();
          double b = (*Machine_Ctrl.JSON__PHmeterConfig)[0]["calibration"][0]["ret"]["b"].as<String>().toDouble();
          double pHValue = m*PH_RowValue + b;
          if (pHValue<0) {
            Machine_Ctrl.SetLog(2, "測量PH數值", "數值:"+String(pHValue,1)+"過低");
            pHValue = 0.;
          } else if (pHValue > 14) {
            Machine_Ctrl.SetLog(2, "測量PH數值", "數值:"+String(pHValue,1)+"過高");
            pHValue = 14.;
          }
          //? websocket相關的數值要限縮在小數點下第2位
          (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"]["pH_volt"]["Value"].set(String(PH_RowValue,2).toDouble());
          (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"]["pH_volt"]["data_time"].set(Machine_Ctrl.GetDatetimeString());
          (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"]["pH"]["Value"].set(String(pHValue,2).toDouble());
          (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["DataItem"]["pH"]["data_time"].set(Machine_Ctrl.GetDatetimeString());
          // (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["pH_volt"].set(String(PH_RowValue,2).toDouble());
          // (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["pH"].set(String(pHValue,2).toDouble());
          digitalWrite(7, LOW);
          Machine_Ctrl.ReWriteLastDataSaveFile(Machine_Ctrl.FilePath__SD__LastSensorDataSave, (*Machine_Ctrl.JSON__sensorDataSave).as<JsonObject>());
          String DataFileFullPath = Machine_Ctrl.SensorDataFolder + Machine_Ctrl.GetDateString("") + "_data.csv";
          Machine_Ctrl.SaveSensorData_photometer(
            DataFileFullPath,Machine_Ctrl.GetDatetimeString() ,"PH測量", poolChose, "", "",
            "phValue", -1, PH_RowValue, pHValue
          );
          Machine_Ctrl.InsertNewDataToDB(
            Machine_Ctrl.GetDatetimeString(),
            "PH測量", 
            poolChose,
            "pH_volt",
            PH_RowValue,
            Machine_Ctrl.TaskUUID
          );
          Serial.println(3);
          Machine_Ctrl.InsertNewDataToDB(
            Machine_Ctrl.GetDatetimeString(),
            "PH測量", 
            poolChose,
            "pH",
            pHValue,
            Machine_Ctrl.TaskUUID
          );

          Serial.println(14);
          char logBuffer[1000];
          sprintf(
            logBuffer, 
            "PH量測結果, 測量原始值: %s, 轉換後PH值: %s",
            String(PH_RowValue, 2).c_str(), String(pHValue, 2).c_str()
          );
          ESP_LOGI("LOADED_ACTION","       - %s", String(logBuffer).c_str());
          Machine_Ctrl.SetLog(5, "測量PH數值", String(logBuffer), Machine_Ctrl.BackendServer.ws_);
        }

      }
      else if (eventItem.containsKey("wait")) {
        int waitSeconds = eventItem["wait"].as<int>();
        ESP_LOGI("LOADED_ACTION","      等待 %d 秒",waitSeconds);
        vTaskDelay(waitSeconds*1000/portTICK_PERIOD_MS);
      }
      else if (eventItem.containsKey("upload")) {
        serializeJsonPretty(eventItem, Serial);
        String poolChose = eventItem["upload"][0]["pool"].as<String>();
        ESP_LOGI("LOADED_ACTION","      更新 %s 資料時間",poolChose.c_str());
        (*Machine_Ctrl.JSON__sensorDataSave)[poolChose]["Data_datetime"].set(Machine_Ctrl.GetDatetimeString());
        Machine_Ctrl.BroadcastNewPoolData(poolChose);
      }
      //! log發出
      else if (eventItem.containsKey("log")) {
        JsonObject logAction = eventItem["log"].as<JsonObject>();
        int logLevel = logAction["level"].as<int>();
        String logTitle = logAction["title"].as<String>() ;
        String logDesp  = logAction["desp"].as<String>();
        Machine_Ctrl.SetLog(
          logLevel, logTitle, logDesp, Machine_Ctrl.BackendServer.ws_, NULL
        );
      }
    }
    vTaskDelay(10);
  }

  if (isStepFail) {
    (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("FAIL");
  }
  else {
    (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupNameString]["RESULT"].set("SUCCESS");
  }
  Machine_Ctrl.pipelineTaskHandleMap.erase(stepsGroupNameString);
  free(stepsGroupName);
  *TaskInfoItem.TaskThreadFree = true;
  vTaskDelete(NULL);
}


//? 建立新的流程Task
void SMachine_Ctrl::AddNewPiplelineFlowTask(String stepsGroupName)
{
  if ((*JSON__pipelineConfig)["steps_group"].containsKey(stepsGroupName)) {
    ESP_LOGI("","RUN: %s", stepsGroupName.c_str());
    TaskInfo* ThisTaskInfo = new TaskInfo();
    int nameLength = stepsGroupName.length();
    (*ThisTaskInfo).stepsGroupName = (char*)malloc((nameLength + 1) * sizeof(char));
    strcpy((*ThisTaskInfo).stepsGroupName, stepsGroupName.c_str());

    (*JSON__pipelineConfig)["steps_group"][stepsGroupName]["RESULT"].set("RUNNING");

    if (TaskThread_Free_1) {
      TaskThread_Free_1 = false;
      Serial.println("使用第一");
      (*ThisTaskInfo).TaskThreadFree = &TaskThread_Free_1;
      (*ThisTaskInfo).TaskThreadStackBuffer = TaskThread_xStack_1;
      (*ThisTaskInfo).TaskThreadTaskBuffer = &TaskThread_xTaskBuffer_1;
    }
    else if (TaskThread_Free_2) {
      TaskThread_Free_2 = false;
      Serial.println("使用第2");
      (*ThisTaskInfo).TaskThreadFree = &TaskThread_Free_2;
      (*ThisTaskInfo).TaskThreadStackBuffer = TaskThread_xStack_2;
      (*ThisTaskInfo).TaskThreadTaskBuffer = &TaskThread_xTaskBuffer_2;
    }
    else if (TaskThread_Free_3) {
      TaskThread_Free_3 = false;
      Serial.println("使用第3");
      (*ThisTaskInfo).TaskThreadFree = &TaskThread_Free_3;
      (*ThisTaskInfo).TaskThreadStackBuffer = TaskThread_xStack_3;
      (*ThisTaskInfo).TaskThreadTaskBuffer = &TaskThread_xTaskBuffer_3;
    }
    else if (TaskThread_Free_4) {
      TaskThread_Free_4 = false;
      Serial.println("使用第4");
      (*ThisTaskInfo).TaskThreadFree = &TaskThread_Free_4;
      (*ThisTaskInfo).TaskThreadStackBuffer = TaskThread_xStack_4;
      (*ThisTaskInfo).TaskThreadTaskBuffer = &TaskThread_xTaskBuffer_4;
    }
    else if (TaskThread_Free_5) {
      TaskThread_Free_5 = false;
      Serial.println("使用第5");
      (*ThisTaskInfo).TaskThreadFree = &TaskThread_Free_5;
      (*ThisTaskInfo).TaskThreadStackBuffer = TaskThread_xStack_5;
      (*ThisTaskInfo).TaskThreadTaskBuffer = &TaskThread_xTaskBuffer_5;
    }
    else if (TaskThread_Free_6) {
      TaskThread_Free_6 = false;
      Serial.println("使用第6");
      (*ThisTaskInfo).TaskThreadFree = &TaskThread_Free_6;
      (*ThisTaskInfo).TaskThreadStackBuffer = TaskThread_xStack_6;
      (*ThisTaskInfo).TaskThreadTaskBuffer = &TaskThread_xTaskBuffer_6;
    }
    pipelineTaskHandleMap[stepsGroupName] = xTaskCreateStatic(PiplelineFlowTask, NULL, 15000, (void*)ThisTaskInfo, 3, (*ThisTaskInfo).TaskThreadStackBuffer, (*ThisTaskInfo).TaskThreadTaskBuffer);
  }
  else {
    ESP_LOGE("", "設定中找不到名為: %s 的 steps group", stepsGroupName.c_str());
  }
}

//? 清空以下Task
//? 1. TASK__pipelineFlowScan
//? 2. pipelineTaskHandleMap 裡面所有的TASK
void SMachine_Ctrl::CleanAllStepTask()
{
  if (TASK__pipelineFlowScan != NULL) {
    vTaskSuspend(TASK__pipelineFlowScan);
    vTaskDelete(TASK__pipelineFlowScan);
    TASK__pipelineFlowScan = NULL;
  }
  for (const auto& pipelineTaskHandle : pipelineTaskHandleMap) {
    TaskHandle_t taskChose = pipelineTaskHandle.second;
    String stepName = pipelineTaskHandle.first;
    vTaskSuspend(taskChose);
    vTaskDelete(taskChose);
    pipelineTaskHandleMap.erase(stepName);
  }
  TaskThread_Free_1 = true;
  TaskThread_Free_2 = true;
  TaskThread_Free_3 = true;
  TaskThread_Free_4 = true;
  TaskThread_Free_5 = true;
  TaskThread_Free_6 = true;
}

//? 負責檢查Pipeline目前進度，以及判斷是否要執行
//! 這個流程在確定執行完畢後，要負責釋放互斥鎖: LOAD__ACTION_V2_xMutex
void PipelineFlowScan(void* parameter)
{ 
  //? pipelineStackList: 多流程設定的列隊

  ESP_LOGD("","開始執行流程管理Task");

  // DynamicJsonDocument pipelineStackList = *((DynamicJsonDocument*)parameter);
  //! 注意: Task中的參數傳遞請不要用 *((DynamicJsonDocument*)parameter) 這種方式建立
  //! 要使用指針的方式 (DynamicJsonDocument*)parameter
  //! 不然等Task刪除後，記憶體不會釋放
  DynamicJsonDocument* pipelineStackList = (DynamicJsonDocument*)parameter;
  // ESP_LOGD("", "記憶體Check: %d", xPortGetFreeHeapSize());

  ESP_LOGD("","一共有 %d 個流程會依序執行", (*pipelineStackList).size());
  for (int pipelineIndex = 0;pipelineIndex<(*pipelineStackList).size();pipelineIndex++) {
    ESP_LOGI("", "開始執行第 %d 個流程", pipelineIndex+1);
    JsonObject pipelineChose = (*pipelineStackList)[pipelineIndex].as<JsonObject>();
    String pipelineConfigFileFullPath = pipelineChose["FullFilePath"].as<String>();
    //STEP 1 檢查檔案是否存在
    ESP_LOGI("", "STEP 1 檢查檔案是否存在: %s", pipelineConfigFileFullPath.c_str());
    if (!SD.exists(pipelineConfigFileFullPath)) {
      ESP_LOGE("", "檔案: %s 不存在,跳至下一流程", pipelineConfigFileFullPath.c_str());
      Machine_Ctrl.SetLog(1, "檔案不存在，跳至下一流程", pipelineConfigFileFullPath, Machine_Ctrl.BackendServer.ws_);
      continue;
    }
    //STEP 2 檢查檔案是否可以被讀取
    ESP_LOGI("", "STEP 2 檢查檔案是否可以被讀取");
    File pipelineConfigFile = SD.open(pipelineConfigFileFullPath);
    if (!pipelineConfigFile) {
      ESP_LOGE("", "無法打開檔案: %s ,跳至下一流程", pipelineConfigFileFullPath.c_str());
      Machine_Ctrl.SetLog(1, "無法打開檔案，跳至下一流程", pipelineConfigFileFullPath, Machine_Ctrl.BackendServer.ws_);
      continue;
    }
    //STEP 3 檢查檔案是否可以被解析成JSON格式
    ESP_LOGI("", "STEP 3 檢查檔案是否可以被解析成JSON格式");
    (*Machine_Ctrl.JSON__pipelineConfig).clear();
    DeserializationError error = deserializeJson(*Machine_Ctrl.JSON__pipelineConfig, pipelineConfigFile,  DeserializationOption::NestingLimit(20));
    pipelineConfigFile.close();
    if (error) {
      ESP_LOGE("LOAD__ACTION_V2", "JOSN解析失敗: %s ,跳至下一流程", error.c_str());
      Machine_Ctrl.SetLog(1, "JOSN解析失敗, 跳至下一流程", String(error.c_str()), Machine_Ctrl.BackendServer.ws_);
      continue;
    }
    //STEP 4 整理設定檔內容，以利後續使用
    ESP_LOGI("", "STEP 4 整理設定檔內容，以利後續使用");
    /**
    //* 範例
    //* [["NO3WashValueTest",["NO3ValueTest","NO3TestWarning"]],["NO3ValueTest",["NH4WashValueTest"]],["NH4WashValueTest",["NH4Test","NH4TestWarning"]]]
    //* 轉換為
    //* {
    //*   "NO3WashValueTest": {
    //*    "Name": "NO3WashValueTest",
    //*     "childList": {"NO3ValueTest": {},"NO3TestWarning": {}},
    //*     "parentList": {}
    //*   },
    //*   "NO3ValueTest": {
    //*     "Name": "NO3ValueTest",
    //*     "childList": {"NH4WashValueTest": {}},
    //*     "parentList": {"NO3WashValueTest": {}}
    //*   },
    //*   "NO3TestWarning": {
    //*     "Name": "NO3TestWarning",
    //*     "childList": {},
    //*     "parentList": {"NO3WashValueTest": {}}
    //*   },
    //*   "NH4WashValueTest": {
    //*     "Name": "NH4WashValueTest",
    //*     "childList": {"NH4Test": {},"NH4TestWarning": {}},
    //*     "parentList": {"NO3ValueTest": {}}
    //*   },
    //*   "NH4Test": {
    //*     "Name": "NH4Test","childList": {},
    //*     "parentList": {"NH4WashValueTest": {}}
    //*   },
    //*   "NH4TestWarning": {
    //*     "Name": "NH4TestWarning",
    //*     "childList": {},
    //*     "parentList": {"NH4WashValueTest": {}}
    //*   }
    //* }
     */
    String onlyStepGroup = pipelineChose["stepChose"].as<String>();
    DynamicJsonDocument piplineSave(65525);
    //? 若不指定Step
    if (onlyStepGroup == "") {
      ESP_LOGD("", "無指定Step，將會執行完整的流程內容");
      JsonArray piplineArray = (*Machine_Ctrl.JSON__pipelineConfig)["pipline"].as<JsonArray>();
      for (JsonArray singlePiplineArray : piplineArray) {
        String ThisNodeNameString = singlePiplineArray[0].as<String>();
        if (!piplineSave.containsKey(ThisNodeNameString)) {
          piplineSave[ThisNodeNameString]["Name"] = ThisNodeNameString;
          piplineSave[ThisNodeNameString].createNestedObject("childList");
          piplineSave[ThisNodeNameString].createNestedObject("parentList");
        }
        for (String piplineChildName :  singlePiplineArray[1].as<JsonArray>()) {
          if (!piplineSave.containsKey(piplineChildName)) {
            piplineSave[piplineChildName]["Name"] = piplineChildName;
            piplineSave[piplineChildName].createNestedObject("childList");
            piplineSave[piplineChildName].createNestedObject("parentList");
          }
          if (!piplineSave[ThisNodeNameString]["childList"].containsKey(piplineChildName)){
            piplineSave[ThisNodeNameString]["childList"].createNestedObject(piplineChildName);
          }

          if (!piplineSave[piplineChildName]["parentList"].containsKey(ThisNodeNameString)){
            piplineSave[piplineChildName]["parentList"].createNestedObject(ThisNodeNameString);
          }
        }
      }
      (*Machine_Ctrl.JSON__pipelineConfig)["pipline"].set(piplineSave);
      //? 將 steps_group 內的資料多加上key值: RESULT 來讓後續Task可以判斷流程是否正確執行
      //? 如果沒有"trigger"這個key值，則預設task觸發條件為"allDone"
      JsonObject stepsGroup = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"].as<JsonObject>();
      for (JsonPair stepsGroupItem : stepsGroup) {
        String stepsGroupName = String(stepsGroupItem.key().c_str());
        //? 如果有"same"這個key值，則 steps 要繼承其他設定內容
        if ((*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName].containsKey("same")) {
          (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["steps"].set(
            (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][
              (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["same"].as<String>()
            ]["steps"].as<JsonArray>()
          );
        }
        (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["RESULT"].set("WAIT");
        if (!(*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName].containsKey("trigger")) {
          (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["trigger"].set("allDone");
        }
      };

      if (CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL==5) {
        String piplineSaveString = "";
        serializeJson((*Machine_Ctrl.JSON__pipelineConfig)["pipline"], piplineSaveString);
        ESP_LOGV("", "流程設定檔: %s", piplineSaveString);
      }
    }
    else {
      ESP_LOGI("", "指定執行Step: %s", onlyStepGroup.c_str());
      JsonObject piplineArray = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"].as<JsonObject>();
      if (piplineArray.containsKey(onlyStepGroup)) {
        piplineSave[onlyStepGroup]["Name"] = onlyStepGroup;
        piplineSave[onlyStepGroup].createNestedObject("childList");
        piplineSave[onlyStepGroup].createNestedObject("parentList");
        (*Machine_Ctrl.JSON__pipelineConfig)["pipline"].set(piplineSave);
        (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][onlyStepGroup]["RESULT"].set("WAIT");
        if ((*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][onlyStepGroup].containsKey("same")) {
          (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][onlyStepGroup]["steps"].set(
            (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][
              (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][onlyStepGroup]["same"].as<String>()
            ]["steps"].as<JsonArray>()
          );
        }
        String onlyEvent = pipelineChose["eventChose"].as<String>();
        if (onlyEvent != "") {
          //? 若有指定Event執行
          //? 防呆: 檢查此event是否存在
          if (!(*Machine_Ctrl.JSON__pipelineConfig)["events"].containsKey(onlyEvent)) {
            //! 指定的event不存在
            ESP_LOGE("LOAD__ACTION_V2", "設定檔 %s 中找不到Event: %s ，終止流程執行", pipelineConfigFileFullPath.c_str(), onlyEvent.c_str());
            Machine_Ctrl.SetLog(1, "找不到事件: " + onlyEvent + "，終止流程執行", "設定檔名稱: "+pipelineConfigFileFullPath, Machine_Ctrl.BackendServer.ws_);
            continue;
          }
          (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][onlyStepGroup]["steps"].clear();
          (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][onlyStepGroup]["steps"].add(onlyEvent);
          int onlyIndex = pipelineChose["eventIndexChose"].as<int>();
          if (onlyIndex != -1) {
            //? 若有指定事件中的步驟執行
            //? 防呆: 檢查此步驟是否存在
            if (onlyIndex >= (*Machine_Ctrl.JSON__pipelineConfig)["events"][onlyEvent]["event"].size()) {
              //! 指定的index不存在
              ESP_LOGE("LOAD__ACTION_V2", "設定檔 %s 中的Event: %s 並無步驟: %d，終止流程執行", pipelineConfigFileFullPath.c_str(), onlyEvent.c_str(), onlyIndex);
              Machine_Ctrl.SetLog(1, "事件: " + onlyEvent + "找不到步驟: " + String(onlyIndex) + "，終止流程執行", "設定檔名稱: "+pipelineConfigFileFullPath, Machine_Ctrl.BackendServer.ws_);
              continue;
            }
            DynamicJsonDocument newEventIndexArray(200);
            JsonArray array = newEventIndexArray.to<JsonArray>();
            array.add(
              (*Machine_Ctrl.JSON__pipelineConfig)["events"][onlyEvent]["event"][onlyIndex]
            );
            (*Machine_Ctrl.JSON__pipelineConfig)["events"][onlyEvent]["event"].set(array);
          }
        }
      }
    }
    
    //STEP 5 開始執行流程判斷功能
    ESP_LOGI("", "STEP 5 開始執行流程判斷功能");
    uuid.generate();
    Machine_Ctrl.TaskUUID = String(uuid.toCharArray());
    JsonObject stepsGroup = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"].as<JsonObject>();
    JsonObject pipelineSet = (*Machine_Ctrl.JSON__pipelineConfig)["pipline"].as<JsonObject>();
    //? isAllDone: 用來判斷是否所有流程都運行完畢，如果都完畢，則此TASK也可以關閉
    //? 判斷完畢的邏輯: 全部step都不為 "WAIT"、"RUNNING" 則代表完畢
    bool isAllDone = false;
    while(isAllDone == false) {
      isAllDone = true;
      for (JsonPair stepsGroupItem : pipelineSet) {
        //? stepsGroupName: 流程ID
        String stepsGroupName = String(stepsGroupItem.key().c_str());
        //? stepsGroupTitle: 流程名稱
        String stepsGroupTitle = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["title"].as<String>();
        //? stepsGroupResult: 此流程的運行狀態
        String stepsGroupResult = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["RESULT"].as<String>();
        //? 只有在"WAIT"時，才有必要判斷是否要執行
        if (stepsGroupResult == "WAIT") {
          isAllDone = false;
          //? parentList: 此流程的parentList
          JsonObject parentList = (*Machine_Ctrl.JSON__pipelineConfig)["pipline"][stepsGroupName]["parentList"].as<JsonObject>();
          //? 如果此step狀態是WAIT並且parent數量為0，則直接觸發
          if (parentList.size() == 0) {
            ESP_LOGI("", "發現初始流程: %s(%s)，準備執行", stepsGroupTitle.c_str(), stepsGroupName.c_str());
            Machine_Ctrl.AddNewPiplelineFlowTask(stepsGroupName);
            continue;
          }

          //? 程式執行到這邊，代表都不是ENTEY POINT
          //? TrigerRule: 此流程的觸發條件
          String TrigerRule = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["trigger"].as<String>();
          //? 如果觸發條件是 allDone，則會等待parents都不為 "WAIT"、"RUNNING"、"NORUN" 時則會開始執行
          //? 而若任何一個parent為 "NORUN" 時，則本step則視為不再需要執行，立刻設定為 "NORUN"
          if (TrigerRule == "allDone") {
            bool stepRun = true;
            for (JsonPair parentItem : parentList ) {
              String parentName = String(parentItem.key().c_str());
              String parentResult = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][parentName]["RESULT"].as<String>();
              if (parentResult=="WAIT" or parentResult=="RUNNING") {
                //? 有任何一個parent還沒執行完畢，跳出
                stepRun = false;
                break; 
              } else if ( parentResult=="NORUN") {
                //? 有任何一個parent不需執行，此step也不再需執行
                ESP_LOGW("", "Task %s 不符合 allDone 的執行條件，關閉此Task", stepsGroupName.c_str());
                (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["RESULT"].set("NORUN");
                stepRun = false;
                break; 
              }
            }
            if (stepRun) {
              ESP_LOGI("", "流程: %s(%s) 符合觸發條件: allDone，準備執行", stepsGroupTitle.c_str(), stepsGroupName.c_str());
              Machine_Ctrl.AddNewPiplelineFlowTask(stepsGroupName);
              continue;
            }
          }
          //? 如果觸發條件是 oneFail，則會等待parent任何一項的RESULT變成 "FAIL" 就執行
          //? 而若所有parent都不是 "FAIL" 並且不為 "WAIT"、"RUNNING" 時，則本Step則視為不再需要執行，立刻設定為 "NORUN"
          else if (TrigerRule == "oneFail") {
            bool setNoRun = true;
            for (JsonPair parentItem : parentList ) {
              String parentName = String(parentItem.key().c_str());
              String parentResult = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][parentName]["RESULT"].as<String>();
              if (parentResult=="FAIL") {
                ESP_LOGI("", "流程: %s(%s) 符合觸發條件: oneFail，準備執行", stepsGroupTitle.c_str(), stepsGroupName.c_str());
                Machine_Ctrl.AddNewPiplelineFlowTask(stepsGroupName);
                setNoRun = false;
                break;
              } else if (parentResult=="WAIT" or parentResult=="RUNNING") {
                setNoRun = false;
                break;
              }
            }
            if (setNoRun) {
              ESP_LOGW("", "Task %s(%s) 不符合 oneFail 的執行條件，關閉此Task", stepsGroupTitle.c_str(), stepsGroupName.c_str());
              (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["RESULT"].set("NORUN");
            }
          }
          //? 如果觸發條件是 allSuccess，則會等待所有parent的RESULT變成 "SUCCESS" 就執行
          //? 如果任何一個parent為 "WAIT"、"RUNNING"，則繼續等待
          //? 如果任何一個parent為 "NORUN"、、"FAIL" 時，則本Task則視為不再需要執行，立刻設定為 "NORUN"，並且刪除Task
          else if (TrigerRule == "allSuccess") {
            bool stepRun = true;
            for (JsonPair parentItem : parentList ) {
              String parentName = String(parentItem.key().c_str());
              String parentResult = (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][parentName]["RESULT"].as<String>();
              if (parentResult=="NORUN" or parentResult=="FAIL") {
                ESP_LOGW("", "Task %s(%s) 不符合 allSuccess 的執行條件，關閉此Task", stepsGroupTitle.c_str(), stepsGroupName.c_str());
                (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepsGroupName]["RESULT"].set("NORUN");
                stepRun = false;
                break;
              }
              else if (parentResult=="WAIT" or parentResult=="RUNNING") {
                stepRun = false;
                break;
              }
            }
            if (stepRun) {
              ESP_LOGI("", "流程: %s(%s) 符合觸發條件: allSuccess，準備執行", stepsGroupTitle.c_str(), stepsGroupName.c_str());
              Machine_Ctrl.AddNewPiplelineFlowTask(stepsGroupName);
              continue;
            }
          }
        } 
        else if (stepsGroupResult == "RUNNING") {isAllDone = false;}
        else if (stepsGroupResult == "STOP_THIS_PIPELINE") {
          ESP_LOGI("", "發現有流程觸發了 STOP_THIS_PIPELINE");
          ESP_LOGI("", " - 流程名稱:\t%s (%s)", stepsGroupTitle.c_str(), stepsGroupName.c_str());
          ESP_LOGI("", "準備停止當前正在執行的各項Task");
          for (const auto& pipelineTaskHandle : Machine_Ctrl.pipelineTaskHandleMap) {
            String stepName = pipelineTaskHandle.first;
            ESP_LOGI("", "停止流程: %s (%s)", (*Machine_Ctrl.JSON__pipelineConfig)["steps_group"][stepName]["title"].as<String>().c_str(), stepName.c_str());
            TaskHandle_t taskChose = pipelineTaskHandle.second;
            vTaskSuspend(taskChose);
            vTaskDelete(taskChose);
            Machine_Ctrl.pipelineTaskHandleMap.erase(stepName);
          }
          Machine_Ctrl.peristalticMotorsCtrl.SetAllMotorStop();
          Machine_Ctrl.MULTI_LTR_329ALS_01_Ctrler.closeAllSensor();
          isAllDone = true;
          break;
        }
      };
      vTaskDelay(100/portTICK_PERIOD_MS);
    }
    ESP_LOGI("", "第 %d 個流程執行完畢", pipelineIndex+1);
  }
  
  Machine_Ctrl.TASK__pipelineFlowScan = NULL;
  xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex); //! 釋放互斥鎖!!
  ESP_LOGI("", "所有流程已執行完畢");
  vTaskDelay(100/portTICK_PERIOD_MS);
  Machine_Ctrl.SetLog(
    3, "所有流程已執行完畢", (*Machine_Ctrl.JSON__pipelineConfig)["Title"].as<String>(), Machine_Ctrl.BackendServer.ws_, NULL
  );
  ESP_LOGD("", "剩餘記憶體: %d", xPortGetFreeHeapSize());
  vTaskDelete(NULL);
}

//? 建立Pipeline排程的控制Task
void SMachine_Ctrl::CreatePipelineFlowScanTask(DynamicJsonDocument *pipelineStackList) {
  ESP_LOGD("", "準備建立流程管理Task");
  //!! 防呆，在此funtion內，互斥鎖: LOAD__ACTION_V2_xMutex 應該都是上鎖的狀態
  //!! 若判定執行失敗，則立刻釋放互斥鎖
  if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdTRUE) {
    ESP_LOGE("LOAD__ACTION_V2", "發現不合理的流程觸發，請排查code是否錯誤");
    SetLog(1, "發現不合理的流程觸發，終止流程執行", "請通知工程師排除");
    xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
  }
  else {
    if (TASK__pipelineFlowScan == NULL) {
      // xTaskCreate(
      //   PipelineFlowScan, "PipelineScan",
      //   20000, (void*)pipelineStackList, 4, &TASK__pipelineFlowScan
      // );
      TASK__pipelineFlowScan = xTaskCreateStatic(
        PipelineFlowScan, "PipelineScan", 20000,(void*)pipelineStackList, 4,
        PipelineFlowScanTask_xStack, &PipelineFlowScanTask_xTaskBuffer
      );
    }
    else {
      ESP_LOGE("LOAD__ACTION_V2", "發現不合理的流程觸發，TASK__pipelineFlowScan不為NULL，請排查code是否錯誤");
      SetLog(1, "發現不合理的流程觸發，TASK__pipelineFlowScan不為NULL，終止流程執行", "請通知工程師排除");
      xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    }
  }
}

////////////////////////////////////////////////////
// For 事件執行
////////////////////////////////////////////////////
// SemaphoreHandle_t LOAD__ACTION_V2_xMutex = NULL;
// vSemaphoreCreateBinary(LOAD__ACTION_V2_xMutex);
// SemaphoreHandle_t LOAD__ACTION_V2_xMutex = vSemaphoreCreateBinary();

bool SMachine_Ctrl::LOAD__ACTION_V2(DynamicJsonDocument *pipelineStackList)
{
  ESP_LOGD("", "收到執行排程的需求");

  if (xPortGetFreeHeapSize() < 30000) {
    SetLog(1, "記憶體數量不足，強制重開機", "記憶體數量不足，強制重開機");
    ESP_LOGE("", "記憶體數量不足，強制重開機");
    ESP.restart();
  }



  //!! 防呆，在此funtion內，互斥鎖: LOAD__ACTION_V2_xMutex 應該都是上鎖的狀態
  //!! 若判定執行失敗，則立刻釋放互斥鎖
  if (xSemaphoreTake(Machine_Ctrl.LOAD__ACTION_V2_xMutex, 0) == pdTRUE) {
    ESP_LOGE("LOAD__ACTION_V2", "發現不合理的流程觸發，請排查code是否錯誤");
    SetLog(1, "發現不合理的流程觸發，終止流程執行", "請通知工程師排除");
    xSemaphoreGive(Machine_Ctrl.LOAD__ACTION_V2_xMutex);
    return false;
  }
  if (CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL==5) {
    String pipelineStackListString;
    serializeJson(*pipelineStackList, pipelineStackListString);
    ESP_LOGD("", "詳細流程設定: %s", pipelineStackListString.c_str());
  }
  CreatePipelineFlowScanTask(pipelineStackList);
  return true;
}



typedef struct {
  String message;
} TaskParameters;



void wsSendStepStatus() 
{
  DynamicJsonDocument json_doc = Machine_Ctrl.BackendServer.GetBaseWSReturnData("RunHistory");
  json_doc["parameter"]["message"].set("update");
  String returnString;
  serializeJsonPretty(json_doc, returnString);
  Machine_Ctrl.BackendServer.ws_->binaryAll(returnString);
}


////////////////////////////////////////////////////
// For 互動相關
////////////////////////////////////////////////////

SemaphoreHandle_t SetLog_xMutex = xSemaphoreCreateMutex();

/**
 * @brief 
 * 
 * @param Level 0: DEBUG, 1: ERROR, 2: WARNING, 3: INFO, 4: DETAIL, 5: SUCCESS, 6: FAIL
 * @param Title 
 * @param description 
 * @param wsBroadcast 
 * @return DynamicJsonDocument 
 */
DynamicJsonDocument SMachine_Ctrl::SetLog(int Level, String Title, String description, AsyncWebSocket *server, AsyncWebSocketClient *client, bool Save)
{
  DynamicJsonDocument logItem(500);
  String timeString = GetDatetimeString();
  logItem["level"].set(Level);
  logItem["time"].set(timeString);
  logItem["title"].set(Title);
  logItem["desp"].set(description);
  // db_exec(DB_Log, "Select * from domain_rank where domain = 'zoho.com'");
  String SqlString = "INSERT INTO logs ( level, time, title, desp ) VALUES ( ";
  SqlString += String(Level);
  SqlString += " ,'";
  SqlString += GetDatetimeString();
  SqlString += "' ,'";
  SqlString += Title;
  SqlString += "','";
  SqlString += description;
  SqlString += "' );";
  db_exec(DB_Log, SqlString);

  // serializeJsonPretty(logItem, Serial);
  if (Save==true) {
    // vTaskDelay(10);
    if (xSemaphoreTake(SetLog_xMutex, portMAX_DELAY) == pdTRUE) {
      String logFileFullPath = LogFolder + GetDateString("") + "_log.csv";
      File logFile;
      if (SD.exists(logFileFullPath)) {
        logFile = SD.open(logFileFullPath, FILE_APPEND);
      } else {
        ExFile_CreateFile(SD, logFileFullPath);
        logFile = SD.open(logFileFullPath, FILE_APPEND);
        logFile.print("\xEF\xBB\xBF");
      }
      // Serial.println("Write");
      logFile.printf("%d,%s,%s,%s\n",
        Level, timeString.c_str(),
        Title.c_str(), description.c_str()
      );
      logFile.close();
      xSemaphoreGive(SetLog_xMutex);
      (*Machine_Ctrl.JSON__DeviceLogSave)["Log"].add(logItem);
      if ((*Machine_Ctrl.JSON__DeviceLogSave)["Log"].size() > 100) {
        (*Machine_Ctrl.JSON__DeviceLogSave)["Log"].remove(0);
      }
    }
  }

  if (server != NULL or client != NULL) {
    DynamicJsonDocument D_baseInfo = BackendServer.GetBaseWSReturnData("LOG");
    D_baseInfo["cmd"].set("log");
    D_baseInfo["status"].set("OK");
    D_baseInfo["action"]["target"].set("Log");
    switch (Level)
    {
      case 0:
        D_baseInfo["action"]["method"].set("DEBUG");
        break;
      case 1:
        D_baseInfo["action"]["method"].set("ERROR");
        D_baseInfo["action"]["status"].set("FAIL");
        break;
      case 2:
        D_baseInfo["action"]["method"].set("WARNING");
        break;
      case 3:
        D_baseInfo["action"]["method"].set("INFO");
        break;
      case 4:
        D_baseInfo["action"]["method"].set("DETAIL");
        break;
      case 5:
        D_baseInfo["action"]["method"].set("SUCCESS");
        break;
      case 6:
        D_baseInfo["action"]["method"].set("FAIL");
        D_baseInfo["action"]["status"].set("FAIL");
        break;
      default:
        D_baseInfo["action"]["method"].set("INFO");
        break;
    }
    D_baseInfo["action"]["message"].set(Title);
    D_baseInfo["action"]["desp"].set(description);
    D_baseInfo["action"]["status"].set("OK");
    String returnString;
    serializeJson(D_baseInfo, returnString);
    if (server != NULL) {
      server->binaryAll(returnString);
    }
    if (client != NULL) {
      client->binary(returnString);
    }
    D_baseInfo.clear();
  }
  return logItem;
}

//* 廣播指定池的感測器資料出去
//!! 注意，這個廣撥出去的資料是會進資料庫的
void SMachine_Ctrl::BroadcastNewPoolData(String poolID)
{
  DynamicJsonDocument D_baseInfo = BackendServer.GetBaseWSReturnData("");
  D_baseInfo["cmd"].set("SinglePoolData");
  D_baseInfo["action"]["message"].set("NewData");
  D_baseInfo["action"]["target"].set("SinglePoolData");
  D_baseInfo["action"]["method"].set("Update");
  D_baseInfo["action"]["status"].set("OK");
  D_baseInfo["parameter"][poolID].set((*JSON__sensorDataSave)[poolID]);
  String AfterSensorData;
  serializeJsonPretty(D_baseInfo, AfterSensorData);
  BackendServer.ws_->binaryAll(AfterSensorData);
}

////////////////////////////////////////////////////
// For 基礎行為
////////////////////////////////////////////////////

//? 獲得當前儀器時間字串
//? 
String SMachine_Ctrl::GetNowTimeString()
{
  char datetimeChar[30];
  time_t nowTime = now();
  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(nowTime), month(nowTime), day(nowTime),
    hour(nowTime), minute(nowTime), second(nowTime)
  );

  return String(datetimeChar);
}



String SMachine_Ctrl::GetDateString(String interval)
{
  time_t nowTime = now();
  char date[11];
  sprintf(date, "%04d%s%02d%s%02d",
    year(nowTime), interval.c_str(), month(nowTime), interval.c_str(), day(nowTime)
  );
  return String(date);
}

String SMachine_Ctrl::GetTimeString(String interval)
{
  time_t nowTime = now();
  char time_[11];
  sprintf(time_, "%02d%s%02d%s%02d",
    hour(nowTime), interval.c_str(), minute(nowTime), interval.c_str(), second(nowTime)
  );
  return String(time_);
}

String SMachine_Ctrl::GetDatetimeString(String interval_date, String interval_middle, String interval_time)
{
  return GetDateString(interval_date)+interval_middle+GetTimeString(interval_time);
}

void SMachine_Ctrl::PrintOnScreen(String content)
{
  WireOne.end();
  // Wire.end();
  U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, WireOne_SCL, WireOne_SDA);
  
  u8g2.begin();
  u8g2.clearDisplay();
  u8g2.firstPage();
  u8g2.setFont(u8g2_font_t0_11_tf);
  u8g2.setColorIndex(1);
  String line;
  do {
    int lineIndex = 0;
    int newlineIndex = content.indexOf('\n', lineIndex);
    int lineCount = 1;
    while (newlineIndex != -1) {
      line = content.substring(lineIndex, newlineIndex);
      lineIndex = newlineIndex + 1;
      u8g2.drawUTF8(0,8*lineCount,line.c_str());
      newlineIndex = content.indexOf('\n', lineIndex);
      lineCount++;
    }
    line = content.substring(lineIndex);
    lineIndex = newlineIndex + 1;
    u8g2.drawUTF8(0,8*lineCount,line.c_str());
    // u8g2.print(content.c_str());
  } while ( u8g2.nextPage() );
  Wire.end();
  INIT_I2C_Wires();
}

void SMachine_Ctrl::ShowIPAndQRCodeOnOled()
{
  //TODO 不知為何 U8G2一直不能用Wire1來運作，目前寫死要用oled時 要停止Wire1 先開Wire給oled用
  byte x0 = 3 + 64;
  byte y0 = 3;
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, ("http://" + BackendServer.IP).c_str());

  int ipList[4];
  String IpString = BackendServer.IP+"";
  String delimiter = ".";
  int delimiterIndex = IpString.indexOf(delimiter);
  int rowChose = 0;
  while (delimiterIndex != -1) {
    ipList[rowChose] =  IpString.substring(0, delimiterIndex).toInt();
    IpString = IpString.substring(delimiterIndex+1);
    delimiterIndex = IpString.indexOf(delimiter);
    rowChose++;
  }
  ipList[3] = IpString.toInt();
  WireOne.end();
  U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, Machine_Ctrl.WireOne_SCL, Machine_Ctrl.WireOne_SDA);
  u8g2.begin();
  u8g2.setFont(u8g2_font_HelvetiPixelOutline_te);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_VCR_OSD_tn);
    u8g2.setColorIndex(1); // 設成白色


    for (int i = 0; i < 3; i++) {
      u8g2.drawUTF8(0, 16 * (i + 1), (String(ipList[i]) + ".").c_str());
    }
    u8g2.drawUTF8(0, 16 * (3 + 1), (String(ipList[3])).c_str());

    u8g2.drawBox(65, 0, 66, 64);
    for (uint8_t y = 0; y < qrcode.size; y++)
    {
      for (uint8_t x = 0; x < qrcode.size; x++)
      {
        if (qrcode_getModule(&qrcode, x, y))
        {
          u8g2.setColorIndex(0);
        }
        else
        {
          u8g2.setColorIndex(1);
        }
        u8g2.drawBox(x0 + x * 2, y0 + y * 2, 2, 2);
      }
    }


  } while ( u8g2.nextPage() );
  
  Wire.end();
  INIT_I2C_Wires();
}


//! SD卡系統相關

void SMachine_Ctrl::SaveSensorData_photometer(
  String filePath, String dataTime, String title, String desp, String Gain, String Channel,
  String ValueName, double dilution, double result, double ppm
)
{
  File SaveFile;
  if (SD.exists(filePath) == false) {
    ExFile_CreateFile(SD, filePath);
    SaveFile = SD.open(filePath, FILE_APPEND);
    SaveFile.print("\xEF\xBB\xBF");
  } else {
    SaveFile = SD.open(filePath, FILE_APPEND);
  }
  SaveFile.printf(
    "%s, %s,%s,%s,%s,%s,%.2f,%.2f,%.2f\n",
    dataTime.c_str(), title.c_str(), desp.c_str(), Gain.c_str(), Channel.c_str(), ValueName.c_str(), dilution, result, ppm
  );
  SaveFile.close();
}


void SMachine_Ctrl::ReWriteLastDataSaveFile(String filePath, JsonObject tempData){
  ExFile_CreateFile(SD, filePath);
  File SaveFile = SD.open(filePath, FILE_WRITE);
  serializeJson(tempData, SaveFile);
  SaveFile.close();
}

SMachine_Ctrl Machine_Ctrl;
Adafruit_SH1106 display;