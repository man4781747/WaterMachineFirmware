#include "LTR_329ALS_01.h"
#include <Wire.h>

void print_binary(uint8_t num, int len) {
  for (int i = len-1; i >= 0; i--) {
    Serial.print((num >> i) & 1, BIN);
  }
}

void insetBit(uint8_t *configBits, uint8_t insertFrom, int Start, int len)
{
  for (int index = len-1; index>=0;index--) {
    *configBits |= (((insertFrom >> index) & 1) << Start+index);
  }
}

void CLTR_329ALS_01::Set_ALS_Contr(ALS_Contr_t config)
{
  uint8_t configBit = 0;
  insetBit(&configBit, config.ALS_mode, 0, 1);
  insetBit(&configBit, config.SW_Reset, 1, 1);
  insetBit(&configBit, config.ALS_Gain, 2, 3);
  // Serial.print("configBit: ");
  // print_binary(configBit, 8);
  // Serial.println("");
  I2CWire->beginTransmission(addr);
  I2CWire->write(0x80);
  I2CWire->write(configBit);
  I2CWire->endTransmission();
  delay(10);
  I2CWire->requestFrom(0x29, 1);
  Serial.print("ALS_CTRL: ");
  print_binary(I2CWire->read(), 8);
  Serial.println("");
}

void CLTR_329ALS_01::Set_ALS_Meas_Rate(ALS_Meas_Rate_t config)
{
  uint8_t configBit = 0;
  insetBit(&configBit, config.ALS_measurement_rate, 0, 3);
  insetBit(&configBit, config.ALS_integration_time, 3, 3);
  I2CWire->beginTransmission(addr);
  I2CWire->write(0x85);
  I2CWire->write(configBit);
  I2CWire->endTransmission();
  delay(10);
  I2CWire->requestFrom(0x29, 1);
  Serial.print("configBit: ");
  print_binary(configBit, 8);
  Serial.println("");
}

ALS_01_Data_t CLTR_329ALS_01::GetData()
{
  ALS_01_Data_t returnData;

  I2CWire->beginTransmission(0x29);
  I2CWire->write(0x89);
  I2CWire->endTransmission();
  delay(1);
  I2CWire->requestFrom(0x29, 1);
  uint8_t CH1_high_byte = I2CWire->read();

  I2CWire->beginTransmission(0x29);
  I2CWire->write(0x88);
  I2CWire->endTransmission();
  delay(1);
  I2CWire->requestFrom(0x29, 1);
  uint8_t CH1_low_byte = I2CWire->read();
  returnData.CH_1 = ((uint16_t)CH1_high_byte << 8) | CH1_low_byte;


  I2CWire->beginTransmission(0x29);
  I2CWire->write(0x8B);
  I2CWire->endTransmission();
  delay(1);
  I2CWire->requestFrom(0x29, 1);
  uint8_t CH0_high_byte = I2CWire->read();

  I2CWire->beginTransmission(0x29);
  I2CWire->write(0x8A);
  I2CWire->endTransmission();
  delay(1);
  I2CWire->requestFrom(0x29, 1);
  uint8_t CH0_low_byte = I2CWire->read();
  returnData.CH_0 = ((uint16_t)CH0_high_byte << 8) | CH0_low_byte;

  return returnData;
}

ALS_01_Data_t CLTR_329ALS_01::TakeOneValue()
{
  ALS_Contr_Config.SW_Reset = true;
  ALS_Contr_Config.ALS_mode = true;
  Set_ALS_Contr(ALS_Contr_Config);
  delay(10);
  ALS_Contr_Config.SW_Reset = false;
  ALS_Contr_Config.ALS_mode = true;
  Set_ALS_Contr(ALS_Contr_Config);
  Set_ALS_Meas_Rate(ALS_Meas_Rate_Config);
  while (IfNewData() == false){
    delay(10);
  } 
  ALS_01_Data_t returnData = GetData();
  ALS_Contr_Config.SW_Reset = true;
  ALS_Contr_Config.ALS_mode = true;
  Set_ALS_Contr(ALS_Contr_Config);


  return returnData;
}

bool CLTR_329ALS_01::IfNewData()
{
  I2CWire->beginTransmission(0x29);
  I2CWire->write(0x8C);
  I2CWire->endTransmission();
  delay(1);
  I2CWire->requestFrom(0x29, 1);
  uint8_t AllSataus = I2CWire->read();
  return (AllSataus >> 2) & 1;
}