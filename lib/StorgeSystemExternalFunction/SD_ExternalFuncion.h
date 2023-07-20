#ifndef ExSD_FUNCTION_H
#define ExSD_FUNCTION_H
#include <esp_log.h>
#include <SD.h>

inline bool ExSD_CreateFile(fs::SDFS& SD_, String FilePath)
{
  ESP_LOGI("ExSD", "CreateFile: %s", FilePath.c_str());
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
    if (SD_.exists(folderPathStack) == false){
      ESP_LOGD("ExSD", "Create folder: %s", folderPathStack.c_str());
      if (SD_.mkdir(folderPathStack) == false) {
        return false;
      }
    }
  }
  if (SD_.exists(FilePath) == false){
    File selectedFile = SD_.open(FilePath, FILE_WRITE);
    selectedFile.close();
  }
  return true;
}


#endif