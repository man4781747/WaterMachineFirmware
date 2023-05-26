//https://optoelectronics.liteon.com/upload/download/DS86-2014-0006/LTR-329ALS-01_DS_V1.pdf
#ifndef LTR_329ALS_01_H
#define LTR_329ALS_01_H

#include <Arduino.h>
#include <Wire.h>

struct ALS_01_Data_t {
  uint16_t CH_1=0;
  uint16_t CH_0=0;
};

/**
 * @brief 3位有效
 * 
 */
enum ALS_Gain : uint8_t {
  Gain_1X = 0b000,
  Gain_2X = 0b001,
  Gain_4X = 0b010,
  Gain_8X = 0b011,
  Gain_48X = 0b110,
  Gain_96X = 0b111,
};

struct ALS_Contr_t {
  uint8_t ALS_Gain = ALS_Gain::Gain_1X;
  bool SW_Reset = false;
  bool ALS_mode = true;
};

enum ALS_integration_time : uint8_t {
  T_100ms = 0b000,
  T_50ms  = 0b001,
  T_200ms = 0b010,
  T_400ms = 0b011,
  T_150ms = 0b100,
  T_250ms = 0b101,
  T_300ms = 0b110,
  T_350ms = 0b111
};

enum ALS_measurement_rate : uint8_t {
  Meas_Rate_50ms   = 0b000,
  Meas_Rate_100ms  = 0b001,
  Meas_Rate_200ms  = 0b010,
  Meas_Rate_500ms  = 0b011,
  Meas_Rate_1000ms = 0b100,
  Meas_Rate_2000ms = 0b101,
};

struct ALS_Meas_Rate_t {
  uint8_t ALS_integration_time = ALS_integration_time::T_100ms;
  uint8_t ALS_measurement_rate = ALS_measurement_rate::Meas_Rate_500ms;
};


class CLTR_329ALS_01
{
  public:
    CLTR_329ALS_01(TwoWire* TwoWire_in){I2CWire = TwoWire_in;};
    void Set_ALS_Contr(ALS_Contr_t config);
    void Set_ALS_Meas_Rate(ALS_Meas_Rate_t config);

    void Set_Meas_Rate(ALS_measurement_rate value);

    bool IfNewData();

    ALS_01_Data_t TakeOneValue();
    ALS_01_Data_t GetData();

    ALS_Meas_Rate_t ALS_Meas_Rate_Config;
    ALS_Contr_t ALS_Contr_Config;
    int addr = 0x29;
    TwoWire* I2CWire;
  private:
};

class CMULTI_LTR_329ALS_01
{
  public:
    CMULTI_LTR_329ALS_01(int data_, int shcp_, int stcp_, TwoWire* Wire_){
      dataPin = data_;
      shcpPin = shcp_;
      stcpPin = stcp_;
      I2CWire = Wire_;
      pinMode(dataPin, OUTPUT);
      pinMode(stcpPin, OUTPUT);
      pinMode(shcpPin, OUTPUT);
      LTR_329ALS_01 = new CLTR_329ALS_01(Wire_);
    };
    void openSensorByIndex(int index);
    void closeSensorByIndex(int index);
    void closeAllSensor();
    void SetGain(ALS_Gain gain);
    ALS_01_Data_t TakeOneValue();
    
    int dataPin, shcpPin, stcpPin;
    TwoWire* I2CWire;
    CLTR_329ALS_01* LTR_329ALS_01;
  private:
};

#endif