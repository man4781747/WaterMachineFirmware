#include "Pools_Ctrl.h"

#include <ArduinoJson.h>
#include <ESP32Servo.h>

#include "../../SinglePool_Ctrl/src/SinglePool_Ctrl.h"

/**
 * @brief 新增1項 pool 的設定
 * 
 * @param poolID 
 */
void SPOOLS_Ctrl::addNewPool(String poolID)
{
  ESP_LOGI("POOLS_Ctrl","Add new pool %s", poolID.c_str());
  poolsArray.push_back(SSINGLE_POOL_Ctrl(poolID));
};

/**
 * @brief 獲得當前所有pool的資訊
 * 
 * @return StaticJsonDocument<2048> 
 */
StaticJsonDocument<2048> SPOOLS_Ctrl::GetAllPoolsBaseInfo()
{
  StaticJsonDocument<2048> json_doc;
  // DeserializationError json_error;
  int arrayLen = (int)poolsArray.size();
  JsonVariant json_obj = json_doc.to<JsonVariant>();
  json_doc["poolsCount"].set(arrayLen);
  for (int poolIndex = 0; poolIndex<(int)poolsArray.size(); poolIndex++) {
    json_doc["poolsData"][poolsArray[poolIndex].PoolID].set(poolsArray[poolIndex].GetThisPoolData());
  }
  return json_doc;
};


DynamicJsonDocument SPOOLS_Ctrl::GetPoolInfo(String poolID)
{
  DynamicJsonDocument json_doc(2048);
  int arrayLen = (int)poolsArray.size();
  bool ifPoolExists = false;
  for (int poolIndex = 0; poolIndex<(int)poolsArray.size(); poolIndex++) {
    if (poolsArray[poolIndex].PoolID == poolID) {
      json_doc.set(poolsArray[poolIndex].GetThisPoolData());
      ifPoolExists = true;
      break;
    }
  }
  if (ifPoolExists == false) {
    json_doc["WaringMessage"] = "Pool: "+poolID+" don't exists";
  }
  return json_doc;
};



/**
 * @brief (測試用)亂數更新每個pool的資料
 * 
 */
void SPOOLS_Ctrl::UpdateAllPoolsDataRandom()
{
  // DeserializationError json_error;
  int arrayLen = (int)poolsArray.size();
  for (int poolIndex = 0; poolIndex<(int)poolsArray.size(); poolIndex++) {
    poolsArray[poolIndex].UpdateDataRandom();
  }
};

