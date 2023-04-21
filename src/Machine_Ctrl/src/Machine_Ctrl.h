#ifndef MACHINE_CTRL_H
#define MACHINE_CTRL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <variant>

#include <Pools_Ctrl.h>
#include <Motor_Ctrl.h>
#include <SPIFFS_Ctrl.h>
#include <Machine_Base_info.h>

#include "../../Wifi_Ctrl/src/Wifi_Ctrl.h"

struct PWM_MOTOR_STATUS_SET_OBJ {
  int motortIndex;
  int motortStatus;
};

enum PWM_POSITION_MAPPING : int
{
  S_B1 = 31, S_B2 = 30, S_B3 = 29, S_B4 = 28, S_M0 = 19,
  S_M1 = 18, S_M2 = 17, S_M3 = 16, S_M4 = 3, S_M5 = 2,
  S_M6 = 1, S_M7 = 0, S_M8 = 4, S_L1 = 8, S_L2 = 9,
  S_PH1 = 22, S_PH2 = 23,
};

struct PERISTALTIC_STATUS_SET_OBJ {
  int motortIndex;
  int motortStatus;
  int activeTime;
};

enum PERISTALTIC_MOTOR_MAPPING : int
{
  M1, M2, M3, M4, M5, M6, M7
};

/**
 * @brief 伺服馬達與蠕動馬達的控制組設定物件
 * 
 */
struct RUN_MOTOR_GROUP {
  String Title;
  String Description;
  std::vector<PWM_MOTOR_STATUS_SET_OBJ> pwmCtrlList;
  // PWM_MOTOR_STATUS_SET_OBJ* pwmCtrlList;
  // int pwmCtrlListLength;
  PERISTALTIC_STATUS_SET_OBJ perostalicMotorCtrl;
};

/**
 * @brief 伺服馬達與蠕動馬達的控制組設定
 * 請至 Machine_Ctrl.cpp 中編輯細節內容
 */

extern RUN_MOTOR_GROUP Clear_MixRoom;
extern RUN_MOTOR_GROUP Mix_Liquid_In_MixRoom;

extern RUN_MOTOR_GROUP Push_RO_Liquid_To_MixRoom;
extern RUN_MOTOR_GROUP Push_Sample_Liquid_To_MixRoom;

extern RUN_MOTOR_GROUP Push_NO2_Liquid_To_MixRoom;
extern RUN_MOTOR_GROUP Push_NH3R1_Liquid_To_MixRoom;
extern RUN_MOTOR_GROUP Push_NH3R2_Liquid_To_MixRoom;

extern RUN_MOTOR_GROUP Push_MixRoom_To_NO2_SensorRoom;
extern RUN_MOTOR_GROUP Clear_NO2_SensorRoom_To_MixRoom;

extern RUN_MOTOR_GROUP Push_MixRoom_To_NH3_SensorRoom;
extern RUN_MOTOR_GROUP Clear_NH3_SensorRoom_To_MixRoom;

/**
 * @brief 單一事件組的設定物件
 * 
 */

class EVENT_GROUP
{
  public:
    EVENT_GROUP(RUN_MOTOR_GROUP *motorGroupEvent_){
      motorGroupEvent = motorGroupEvent_;
    };
    RUN_MOTOR_GROUP *motorGroupEvent = NULL;
};

/**
 * @brief 事件組組合的設定物件
 * 
 */
struct RUN_EVENT_GROUP {
  String Title;
  String Description;
  std::vector<EVENT_GROUP> eventGroup;
};

/**
 * @brief 事件組組合的設定設定
 * 請至 Machine_Ctrl.cpp 中編輯細節內容
 */

extern RUN_EVENT_GROUP RUN_NO2_Original_Value;

enum DeviceStatusCode : int
{
  device_idel, device_busy
};

struct DeviceStatus {
  DeviceStatusCode deviceStatusCode = DeviceStatusCode::device_idel;
  String StartTime = "";
  int NowStep = 0;
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

    void Set_SW_MotorStatus(std::vector<PWM_MOTOR_STATUS_SET_OBJ> motorStatusList);

    ////////////////////////////////////////////////////
    // For 組合行為
    ////////////////////////////////////////////////////
    
    void SwitchPWMMotor__AND__RunPeristalticMotor(RUN_MOTOR_GROUP *setting);
    
    void RUN_EVENT(RUN_EVENT_GROUP *eventGroupSetting);

    void RUN_NO2_Original_Value();


    ////////////////////////////////////////////////////
    // For 測試
    ////////////////////////////////////////////////////

    void UpdateAllPoolsDataRandom();

    void LoopTest();

    ////////////////////////////////////////////////////
    // 公用參數
    ////////////////////////////////////////////////////

    Motor_Ctrl motorCtrl;
    C_Peristaltic_Motors_Ctrl peristalticMotorsCtrl;
    SPOOLS_Ctrl poolsCtrl;
    CWIFI_Ctrler BackendServer;
    SPIFFS_Ctrl spiffs;
    Machine_Info MachineInfo;
    
    DeviceStatus NowDeviceStatus;
    
    ////////////////////////////////////////////////////
    // 捨棄使用，純紀錄
    ////////////////////////////////////////////////////

    // void ChangeMotorStatus(MOTOR_STATUS StatusCode, char* TaskName, char* NextTaskName="", bool waitForTigger=false);


  private:
};
extern SMachine_Ctrl Machine_Ctrl;
#endif