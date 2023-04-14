#include "Schedule_Task.h"
#include <ArduinoJson.h>

TaskHandle_t TASK_BumpPoolWater;



void SSchedule_Task::RunBumpPoolWaterTask()
{
  xTaskCreate(
    Task_BumpPoolWater, "Task_BumpPoolWater",
    10000, NULL, 1, &TASK_BumpPoolWater
  );
}

void Task_BumpPoolWater(void * parameter)
{
  for (;;) {
    switch (Schedule_Task.SW_BumpPoolWater)
    {
    case BumpPoolWaterStatus::PUSH:
      break;
    case BumpPoolWaterStatus::PULL:
      break;
    default:
      break;
    }
    vTaskDelay(1);
  }
}