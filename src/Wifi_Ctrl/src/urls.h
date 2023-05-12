#ifndef WIFI_URL_H
#define WIFI_URL_H

#include "models.h"
#include "Wifi_Ctrl.h"

void setAPI(CWIFI_Ctrler &WIFI_Ctrler)
{
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(<name>.*)", "DELETE", &ws_DeletePeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(<name>.*)", "GET", &ws_GetPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(<name>.*)", "PATCH", &ws_PatchPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor", "GET", &ws_GetAllPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor", "POST", &ws_AddNewPeristalticMotorInfo);

  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(<name>.*)", "GET", &ws_GetPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(<name>.*)", "PATCH", &ws_PatchPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(<name>.*)", "DELETE", &ws_DeletePwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor", "GET", &ws_GetAllPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor", "POST", &ws_AddNewPwmMotorInfo);
  
  WIFI_Ctrler.AddWebsocketAPI("/api/Event/(<name>.*)", "GET", &ws_GetEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event/(<name>.*)", "PATCH", &ws_PatchEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event/(<name>.*)", "DELETE", &ws_DeleteEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event", "GET", &ws_GetAllEventInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/Event", "POST", &ws_AddNewEventInfo);

}

#endif