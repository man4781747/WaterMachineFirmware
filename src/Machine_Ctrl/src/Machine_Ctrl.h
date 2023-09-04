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
      logArray = DeviceLogSave->createNestedArray("Log");
      vSemaphoreCreateBinary(LOAD__ACTION_V2_xMutex);
    };

    ////////////////////////////////////////////////////
    // For 初始化
    ////////////////////////////////////////////////////

    void INIT_SPIFFS_config();
    void INIT_SD_Card();
    void INIT_I2C_Wires();
    void INIT_PoolData();


    bool LoadJsonConfig(fs::FS& fileSystem, String FilePath, JsonDocument &doc);
    void LoadPiplineConfig();
    void LoadOldLogs();

    //? 緊急終止所有動作，並且回歸初始狀態
    void StopDeviceAndINIT();


    String spectrophotometerConfigFileFullPath = "/config/spectrophotometer_config.json";
    DynamicJsonDocument *spectrophotometerConfig = new DynamicJsonDocument(4000);
    void LoadspectrophotometerConfig();

    String PHmeterConfigFileFullPath = "/config/PHmeter_config.json";
    DynamicJsonDocument *PHmeterConfig = new DynamicJsonDocument(2000);
    void LoadPHmeterConfig();


    DynamicJsonDocument *PipelineConfigList = new DynamicJsonDocument(10000);
    //? 更新當前pipeline列表資料
    //? 以下情況需要觸發他: 1. 剛開機時、2. pipeline檔案有變動時
    void UpdatePipelineConfigList();

    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! 流程設定相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    //? pipelineStack: 待執行的Step列表
    DynamicJsonDocument *pipelineStack = new DynamicJsonDocument(10000);
    //? pipelineConfig: 當前運行的Pipeline詳細設定
    DynamicJsonDocument *pipelineConfig = new DynamicJsonDocument(65525);
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

    String configsFolder = "/config/";
    String deviceBaseConfigFileFullPath = configsFolder+"device_base_config.json";
    String wifiConfigFileFullPath = configsFolder+"wifi_config.json";

    
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //! SD卡系統相關
    //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    
    String SD__configsFolder = "/config/";
    String SD__piplineConfigsFileFullPath = SD__configsFolder+"event_config.json";
    //? LOG 資料夾位置
    String LogFolder = "/logs/";
    //? 感測器資料儲存資料夾
    String SensorDataFolder = "/datas/";
    //? 最新資料儲存檔案位置
    //? API路徑:<ip>/static/SD/datas/temp.json
    String LastDataSaveFilePath = SensorDataFolder+"temp.json";
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

    ////////////////////////////////////////////////////
    //* 公用參數
    ////////////////////////////////////////////////////

    //! SPIFFS TASK
    TaskHandle_t TASK__SPIFFS_Working;
    String ReWriteDeviceSetting();


    //? sensorDataSave: 紀錄當前儀器的感測器資料
    DynamicJsonDocument *sensorDataSave = new DynamicJsonDocument(50000);

    //? DeviceLogSave: 當前儀器Log的儲存
    DynamicJsonDocument *DeviceLogSave = new DynamicJsonDocument(50000);
    JsonArray logArray;



    Motor_Ctrl motorCtrl;
    C_Peristaltic_Motors_Ctrl peristalticMotorsCtrl;
    SPOOLS_Ctrl poolsCtrl;
    CWIFI_Ctrler BackendServer;
    SPIFFS_Ctrl spiffs;

    TwoWire WireOne = Wire;
    int WireOne_SDA = 5;
    int WireOne_SCL = 6;
    CMULTI_LTR_329ALS_01 MULTI_LTR_329ALS_01_Ctrler = CMULTI_LTR_329ALS_01(16, 18, 17, &WireOne);

  private:
};
extern SMachine_Ctrl Machine_Ctrl;
// extern U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2;
#endif