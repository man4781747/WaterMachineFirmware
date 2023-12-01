#ifndef MACHINE_CTRL_H
#define MACHINE_CTRL_H

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <vector>
#include <variant>

#include <Pools_Ctrl.h>
#include <Motor_Ctrl.h>
#include <SPIFFS_Ctrl.h>
#include <LTR_329ALS_01.h>

#include <TimeLib.h>   
#include <sqlite3.h>

#include <U8g2lib.h>
#include <Adafruit_GFX.h>
#include "Adafruit_SH1106.h"
#include "../lib/QRCode/src/qrcode.h"

#include "Wifi_Ctrl/src/Wifi_Ctrl.h"

/**
 * 儀器控制主體
*/
class SMachine_Ctrl
{
  public:
    SMachine_Ctrl(void){
      vSemaphoreCreateBinary(LOAD__ACTION_V2_xMutex);
      vSemaphoreCreateBinary(SQL_xMutex);
      vSemaphoreCreateBinary(LX_20S_xMutex);
      // sqlite3_initialize();
    };

    void INIT_I2C_Wires();
    bool INIT_PoolData();

    //? 緊急終止所有動作，並且回歸初始狀態
    void StopDeviceAndINIT();

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! SQL相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    int db_exec(sqlite3 *db, String sql, JsonDocument *jsonData);
    sqlite3 *DB_Log;
    String FilePath__SD__LogDB = "/sd/logDB.db";
    int openLogDB();
    sqlite3 *DB_Sensor;
    String FilePath__SD__SensorDB = "/sd/sensorDB.db";
    int openSensorDB();
    void InsertNewDataToDB(String time,String title, String pool, String VlaueName, double result, String HASH);
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! 流程設定相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    String TaskUUID;
    //? JSON__pipelineStack: 待執行的Step列表
    DynamicJsonDocument *JSON__pipelineStack = new DynamicJsonDocument(60000);
    //? JSON__pipelineConfig: 當前運行的Pipeline詳細設定
    DynamicJsonDocument *JSON__pipelineConfig = new DynamicJsonDocument(120000);
    //? pipelineTaskHandleMap: 記錄了Task專用的記憶體空間
    void INIT_TaskMemeryPoolItemMap();
    //? pipelineTaskHandleMap: 記錄了當前正在執行Step的Task，Key為Step名稱、Value為TaskHandle_t
    std::map<String, TaskHandle_t> pipelineTaskHandleMap;
    bool LOAD__ACTION_V2(DynamicJsonDocument *pipelineStackList);
    void AddNewPiplelineFlowTask(String stepsGroupName);
    void CleanAllStepTask();

    //? PipelineFlowScanTask專用記憶體空間
    StackType_t PipelineFlowScanTask_xStack[20000* sizeof(StackType_t)];
    StaticTask_t PipelineFlowScanTask_xTaskBuffer; 
  
    //? TaskThread專用記憶體空間
    //? 目前最多可以支援6個Task同時執行
    //TODO 目前使用比較白癡的寫法，待之後成功用 std::map與struct來放置資料後再改掉
    StackType_t TaskThread_xStack_1[15000* sizeof(StackType_t)];
    StaticTask_t TaskThread_xTaskBuffer_1; 
    bool TaskThread_Free_1 = true;
    StackType_t TaskThread_xStack_2[15000* sizeof(StackType_t)];
    StaticTask_t TaskThread_xTaskBuffer_2; 
    bool TaskThread_Free_2 = true;
    StackType_t TaskThread_xStack_3[15000* sizeof(StackType_t)];
    StaticTask_t TaskThread_xTaskBuffer_3; 
    bool TaskThread_Free_3 = true;
    StackType_t TaskThread_xStack_4[15000* sizeof(StackType_t)];
    StaticTask_t TaskThread_xTaskBuffer_4; 
    bool TaskThread_Free_4 = true;
    StackType_t TaskThread_xStack_5[15000* sizeof(StackType_t)];
    StaticTask_t TaskThread_xTaskBuffer_5; 
    bool TaskThread_Free_5 = true;
    StackType_t TaskThread_xStack_6[15000* sizeof(StackType_t)];
    StaticTask_t TaskThread_xTaskBuffer_6; 
    bool TaskThread_Free_6 = true;
    //TODO END

