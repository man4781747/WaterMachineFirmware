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


  //!機器步驟執行相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/RunHistory", "GET", &ws_GetNRunHistoryInfo);

  //!Sensor結果資料

  WIFI_Ctrler.AddWebsocketAPI("/api/PoolData", "GET", &ws_GetAllPoolData);

  //!蝦池設定相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/Pool/(<name>.*)", "DELETE", &ws_DeletePoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool/(<name>.*)", "GET", &ws_GetPoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool/(<name>.*)", "PATCH", &ws_PatchPoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool", "GET", &ws_GetAllPoolInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pool", "POST", &ws_AddNewPoolInfo);

  //!分光光度計設定相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/Spectrophotometer/(<name>.*)/test", "GET", &ws_TestSpectrophotometer);
  WIFI_Ctrler.AddWebsocketAPI("/api/Spectrophotometer/(<name>.*)", "DELETE", &ws_DeleteSpectrophotometerInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Spectrophotometer/(<name>.*)", "GET", &ws_GetSpectrophotometerInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Spectrophotometer/(<name>.*)", "PATCH", &ws_PatchSpectrophotometerInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Spectrophotometer", "GET", &ws_GetAllSpectrophotometerInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Spectrophotometer", "POST", &ws_AddNewSpectrophotometerInfo);

  //!蠕動馬達設定相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(<name>.*)/test", "GET", &ws_TestPeristalticMotor);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(<name>.*)", "DELETE", &ws_DeletePeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(<name>.*)", "GET", &ws_GetPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(<name>.*)", "PATCH", &ws_PatchPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor", "GET", &ws_GetAllPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor", "POST", &ws_AddNewPeristalticMotorInfo);


  //!伺服馬達設定相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(<name>.*)/test", "GET", &ws_TestPwmMotor);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(<name>.*)", "GET", &ws_GetPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(<name>.*)", "PATCH", &ws_PatchPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(<name>.*)", "DELETE", &ws_DeletePwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor", "GET", &ws_GetAllPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor", "POST", &ws_AddNewPwmMotorInfo);
  

  //!事件組設定相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/Event/(<name>.*)/test", "GET", &ws_TestEvent);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event/(<name>.*)", "GET", &ws_GetEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event/(<name>.*)", "PATCH", &ws_PatchEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event/(<name>.*)", "DELETE", &ws_DeleteEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event", "GET", &ws_GetAllEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event", "POST", &ws_AddNewEventInfo);


  //!步驟設定相關API

  WIFI_Ctrler.AddWebsocketAPI("/api/Step/(<name>.*)/test", "GET", &ws_TestStep);
  WIFI_Ctrler.AddWebsocketAPI("/api/Step/(<name>.*)", "GET", &ws_GetStepInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Step/(<name>.*)", "PATCH", &ws_PatchStepInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Step/(<name>.*)", "DELETE", &ws_DeleteStepInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Step", "GET", &ws_GetAllStepInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Step", "POST", &ws_AddNewStepInfo);

  //!流程設定相關API

  //? [GET]/api/Pipeline/pool_all_data_get/RUN 這支API比較特別，目前是寫死的
  //? 目的在於執行時，他會依序執行所有池的資料，每池檢測完後丟出一次NewData
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline/pool_all_data_get/RUN", "GET", &ws_RunAllPoolPipeline);
  
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline/(<name>.*)/RUN", "GET", &ws_RunPipeline);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline/(<name>.*)/test", "GET", &ws_TestPipeline);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline/(<name>.*)", "GET", &ws_GetPipelineInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline/(<name>.*)", "PATCH", &ws_PatchPipelineInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline/(<name>.*)", "DELETE", &ws_DeletePipelineInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline", "GET", &ws_GetAllPipelineInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline", "POST", &ws_AddNewPipelineInfo);

  //! 流程設定V2
  WIFI_Ctrler.AddWebsocketAPI("/api/Pipeline/(<name>.*)/RUN", "GET", &ws_v2_RunPipeline);


}

#endif