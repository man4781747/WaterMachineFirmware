#include "SinglePool_Ctrl.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>

void SSINGLE_POOL_Ctrl::INIT_ThisPool()
{
  
}


StaticJsonDocument<512> SSINGLE_POOL_Ctrl::GetThisPoolData()
{
  StaticJsonDocument<512> json_doc;
  json_doc["PoolID"] = PoolID;
  json_doc["Data_datetime"] = PoolData.Data_datetime;
  json_doc["NO2_wash_volt"] = PoolData.NO2_wash_volt;
  json_doc["NO2_test_volt"] = PoolData.NO2_test_volt;
  json_doc["NO2"] = PoolData.NO2;
  json_doc["NH4_wash_volt"] = PoolData.NH4_wash_volt;
  json_doc["NH4_test_volt"] = PoolData.NH4_test_volt;
  json_doc["NH4"] = PoolData.NH4;
  json_doc["pH_volt"] = PoolData.pH_volt;
  json_doc["pH"] = PoolData.pH;
  json_doc["salinity_volt"] = PoolData.salinity_volt;
  json_doc["salinity"] = PoolData.salinity;
  json_doc["Temperature"] = PoolData.Temperature;
  json_doc["DO_percentage"] = PoolData.DO_percentage;
  json_doc["DO"] = PoolData.DO;
  json_doc["residual_chlorine"] = PoolData.residual_chlorine;
  json_doc["ca_mg"] = PoolData.ca_mg;
  json_doc["vibrio"] = PoolData.vibrio;
  return json_doc;
}

void SSINGLE_POOL_Ctrl::UpdateDataRandom()
{
  time_t nowTime = now();
  time_t subtractedTime = nowTime - (esp_random() % 1001);
  char datetimeChar[30];

  sprintf(datetimeChar, "%04d-%02d-%02d %02d:%02d:%02d",
    year(subtractedTime), month(subtractedTime), day(subtractedTime),
    hour(subtractedTime), minute(subtractedTime), second(subtractedTime)
  );
  PoolData.Data_datetime = String(datetimeChar);
  PoolData.NO2_wash_volt = (double)((esp_random() % 1001)/100.);   // 亞硝酸鹽清洗電壓
  PoolData.NO2_test_volt = (double)((esp_random() % 501)/100.);    // 亞硝酸鹽測試電壓
  PoolData.NO2 = (double)((esp_random() % 1001)/100.);   // 亞硝酸鹽濃度
  PoolData.NH4_wash_volt = (double)((esp_random() % 501)/100.);  // 氨氮清洗電壓	
  PoolData.NH4_test_volt = (double)((esp_random() % 99901)/100.);  // 氨氮測試電壓
  PoolData.NH4 = (double)((esp_random() % 601)/100.);  // 氨氮濃度
  PoolData.pH_volt = (double)((esp_random() % 99901)/100.);         // 酸鹼電壓
  PoolData.pH = (double)((esp_random() % 1401)/100.);              // 酸鹼值
  PoolData.salinity_volt = (double)((esp_random() % 99901)/100.);    // 鹽度電壓
  PoolData.salinity = (double)((esp_random() % 99901)/100.);        // 鹽度濃度
  PoolData.Temperature = (double)((esp_random() % 4001)/100.);      // 溫度
  PoolData.DO_percentage = (double)((esp_random() % 99901)/100.);    // 溶氧百分比
  PoolData.DO = (double)((esp_random() % 1501)/100.);              // 溶氧濃度
  PoolData.residual_chlorine = (double)((esp_random() % 99901)/100.);      // 餘氯
  PoolData.ca_mg = (double)((esp_random() % 99901)/100.);            // 鈣鎂離子濃度
  PoolData.vibrio = (double)((esp_random() % 99901)/100.);           // 弧菌數
}
