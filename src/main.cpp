// https://github.com/espressif/arduino-esp32/issues/1089
#include <FreeRTOSConfig.h>
// 請到 FreeRTOSConfig.h 新增.
// #define CONFIG_FREERTOS_USE_TRACE_FACILITY 1
// #define CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS 1

// 如果要更改 log level，請去這邊找
// #include <sdkconfig.h>
// #undef CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL
// #define CONFIG_ARDUHAL_LOG_DEFAULT_LEVEL 5
// #undef ARDUHAL_LOG_LEVEL
// #define ARDUHAL_LOG_LEVEL 5

#include <Arduino.h>
#include <Wire.h>
#include <esp_log.h>
#include "Machine_Ctrl/src/Machine_Ctrl.h"

//TODO oled暫時這樣寫死
// #include <Wire.h> 
// #include <Adafruit_GFX.h>
// #include "Adafruit_SH1106.h"
// #include <Adafruit_SSD1306.h>
#include <U8g2lib.h>
#include "../lib/QRCode/src/qrcode.h"
//TODO oled暫時這樣寫死

const char* FIRMWARE_VERSION = "V3.23.1201.2";

void setup() {
  Serial.begin(115200);
  pinMode(16, OUTPUT);
  pinMode(17, OUTPUT);
  digitalWrite(16, LOW);
  digitalWrite(17, LOW);
  Machine_Ctrl.INIT_I2C_Wires();
  Machine_Ctrl.PrintOnScreen("Load SD Card");
  ESP_LOGD("", "儀器啟動，韌體版本為: %s", FIRMWARE_VERSION);
  ESP_LOGD("", "準備讀取SD卡內資訊");
  Machine_Ctrl.INIT_SD_And_LoadConfigs();

  Machine_Ctrl.PrintOnScreen("Load DB File");
  Machine_Ctrl.openLogDB();
  Machine_Ctrl.openSensorDB();
  Machine_Ctrl.PrintOnScreen("Load SPIFFS File");
  ESP_LOGD("", "準備讀取SPIFFS內資訊");
  Machine_Ctrl.INIT_SPIFFS_And_LoadConfigs();
  ESP_LOGD("", "準備更新最新的各池感測器資料");
  Machine_Ctrl.PrintOnScreen("Load Pool Data");
  Machine_Ctrl.INIT_PoolData();
  ESP_LOGD("", "準備初始化蠕動馬達控制模組");
  Machine_Ctrl.PrintOnScreen("INIT Peristaltic Motors");
  Machine_Ctrl.peristalticMotorsCtrl.INIT_Motors(42,41,40,2);
  ESP_LOGD("", "準備初始化伺服馬達控制模組(PCA9685)");
  Machine_Ctrl.PrintOnScreen("INIT Servo Motors");
  Machine_Ctrl.motorCtrl.INIT_Motors(Machine_Ctrl.WireOne);
  ESP_LOGD("", "停止所有馬達動作");
  Machine_Ctrl.StopDeviceAndINIT();
  ESP_LOGD("", "準備讀取log資訊");
  Machine_Ctrl.PrintOnScreen("Load Logs");
  Machine_Ctrl.SD__LoadOldLogs();
  ESP_LOGD("", "準備使用WIFI連線");
  Machine_Ctrl.PrintOnScreen("INIT WiFi");
  Machine_Ctrl.BackendServer.ConnectToWifi();
  ESP_LOGD("", "準備啟動Server");
  Machine_Ctrl.PrintOnScreen("INIT Web Server");
  Machine_Ctrl.BackendServer.ServerStart();
  ESP_LOGD("", "準備建立排程管理");
  Machine_Ctrl.PrintOnScreen("INIT Task Manager");
  Machine_Ctrl.CreateScheduleManagerTask();
  Machine_Ctrl.BuildTimeCheckTask();
  Machine_Ctrl.BuildOLEDCheckTask();
  // Machine_Ctrl.BackendServer.StartSTAConnectCheck();
  Machine_Ctrl.SetLog(3, "儀器啟動完畢", "");
  ESP_LOGD("", "儀器啟動完畢!");
  delay(1000);
  // Machine_Ctrl.BuildAPICheckerTask();
  Machine_Ctrl.BuildDeviceInfoCheckTask();
}

void loop() {
  vTaskDelay(9999999/portTICK_PERIOD_MS);
}

