#ifndef WIFI_URL_H
#define WIFI_URL_H

#include "models.h"
#include "Wifi_Ctrl.h"

void setAPI(CWIFI_Ctrler &WIFI_Ctrler)
{
  WIFI_Ctrler.AddWebsocketAPI("/api/System/STOP", "GET", &ws_StopAllActionTask);

  WIFI_Ctrler.AddWebsocketAPI("/api/System", "GET", &ws_StopAllActionTask);


  WIFI_Ctrler.AddWebsocketAPI("/api/DeiveConfig", "GET", &ws_GetDeiveConfig);
  WIFI_Ctrler.AddWebsocketAPI("/api/DeiveConfig", "PATCH", &ws_PatchDeiveConfig);


  WIFI_Ctrler.AddWebsocketAPI("/api/GetState", "GET", &ws_GetNowStatus);

  //!LOG相關API
  WIFI_Ctrler.AddWebsocketAPI("/api/LOG", "GET", &ws_GetLogs);


  //!Sensor結果資料

  WIFI_Ctrler.AddWebsocketAPI("/api/PoolData", "GET", &ws_GetAllPoolData);

  //!蝦池設定相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/Pool/(<name>.*)", "DELETE", &ws_DeletePoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool/(<name>.*)", "GET", &ws_GetPoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool/(<name>.*)", "PATCH", &ws_PatchPoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool", "GET", &ws_GetAllPoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool", "POST", &ws_AddNewPoolInfo);



  //! 流程設定V2
  WIFI_Ctrler.AddWebsocketAPI("/api/v2/Pipeline/(<name>.*)/RUN", "GET", &ws_v2_RunPipeline);

  //! 儀器控制
  WIFI_Ctrler.AddWebsocketAPI("/api/v2/DeviceCtrl/Spectrophotometer", "GET", &ws_v2_RunPipeline);
  WIFI_Ctrler.AddWebsocketAPI("/api/v2/DeviceCtrl/PwmMotor", "GET", &ws_v2_RunPwmMotor);
  WIFI_Ctrler.AddWebsocketAPI("/api/v2/DeviceCtrl/PeristalticMotor", "GET", &ws_v2_RunPeristalticMotor);
  WIFI_Ctrler.AddWebsocketAPI("/api/v2/DeviceCtrl/PHmeter", "GET", &ws_v2_RunPipeline);
  
}

#endif