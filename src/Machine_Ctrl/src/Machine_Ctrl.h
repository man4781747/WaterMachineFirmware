#ifndef MACHINE_CTRL_H
#define MACHINE_CTRL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include <Pools_Ctrl.h>
#include <Motor_Ctrl.h>
#include <SPIFFS_Ctrl.h>
#include <Machine_Base_info.h>

#include "../../Wifi_Ctrl/src/Wifi_Ctrl.h"

enum Steps
{
  Idle,
  INIT_Machine,                            // 儀器初始化
  CloseAllSwitch,                          // 關閉所有 閥門
  OpenPoolSwitch,                          // 打開 指定蝦池 的 抽水閥門
  PullWaterFromPool,                       // 從 蝦池 抽水至 暫存室
  PushWaterToPool,                         // 將水推回 蝦池

  //// 測定未知濃度亞硝酸鹽樣本
  // 獲得原點濃度值
  PullOriginReagentToMixRoom_NO2_Origin,   // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 抽取 原點試劑 至 混合室
  PushMixRoomToDetectionRoom_NO2_Origin,   // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 將 混和室 液體 移動至 檢測室
  DetectionValue_NO2_Origin,               // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 檢測水質
  DropWater_NO2_Origin,                    // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 丟棄溶液
  // 獲得亞硝酸鹽濃度值
  PullOriginReagentToMixRoom_NO2_Target,   // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 抽取 原點試劑 至 混合室
  PullPoolWaterToMixRoom_NO2_Target,       // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 抽取 暫存室 至 混合室
  MixPoolAndOrigin_NO2_Target,             // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 混和 暫存室 與 標準溶液
  PullNO2ToMixRoom_NO2_Target,             // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 抽取 亞硝酸鹽 至 混合室
  MixNO2AndWater_NO2_Target,               // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 混和 亞硝酸鹽 與 溶液
  WaitFiceMin_NO2_Target,                  // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 等待 5分鐘
  PushMixRoomToDetectionRoom_NO2_Target,   // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 將 混和室 液體 移動至 檢測室
  DetectionValue_NO2_Target,               // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 檢測水質
  DropWater_NO2_Target,                    // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 丟棄溶液

  //// 測定未知濃度氨氮樣本
  // 獲得樣本濃度值
  PullPoolWaterToMixRoom_NH3_Origin,       // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 抽取 暫存室 至 混合室
  PushMixRoomToDetectionRoom_NH3_Origin,   // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 將 混和室 液體 移動至 檢測室
  DetectionValue_NH3_Origin,               // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 檢測水質
  DropWater_NH3_Origin,                    // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 丟棄溶液
  // 獲得亞硝酸鹽濃度值
  PullPoolWaterToMixRoom_NH3_Target,       // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 抽取 暫存室 至 混合室
  Pull_NH3_R1_ToMixRoom_NH3_Target,        // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 抽取 氨氮R1 至 混合室
  Mix_NH3_R1_NH3_Target,                   // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 混和 氨氮R1 與 溶液
  Pull_NH3_R2_ToMixRoom_NH3_Target,        // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 抽取 氨氮R2 至 混合室
  Mix_NH3_R2_And_Wait_NH3_Target,          // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 混和 氨氮R2 與 溶液 並等待 5分鐘
  PushMixRoomToDetectionRoom_NH3_Target,   // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 將 混和室 液體 移動至 檢測室
  DetectionValue_NH3_Target,               // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 檢測水質
  DropWater_NH3_Target,                    // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 丟棄溶液
};

enum MOTOR_STATUS : u_int16_t
{
  ALL_OPEN          = 0b1111111111111111,
  ALL_CLOSE         = 0b0000000000000000,
  GET_TEMP_TANK     = 0b1000000000000000,
  GET_REAGENT_1     = 0b0100000000000000,
  GET_REAGENT_2     = 0b0010000000000000,
  GET_REAGENT_3     = 0b0001000000000000,
  GET_REAGENT_4     = 0b0000100000000000,
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


    DynamicJsonDocument GetDeviceInfos();
    String GetDeviceInfosString();

    ////////////////////////////////////////////////////
    // For 基礎行為
    ////////////////////////////////////////////////////

    void ChangeMotorStatus(MOTOR_STATUS StatusCode);

    ////////////////////////////////////////////////////
    // For 組合行為
    ////////////////////////////////////////////////////
    
    void PumpPoolWaterToTempTank();
    void PreparePumpTempTankWater();

    ////////////////////////////////////////////////////
    // For 測試
    ////////////////////////////////////////////////////

    void UpdateAllPoolsDataRandom();


    Motor_Ctrl motorCtrl;
    SPOOLS_Ctrl poolsCtrl;
    CWIFI_Ctrler BackendServer;
    SPIFFS_Ctrl spiffs;
    Machine_Info MachineInfo;
  private:
};
extern SMachine_Ctrl Machine_Ctrl;
#endif