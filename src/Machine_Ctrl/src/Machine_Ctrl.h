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
    SMachine_Ctrl(void){};

    ////////////////////////////////////////////////////
    // For 初始化
    ////////////////////////////////////////////////////

    void INIT_SPIFFS_config();
    void INIT_I2C_Wires();
    void INIT_PoolData();

    ////////////////////////////////////////////////////
    // For 更新設定
    ////////////////////////////////////////////////////

    void UpdatePWMMotorSetting(JsonObject PWMMotorSetting);

    void UpdatePeristalticMotorSetting(JsonObject PeristalticMotorSetting);

    void UpdateEventGroupSetting(JsonObject EventListSetting);

    void UpdateStepGroupSetting(JsonObject StepGroupSetting);

    ////////////////////////////////////////////////////
    // For 資訊獲得
    ////////////////////////////////////////////////////  

    void PrintAllPWNMotorSetting();

    void PrintAllPeristalticMotorSetting();

    void PrintAllEventSetting();

    void PrintAllStepSetting();


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
    void RUN__LOADED_ACTION();


    EVENT_RESULT RUN__PWMMotorTestEvent(String motorID);
    
    EVENT_RESULT RUN__PWMMotorEvent(JsonArray PWMMotorEventList);

    EVENT_RESULT RUN__PeristalticMotorEvent(Peristaltic_task_config *config_);

    EVENT_RESULT RUN__History();

    void STOP_AllTask();

    void RESUME_AllTask();

    void Stop_AllPeristalticMotor();

    ////////////////////////////////////////////////////
    // For 不間斷監聽
    ////////////////////////////////////////////////////


    ////////////////////////////////////////////////////
    // For 互動相關
    ////////////////////////////////////////////////////

    DynamicJsonDocument GetDeviceInfos();

    String GetDeviceInfosString();

    DynamicJsonDocument GetEventStatus();

    ////////////////////////////////////////////////////
    // For 基礎行為
    ////////////////////////////////////////////////////

    void LoadStepToRunHistoryItem(String StepID, String TrigerBy);

    ////////////////////////////////////////////////////
    // For 組合行為
    ////////////////////////////////////////////////////


    ////////////////////////////////////////////////////
    // For 測試
    ////////////////////////////////////////////////////

    void UpdateAllPoolsDataRandom();

    void LoopTest();

    ////////////////////////////////////////////////////
    // 公用參數
    ////////////////////////////////////////////////////

    /**
     * @brief 當前執行動作TASK
     * 
     */
    TaskHandle_t TASK__NOW_ACTION;
    DynamicJsonDocument *loadedAction = new DynamicJsonDocument(500000);

    DynamicJsonDocument *sensorDataSave = new DynamicJsonDocument(50000);

    TaskHandle_t TASK__PWM_MOTOR;
    TaskHandle_t TASK__Peristaltic_MOTOR;
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