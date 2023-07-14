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
      xSemaphoreGive(MUTEX_Peristaltic_MOTOR);
    };

    ////////////////////////////////////////////////////
    // For 初始化
    ////////////////////////////////////////////////////

    void INIT_SPIFFS_config();
    void INIT_SD_Card();
    void INIT_I2C_Wires();
    void INIT_PoolData();

    //? 緊急終止所有動作，並且回歸初始狀態
    void StopDeviceAndINIT();

    ////////////////////////////////////////////////////
    // For 數值轉換
    ////////////////////////////////////////////////////  

    int pwmMotorIDToMotorIndex(String motorID);

    int PeristalticMotorIDToMotorIndex(String motorID);

    ////////////////////////////////////////////////////
    // For 事件執行
    ////////////////////////////////////////////////////

    // EVENT_RESULT RUN__PWMMotorTestEvent(String motorID);

    void LOAD__ACTION(String actionJSONString);
    void LOAD__ACTION(JsonObject actionJSON);
    void RUN__LOADED_ACTION();

    EVENT_RESULT RUN__PeristalticMotorEvent(Peristaltic_task_config *config_);

    void STOP_AllTask();

    void RESUME_AllTask();

    void Stop_AllPeristalticMotor();

    ////////////////////////////////////////////////////
    // For 互動相關
    ////////////////////////////////////////////////////

    DynamicJsonDocument SetLog(int Level, String Title, String description, AsyncWebSocket *server=NULL, AsyncWebSocketClient *client=NULL);

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


    ////////////////////////////////////////////////////
    //* For 測試
    ////////////////////////////////////////////////////

    void UpdateAllPoolsDataRandom();

    void LoopTest();

    ////////////////////////////////////////////////////
    //* 公用參數
    ////////////////////////////////////////////////////

    //! SPIFFS TASK
    TaskHandle_t TASK__SPIFFS_Working;
    String ReWriteDeviceSetting();

    //! 當前執行動作TASK
    TaskHandle_t TASK__NOW_ACTION;
    DynamicJsonDocument *loadedAction = new DynamicJsonDocument(1000000);

    DynamicJsonDocument *sensorDataSave = new DynamicJsonDocument(50000);
    DynamicJsonDocument *DeviceLogSave = new DynamicJsonDocument(50000);
    JsonArray logArray;

    //! SD卡系統相關參數
    String LogFolder = "/logs/";

    TaskHandle_t TASK__Peristaltic_MOTOR;
    //? 蠕動馬達Task的互斥鎖，確保蠕動馬達的 moduleDataList 內容不會被多個Task同時變動
    SemaphoreHandle_t MUTEX_Peristaltic_MOTOR = xSemaphoreCreateBinary();
    TaskHandle_t TASK__History;
    
    TickType_t LastSuspendTick;
    unsigned long TaskSuspendTimeSum = 0;

    Motor_Ctrl motorCtrl;
    C_Peristaltic_Motors_Ctrl peristalticMotorsCtrl;
    SPOOLS_Ctrl poolsCtrl;
    CWIFI_Ctrler BackendServer;
    SPIFFS_Ctrl spiffs;

    TwoWire WireOne = TwoWire(1);
    int WireOne_SDA = 5;
    int WireOne_SCL = 6;
    CMULTI_LTR_329ALS_01 MULTI_LTR_329ALS_01_Ctrler = CMULTI_LTR_329ALS_01(16, 18, 17, &WireOne);
    // CLTR_329ALS_01 LTR_329ALS_01_Ctrler = CLTR_329ALS_01(&WireOne);

    ////////////////////////////////////////////////////
    // 捨棄使用，純紀錄
    ////////////////////////////////////////////////////

    // void ChangeMotorStatus(MOTOR_STATUS StatusCode, char* TaskName, char* NextTaskName="", bool waitForTigger=false);


  private:
};
extern SMachine_Ctrl Machine_Ctrl;
#endif