//                                `-+syhddmmmddhyo+:`                              
//                             .+hmmdddddddddddddddmmds/`      ``...`              
//                          `/hmddddddddddddddddddddddddmy++osyhyyyhhs.            
//                        `ommddddddddddddddddddddddddddddmmdys+++syhhh:           
//            .`         /mmddddddddddddddddddddddddddddddddmmhyyyyyyyyh/          
//        `:sdNy`      .ymddddddddddddddddddddddddddddddddddddmdhhhyyyyyh-         
//    `.+hmmmddmo     -mmddddddddddddddddddddddddddddddddddddddmmhhhhhhhh+         
//   odmmdddddddms` `+mmddddddddddddddddddddddddddddddddddddddddmmddhhhhh+         
//   ymdmmmmmmmmdmmdmmdddddddddddddmmddddddddddddddddddddddddddddmd:ydhhd:         
//   :Nmmmmmmmmmmmmmmddy+::+ydddms/:::/+osydmdddddddddddddddddddddN-`:+o:          
//    ymmmmmmmmmmmmmmdo.`.``/hmm/+hdd/``````-+ydmdddddddddddddddddmy               
//    .mmmmmmmmmmmmmmh:`-o+`:hm/`-o:.```````os/./ymddddddddddddddddN`              
//     /Nmmmmmmmmmmmmh:..::-od/``dMs````````/hNm:`-ymddddddddddddddN-              
//      sNmmmmmmmmmmmd+-:/-.`..`.hh:`...``:hh..:```:NdmmmmmmmdmddddN/              
//   `::/dmmmmmmmmmmmmdo:```./d/````-..:`-mdd.````.dmdmmmmmmmmmmmdmmy              
// ./::-..+dmmmmmmmmmmh/````-dNms-```..``.+/`````-dmds/:-:ohmmmmmmmmN.             
// -/.``--`/dmmmdysydmy.````omNd+.-::-...-:os:..`-hs-``.``./hmmmmmmmmd.       .os` 
// /:---.`.`-/sy:```omy-````hddo` `s.``/NNNNo...`````-+o/``/dmmmmmmmmmms:..:+ymmN: 
// :/.``.``````-``./dNh/````syyyhshd-``:mNmy.````````.```./hmmmmmmmmmmmmmmmmmmmmmd 
//  .::-```...````+dNNNs-```//::/+ooosyhdds.``````-----:+ydmmmmmmmmmmmmmmmmmmmmmmN:
//    `:/````..``-shhdmms-``./::/:::::::+/``````.+dmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmd
//      ./:-```.-oyyhyhhhy+.`.:/::::///:.````..:ymNNmmo:/ymmmmmmmmmmmmmmmmmmmmmmmmN
//        `:y:./yyyyyyyyhh///:..--:--.````..-/ymNNNNNy.``-hNNNNNmmmmmmmmmmmmmmNmh+-
//        `ymdyhyyyyyyhhy/---o+///:::::://oyhmNNNNNNNo```:osyho/---:ymmNNNNmho:`   
//        `+hhhhhyyyyhhs-----y--o/------:+hhhhhhhddds.````````..-..-+dNds+-`       
//          `.:+oossyyyy:---:ssoy:------+hyyyyhhyyys-``....```..--..//`            
//                     syysyysssho:-----ohyyyyhyhhyy/`````..``````-o.              
//                    `hssssssoyhhyo+++syyhhhhyyhyyh+:::-..-:::::::.               
//                    :hysssyo/yhhssyysssssyhyhhhyhdmmy....`                       
//                    shyyyyyyhhhhyyyyyyyyyyhh/-+syhdo`                            
//                   `hhhhhhhhhhhhhhhhhhhhhhhhy`                                   
//                    -:yhyyyysyhhyyyyyyyyyyyyh-                                   
//                      -hysssssyydsyyssssssssyh.                                  
//                       /hsysyyyyd-.+yyyssyyssyh-                                 
//           -/+oo++/-`   +hhyhhhso`  .ohyyyyyyhho:`   `-:/++++/:.                 
//        -+ooooooooooso+/ssss.yy:      `//+ds/oysss+ossoooooooo+os/.              
//     `:o+:/oooooooooooooooossyh:          oyyssooooooooooooooo+/:os+`            
//    -ssoooooooooooooooooosssssy+          .hyssssssooooooooooooooooss-           
//    /syysssssssssssssssyyyyyyyh-           shyyyysyyysssssssssssssssyy           
//      `-/+ossyyyysso+/:-./++//.             .---` `.-:/+oossssoo++/:-`           