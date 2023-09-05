#ifndef ExSD_FUNCTION_H
#define ExSD_FUNCTION_H
#include <esp_log.h>
#include <SD.h>
#include <ArduinoJson.h>

//! 大檔案讀寫專用變數

TaskHandle_t TASK_ReWriteBigFile = NULL;
QueueHandle_t u_sdMutex = NULL;
struct WaitForWriteInfo {
  char* content;
  char* FilePath;
  size_t contentLength;
  fs::SDFS* SD_;
};
//! 大檔案讀寫專用變數


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

//TODO 尚需完善，目標: 分段小量多次寫入檔暗
void ExSDTask_WriteContent(void* parameter)
{ 
  WaitForWriteInfo* WaitForWriteInfo_ = (WaitForWriteInfo*)parameter;

  while (true) {
    if (xSemaphoreTake(u_sdMutex, (TickType_t)(100)) == pdTRUE) {
      ESP_LOGI("ExSD", "WriteContent GO");
      int blockSize = 1024*2;
      String RealFilePath = String(WaitForWriteInfo_->FilePath);
      String TempFilePath = RealFilePath+".temp";
      File writeFile = (*WaitForWriteInfo_).SD_->open(
        TempFilePath, FILE_WRITE
      );
      writeFile.close();
      for (int i = 0; i < WaitForWriteInfo_->contentLength; i += blockSize) {
        int bytesToWrite = min(blockSize, (int)(WaitForWriteInfo_->contentLength) - i);
        File writeFile = (*WaitForWriteInfo_).SD_->open(
          TempFilePath, FILE_APPEND
        );
        writeFile.write((uint8_t*)(WaitForWriteInfo_->content + i), bytesToWrite);
        writeFile.close();
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      (*WaitForWriteInfo_).SD_->remove(RealFilePath);
      (*WaitForWriteInfo_).SD_->rename(TempFilePath, RealFilePath);
      ESP_LOGI("ExSD", "WriteContent End");
      free((*WaitForWriteInfo_).content);
      free((*WaitForWriteInfo_).FilePath);
      free(WaitForWriteInfo_);
      xSemaphoreGive(u_sdMutex);
      vTaskDelete(NULL);
    }
    ESP_LOGI("ExSD", "WriteContent Wait");
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
inline void ExSD_ReWriteBigFile(fs::SDFS& SD_, String FilePath, String content)
{
  ExSD_CreateFile(SD_, FilePath);
  size_t stringLength = content.length() + 1;
  WaitForWriteInfo* WaitForWriteInfo_= (WaitForWriteInfo*)malloc(sizeof(WaitForWriteInfo));
  (*WaitForWriteInfo_).contentLength = stringLength;
  (*WaitForWriteInfo_).content = (char*)malloc(stringLength);
  strcpy((*WaitForWriteInfo_).content, content.c_str());

  (*WaitForWriteInfo_).FilePath = (char*)malloc(FilePath.length() + 1);
  strcpy((*WaitForWriteInfo_).FilePath, FilePath.c_str());

  (*WaitForWriteInfo_).SD_ = &SD_;
  if (u_sdMutex == NULL) {
    u_sdMutex = xSemaphoreCreateBinary();
    xSemaphoreGive(u_sdMutex);
  }
  xTaskCreatePinnedToCore(
    ExSDTask_WriteContent, "W_BIGFILE_SD",
    10000, WaitForWriteInfo_, 1, NULL
    , 1
  );

}

#endif