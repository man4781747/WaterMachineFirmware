#ifndef MACHINE_CTRL_H
#define MACHINE_CTRL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <variant>

#include <Pools_Ctrl.h>
#include <Motor_Ctrl.h>
#include <SPIFFS_Ctrl.h>

#include <TimeLib.h>   

#include "Wifi_Ctrl/src/Wifi_Ctrl.h"

////////////////////////////////////////////////////
// 事件類別 - START
////////////////////////////////////////////////////


struct EVENT_RESULT {
  int status = 99;
  String message = "";
};

/**
 * @brief 伺服馬達控制事件
 * 內含2參數
 * 1. motorID: 待控制的馬達ID
 * 2. motortStatus: 待控制的馬達目標角度
 * 
 */
struct PWM_MOTOR_STATUS_SET_OBJ {
  String motorID;
  int motortStatus;
};

/**
 * @brief 蠕動馬達控制事件
 * 1. motorID: 待控制的馬達ID
 * 2. motortStatus: 1(正轉)、2(反轉)
 * 3. activeTime: 運轉時長
 * 
 */
struct PERISTALTIC_STATUS_SET_OBJ {
  String motorID;
  int motortStatus;
  int activeTime;
};


/**
 * @brief 等待事件
 * 
 */
struct WAIT_EVENT_OBJ {
  int waitTime;
};

////////////////////////////////////////////////////
// 事件類別 - END
////////////////////////////////////////////////////


/**
 * @brief 單一事件組的設定物件
 * 主要是給 RUN_EVENT_GROUP 使用
 * 依初始化時輸入的參數類型決定內容
 * 
 */
class EVENT
{
  public:
    EVENT(std::vector<PERISTALTIC_STATUS_SET_OBJ> *peristalticMotorList_){
      peristalticMotorList = peristalticMotorList_;
    };
    EVENT(std::vector<PWM_MOTOR_STATUS_SET_OBJ> *PWM_MotorList_){
      PWM_MotorList = PWM_MotorList_;
    };
    EVENT(WAIT_EVENT_OBJ *waitTime_){
      waitTime = waitTime_;
    };
    String Type = "";
    std::vector<PWM_MOTOR_STATUS_SET_OBJ> *PWM_MotorList = NULL;
    std::vector<PERISTALTIC_STATUS_SET_OBJ> *peristalticMotorList = NULL;
    WAIT_EVENT_OBJ *waitTime = NULL;
};

/**
 * @brief 事件組合的設定物件
 * 1. 事件名稱
 * 2. 事件說明
 * 3. 事件清單 []
 * 
 */
struct RUN_EVENT_GROUP {
  String Title;
  String Description;
  std::vector<EVENT> EventList;
};

/**
 * @brief 流程組合的設定物件
 * 1. 流程名稱
 * 2. 流程說明
 * 3. 事件組清單 []
 * 
 */
struct RUN_STEP_GROUP {
  String Title;
  String Description;
  std::vector<String> EventGroupNameList;
};


enum DeviceStatusCode : int
{
  device_idel, device_busy, device_stop
};


/**
 * @brief 當前儀器狀態
 * 
 */
struct DeviceStatus {
  DeviceStatusCode deviceStatusCode = DeviceStatusCode::device_idel;
  String StartTime = "";
  int NowStep = 0;
  RUN_STEP_GROUP *NowRunningSteps = NULL;
  RUN_EVENT_GROUP *NowRunningEvent = NULL;
};

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

    void INIT_SW_Moter();

    void INIT_Peristaltic_Moter();

    void INIT_UpdateEventGroupSetting();

    void INIT_UpdateStepGroupSetting();

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

    EVENT_RESULT RUN__PWMMotorEvent(JsonArray PWMMotorEventList);

    EVENT_RESULT RUN__PeristalticMotorEvent(JsonArray PeristalticMotorEventList);

    EVENT_RESULT RUN__History();

    void STOP_AllTask();

    void RESUME_AllTask();

    void Stop_AllPeristalticMotor();

    ////////////////////////////////////////////////////
    // For 不間斷監聽
    ////////////////////////////////////////////////////

    void Build_SwitchMotorScan();

    void Build_PeristalticMotorScan();

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

    void RUN_EventGroup(String EVENT_NAME);

    void RUN_Step(String STEP_NAME);

    ////////////////////////////////////////////////////
    // For 測試
    ////////////////////////////////////////////////////

    void UpdateAllPoolsDataRandom();

    void LoopTest();

    ////////////////////////////////////////////////////
    // 公用參數
    ////////////////////////////////////////////////////

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
    
    DeviceStatus NowDeviceStatus;

    std::unordered_map<std::string, RUN_EVENT_GROUP> D_eventGroupList;
    std::unordered_map<std::string, RUN_STEP_GROUP> D_stepGroupList;
    
    ////////////////////////////////////////////////////
    // 捨棄使用，純紀錄
    ////////////////////////////////////////////////////

    // void ChangeMotorStatus(MOTOR_STATUS StatusCode, char* TaskName, char* NextTaskName="", bool waitForTigger=false);


  private:
};
extern SMachine_Ctrl Machine_Ctrl;
#endif