#ifndef SCHEDULE_TASK_H
#define SCHEDULE_TASK_H

#include <Arduino.h>
#include <ArduinoJson.h>

enum BumpPoolWaterStatus {
  CLOSE,
  PUSH,
  PULL
};

class SSchedule_Task
{
  public:
    SSchedule_Task(void){};
    void RunBumpPoolWaterTask();
    

    int SW_BumpPoolWater = BumpPoolWaterStatus::CLOSE;

  private:
};
extern SSchedule_Task Schedule_Task;
#endif