    void CreatePipelineFlowScanTask(DynamicJsonDocument *pipelineStackList);
    //? 排程狀態檢視Task，負責檢查排程進度、觸發排程執行
    TaskHandle_t TASK__pipelineFlowScan = NULL;
    //? LOAD__ACTION_V2_xMutex: 儀器是否忙碌一切依此 xMutex 決定
    SemaphoreHandle_t LOAD__ACTION_V2_xMutex = NULL;
    double lastLightValue;
    double lastLightValue_CH0 = 0;
    double lastLightValue_CH1 = 0;
    SemaphoreHandle_t LX_20S_xMutex = NULL;

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! OLED螢幕排程
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    void BuildOLEDCheckTask();

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! 儀器時鐘確認Task
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    void BuildTimeCheckTask();

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! 排程功能相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    //? 建立排程管理Task，負責定期檢查當前時間是否有排程需要執行
    void CreateScheduleManagerTask();
    TaskHandle_t TASK__ScheduleManager = NULL;

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! SPIFFS系統相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void INIT_SPIFFS_And_LoadConfigs();
    bool INIT_SPIFFS();
    String FilePath__SPIFFS__WiFiConfig = "/config/wifi_config.json";
    DynamicJsonDocument *JSON__WifiConfig = new DynamicJsonDocument(5000);
    String FilePath__SPIFFS__DeviceBaseInfo = "/config/device_base_config.json";
    DynamicJsonDocument *JSON__DeviceBaseInfo = new DynamicJsonDocument(1000);
    bool SPIFFS__LoadDeviceBaseInfo();
    bool SPIFFS__ReWriteDeviceBaseInfo();
    bool SPIFFS__LoadWiFiConfig();
    bool SPIFFS__ReWriteWiFiConfig();

    
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! SD卡系統相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    void INIT_SD_And_LoadConfigs();
    bool INIT_SD_Card();
    String FilePath__SD__DeviceConfig = "/config/device_config.json";
    DynamicJsonDocument *JSON__DeviceConfig = new DynamicJsonDocument(1000);
    String FilePath__SD__SpectrophotometerConfig = "/config/spectrophotometer_config.json";
    DynamicJsonDocument *JSON__SpectrophotometerConfig = new DynamicJsonDocument(4000);
    String FilePath__SD__PHmeterConfig = "/config/PHmeter_config.json";
    DynamicJsonDocument *JSON__PHmeterConfig = new DynamicJsonDocument(2000);
    String FilePath__SD__PoolConfig = "/config/pool_config.json";
    DynamicJsonDocument *JSON__PoolConfig = new DynamicJsonDocument(2000);
    String FilePath__SD__ScheduleConfig = "/config/schedule_config.json";
    DynamicJsonDocument *JSON__ScheduleConfig = new DynamicJsonDocument(10000);

    String FilePath__SD__PWNMotorConfig = "/config/pwm_motor_config.json";
    DynamicJsonDocument *JSON__PWNMotorConfig = new DynamicJsonDocument(10000);

    String FilePath__SD__PeristalticMotorConfig = "/config/peristaltic_motor_config.json";
    DynamicJsonDocument *JSON__PeristalticMotorConfig = new DynamicJsonDocument(10000);

    void SD__DeviceConfig();
    void SD__LoadspectrophotometerConfig();
    void SD__LoadPHmeterConfig();
    void SD__LoadPoolConfig();
    void SD__LoadPeristalticMotorConfig();
    void SD__LoadPWNMotorConfig();
    void SD__LoadScheduleConfig();
    void SD__ReWriteScheduleConfig();
    DynamicJsonDocument *JSON__PipelineConfigList = new DynamicJsonDocument(10000);
    //? 更新當前pipeline列表資料
    //? 以下情況需要觸發他: 1. 剛開機時、2. pipeline檔案有變動時
    void SD__UpdatePipelineConfigList();
    //? JSON__DeviceLogSave: 當前儀器Log的儲存
    DynamicJsonDocument *JSON__DeviceLogSave = new DynamicJsonDocument(50000);
    void SD__LoadOldLogs();
    String SD__configsFolder = "/config/";
    //? LOG 資料夾位置
    String LogFolder = "/logs/";

    //? 感測器資料儲存資料夾
    String SensorDataFolder = "/datas/";
    //? 最新資料儲存檔案位置
    //? API路徑:http://<ip>/static/SD/datas/temp.json
    String FilePath__SD__LastSensorDataSave = SensorDataFolder+"temp.json";
    //? 儲存正式的光度計測量數據
    void SaveSensorData_photometer(
      String filePath, String dataTime, String title, String desp, String Gain, String Channel,
      String ValueName, double dilution, double result, double ppm
    );
    //? 更新最新資料儲存檔案
    void ReWriteLastDataSaveFile(String filePath, JsonObject tempData);
    DynamicJsonDocument SetLog(int Level, String Title, String description, AsyncWebSocket *server=NULL, AsyncWebSocketClient *client=NULL, bool Save=true);



    //* 廣播指定池的感測器資料出去
    //!! 注意，這個廣撥出去的資料是會進資料庫的
    void BroadcastNewPoolData(String poolID);

    ////////////////////////////////////////////////////
    //* For 基礎行為
    ////////////////////////////////////////////////////
    SemaphoreHandle_t SQL_xMutex = NULL;




    String GetNowTimeString();
    String GetTimeString(String interval=":");
    String GetDateString(String interval="-");
    String GetDatetimeString(String interval_date="-", String interval_middle=" ", String interval_time=":");
    
    //! led螢幕相關
    void PrintOnScreen(String content);
    void ShowIPAndQRCodeOnOled();

    //? JSON__sensorDataSave: 紀錄當前儀器的感測器資料
    DynamicJsonDocument *JSON__sensorDataSave = new DynamicJsonDocument(50000);



    Motor_Ctrl motorCtrl;
    C_Peristaltic_Motors_Ctrl peristalticMotorsCtrl;
    SPOOLS_Ctrl poolsCtrl;
    CWIFI_Ctrler BackendServer;

    TwoWire WireOne = Wire;
    int WireOne_SDA = 5;
    int WireOne_SCL = 6;
    CMULTI_LTR_329ALS_01 MULTI_LTR_329ALS_01_Ctrler = CMULTI_LTR_329ALS_01(16, 18, 17, &WireOne);

  private:
};
extern SMachine_Ctrl Machine_Ctrl;
extern Adafruit_SH1106 display;
// extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2;
#endif