#ifndef ExFile_FUNCTION_H
#define ExFile_FUNCTION_H
#include <esp_log.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

DynamicJsonDocument ExFile_listDir(fs::FS& fileSystem, String dir) {
  DynamicJsonDocument fileNameListItem(10000);
  if (fileSystem.exists(dir)) {
    File folder = fileSystem.open(dir.c_str());
    while (true) {
      File Entry = folder.openNextFile();
      if (! Entry) {
        break;
      }
      DynamicJsonDocument fileInfo(500);
      fileInfo["size"].set(Entry.size());
      fileInfo["name"].set(String(Entry.name()));
      fileInfo["getLastWrite"].set(Entry.getLastWrite());
      // Serial.println("===============");
      // Serial.println(Entry.name());
      // Serial.println(Entry.getLastWrite());
      // Serial.println(Entry.size());
      // fileNameListItem.add(String(Entry.name()));
      Entry.close();
      fileNameListItem.add(fileInfo);
    }
  }
  return fileNameListItem;
}

#endif