#ifndef ExFile_FUNCTION_H
#define ExFile_FUNCTION_H
#include <esp_log.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

inline DynamicJsonDocument ExFile_listDir(fs::FS& fileSystem, String dir) {
  DynamicJsonDocument fileNameListItem(1000);
  if (fileSystem.exists(dir)) {
    File folder = fileSystem.open(dir.c_str());
    while (true) {
      File Entry = folder.openNextFile();
      if (! Entry) {
        break;
      }
      fileNameListItem.add(String(Entry.name()));
      Entry.close();
    }
  }
  return fileNameListItem;
}

#endif