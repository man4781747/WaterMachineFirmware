#ifndef SINGLE_POOL_Ctrl_H
#define SINGLE_POOL_Ctrl_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include "esp_system.h"

/**
 * @brief Pool資料組
 * 
 */
class SPOOL_DATA
{
  public:
    String Data_datetime = "2023-10-10 10:10:10";
    double NO2_wash_volt = 0;     // 亞硝酸鹽清洗電壓
    double NO2_test_volt = 0;     // 亞硝酸鹽測試電壓
    double           NO2 = 0;     // 亞硝酸鹽濃度

    double NH4_wash_volt = 0;     // 氨氮清洗電壓	
    double NH4_test_volt = 0;     // 氨氮測試電壓
    double           NH4 = 0;     // 氨氮濃度

    double       pH_volt = 0;     // 酸鹼電壓
    double            pH = 0;     // 酸鹼值
};


/**
 * @brief 單一養殖池檢測控制的物件
 * 
 */
class SSINGLE_POOL_Ctrl
{
  public:
    SSINGLE_POOL_Ctrl(String PoolID_){PoolID=PoolID_;};
    void INIT_ThisPool();
    StaticJsonDocument<512> GetThisPoolData();
    
    void UpdateDataRandom(); 

    String PoolID = "";
    SPOOL_DATA PoolData;
  private:
};

#endif