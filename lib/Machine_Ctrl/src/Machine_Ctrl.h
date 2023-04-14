#ifndef MACHINE_CTRL_H
#define MACHINE_CTRL_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include <Pools_Ctrl.h>
#include <Motor_Ctrl.h>

enum Steps
{
  Idle,
  INIT_Machine,               // 儀器初始化
  CloseAllSwitch,             // 關閉所有閥門
  OpenPoolSwitch,             // 打開指定蝦池的抽水閥門
  PullWaterFromPool,          // 從蝦池抽水
  PushWaterToPool,            // 將水推回蝦池

  //// 測定未知濃度亞硝酸鹽樣本
  // 獲得原點濃度值
  PullOriginReagentToMixRoom_NO2_Origin,   // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 抽取原點試劑至混合室
  PushMixRoomToDetectionRoom_NO2_Origin,   // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 將混和室液體移動至檢測室
  DetectionValue_NO2_Origin,               // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 檢測水質
  DropWater_NO2_Origin,                    // (測定未知濃度亞硝酸鹽樣本 - 獲得原點濃度值) 丟棄溶液
  // 獲得亞硝酸鹽濃度值
  PullOriginReagentToMixRoom_NO2_Target, // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 抽取原點試劑至混合室
  PullPoolWaterToMixRoom_NO2_Target,     // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 抽取池水至混合室
  MixPoolAndOrigin_NO2_Target,           // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 混和池水與標準溶液
  PullNO2ToMixRoom_NO2_Target,           // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 抽取亞硝酸鹽至混合室
  MixNO2AndWater_NO2_Target,             // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 混和亞硝酸鹽與溶液
  WaitFiceMin_NO2_Target,                // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 等待5分鐘
  PushMixRoomToDetectionRoom_NO2_Target, // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 將混和室液體移動至檢測室
  DetectionValue_NO2_Target,             // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 檢測水質
  DropWater_NO2_Target,                  // (測定未知濃度亞硝酸鹽樣本 - 獲得亞硝酸鹽濃度值) 丟棄溶液

  //// 測定未知濃度氨氮樣本
  // 獲得樣本濃度值
  PullPoolWaterToMixRoom_NH3_Origin,     // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 抽取池水至混合室
  PushMixRoomToDetectionRoom_NH3_Origin, // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 將混和室液體移動至檢測室
  DetectionValue_NH3_Origin,             // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 檢測水質
  DropWater_NH3_Origin,                  // (測定未知濃度氨氮樣本 - 獲得樣本濃度值) 丟棄溶液
  // 獲得亞硝酸鹽濃度值
  PullPoolWaterToMixRoom_NH3_Target,     // (測定未知濃度氨氮樣本 - 獲得氨氮濃度值) 抽取池水至混合室


  PullWaterFromTempPool_1,  // 第一次從暫存池抽水
  PullReagent_1,            // 抽取試劑-1
  MixWaterAndReagent_1,     // 從暫存池抽水
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

    void UpdateAllPoolsDataRandom();

    DynamicJsonDocument GetDeviceInfos();
    String GetDeviceInfosString();

    void PumpPoolWaterToTempTank();

    void ChangeMotorStatus(MOTOR_STATUS StatusCode);

    
    void PreparePumpTempTankWater();


    Motor_Ctrl motorCtrl;
    SPOOLS_Ctrl poolsCtrl;
    // u_int8_t deviceStatus = Schedule::Idle;
  private:
};

#endif