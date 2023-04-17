#ifndef POOLS_Ctrl_H
#define POOLS_Ctrl_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>

#include "../../SinglePool_Ctrl/src/SinglePool_Ctrl.h"


/**
 * 複數養殖池檢測控制的物件
*/
class SPOOLS_Ctrl
{
  public:
    SPOOLS_Ctrl(void){};
    void addNewPool(String poolID);
    StaticJsonDocument<2048> GetAllPoolsBaseInfo();
    DynamicJsonDocument GetPoolInfo(String poolID);

    void UpdateAllPoolsDataRandom();

  private:
    std::vector<SSINGLE_POOL_Ctrl> poolsArray; // pools陣列
};

#endif