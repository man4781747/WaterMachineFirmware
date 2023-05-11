#ifndef WIFI_URL_H
#define WIFI_URL_H

#include "models.h"
#include "Wifi_Ctrl.h"

void setAPI(CWIFI_Ctrler &WIFI_Ctrler)
{
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(.*)", "DELETE", &ws_DeletePeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(.*)", "GET", &ws_GetPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor/(.*)", "PATCH", &ws_PatchPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor", "GET", &ws_GetAllPeristalticMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/PeristalticMotor", "POST", &ws_AddNewPeristalticMotorInfo);

  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(.*)", "GET", &ws_GetPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(.*)", "PATCH", &ws_PatchPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor/(.*)", "DELETE", &ws_DeletePwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor", "GET", &ws_GetAllPwmMotorInfo);
  WIFI_Ctrler.AddWebsocketAPI("/api/pwmMotor", "POST", &ws_AddNewPwmMotorInfo);
  
}

#endif