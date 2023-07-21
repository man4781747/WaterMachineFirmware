#ifndef ExSPIFFS_FUNCTION_H
#define ExSPIFFS_FUNCTION_H
#include <esp_log.h>
#include <SPIFFS.h>

inline bool ExSPIFFS_CreateFile(fs::SPIFFSFS& SPIFFS_, String FilePath)
{
  ESP_LOGI("ExSPIFFS", "CreateFile: %s", FilePath.c_str());
  String folderPath = FilePath.substring(0,FilePath.lastIndexOf('/')+1);
  if (folderPath.indexOf('/') == 0){
    folderPath = folderPath.substring(1,folderPath.length());
  }
  String folderPathStack = "";
  int index;
  int length;
  while (1){
    index = folderPath.indexOf('/');
    if (index <0) {
      break;
    }
    folderPathStack = folderPathStack + "/" + folderPath.substring(0,index);
    folderPath = folderPath.substring(index+1,folderPath.length());
    if (SPIFFS_.exists(folderPathStack) == false){
      ESP_LOGD("ExSPIFFS", "Create folder: %s", folderPathStack.c_str());
      if (SPIFFS_.mkdir(folderPathStack) == false) {
        return false;
      }
    }
  }
  if (SPIFFS_.exists(FilePath) == false){
    File selectedFile = SPIFFS_.open(FilePath, FILE_WRITE);
    selectedFile.close();
  }
  return true;
}


#endif