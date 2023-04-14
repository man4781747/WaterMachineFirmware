#ifndef WIFI_CTRLER_H
#define WIFI_CTRLER_H

/**
 * @brief 儀器WIFI設定
 * 
 */
class CWIFI_Ctrler
{
  public:
    CWIFI_Ctrler(void){};
    void ConnectToWifi(const char* ssidAP,const char* passwordAP);
    void UpdateMachineTimerByNTP();
    void ServerStart();

    void UploadNewData();

  private:
    void setStaticAPIs();
    void createWebServer();
};

#endif