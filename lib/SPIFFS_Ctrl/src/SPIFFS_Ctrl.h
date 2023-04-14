#ifndef SPIFFS_Ctrl_H
#define SPIFFS_Ctrl_H

#include <Arduino.h>

class SPIFFS_Ctrl
{
  public:
    SPIFFS_Ctrl(void){};
    void INIT_SPIFFS();


    void LoadMachineSetting();

    String GetMotorSetting();
  private:
};

#endif