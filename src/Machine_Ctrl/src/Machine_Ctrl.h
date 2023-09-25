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

#include <U8g2lib.h>
#include "../lib/QRCode/src/qrcode.h"

#include "Wifi_Ctrl/src/Wifi_Ctrl.h"


////////////////////////////////////////////////////
// 事件類別 - START
////////////////////////////////////////////////////


struct EVENT_RESULT {
  int status = 99;
  String message = "";
};

struct Peristaltic_task_config {
  int index;
  int status;
  float time;
  int until;
  bool timeoutFail = false;
  long startTime;
  long endTime;
};


////////////////////////////////////////////////////
// 事件類別 - END
////////////////////////////////////////////////////


/**
 * 儀器控制主體
*/
class SMachine_Ctrl
{
  public:
    SMachine_Ctrl(void){
      vSemaphoreCreateBinary(LOAD__ACTION_V2_xMutex);
    };

    void INIT_I2C_Wires();
    bool INIT_PoolData();

    //? 緊急終止所有動作，並且回歸初始狀態
    void StopDeviceAndINIT();



    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! 流程設定相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    //? JSON__pipelineStack: 待執行的Step列表
    DynamicJsonDocument *JSON__pipelineStack = new DynamicJsonDocument(60000);
    //? JSON__pipelineConfig: 當前運行的Pipeline詳細設定
    DynamicJsonDocument *JSON__pipelineConfig = new DynamicJsonDocument(120000);
    //? pipelineTaskHandleMap: 記錄了當前正在執行Step的Task，Key為Step名稱、Value為TaskHandle_t
    std::map<String, TaskHandle_t*> pipelineTaskHandleMap;
    bool LOAD__ACTION_V2(DynamicJsonDocument *pipelineStackList);
    void AddNewPiplelineFlowTask(String stepsGroupName);
    void CleanAllStepTask();
    void CreatePipelineFlowScanTask();
    void CreatePipelineFlowScanTask(DynamicJsonDocument *pipelineStackList);
    //? 排程狀態檢視Task，負責檢查排程進度、觸發排程執行
    TaskHandle_t TASK__pipelineFlowScan = NULL;
    //? LOAD__ACTION_V2_xMutex: 儀器是否忙碌一切依此 xMutex 決定
    SemaphoreHandle_t LOAD__ACTION_V2_xMutex = NULL;
    double lastLightValue_CH0;
    double lastLightValue_CH1;


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
    String FilePath__SD__SpectrophotometerConfig = "/config/spectrophotometer_config.json";
    DynamicJsonDocument *JSON__SpectrophotometerConfig = new DynamicJsonDocument(4000);
    String FilePath__SD__PHmeterConfig = "/config/PHmeter_config.json";
    DynamicJsonDocument *JSON__PHmeterConfig = new DynamicJsonDocument(2000);
    void SD__LoadspectrophotometerConfig();
    void SD__LoadPHmeterConfig();
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
// extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2;
#endif