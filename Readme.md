
# 儀器傳出格式
```
{
  "time":"2023-3-23 17:29:52",  //時間
  "device_no":"ID_Feeder",      //機台編號
  "mode":"Mode_Slave",          //主/被動模式
  "cmd":"Set_Devno",            //使用者輸入指令
  "message":"OK",               //訊息(執行狀況)
  "wifi":{                      //網路資訊
    "ip":"192.168.20.39",
    "rssi":-67,                 //訊號強度
    "mac_address":"C8:F0:9E:31:8F:30"
  },
  "parameter":{
    // 各項回傳參數
  }
}
```

# 可隨時輸入之指令
## "Reset"
```
```
## "Stop"
```
```
## "GetLatestInformation"
```
{
  "poolsCount": N,                             // 蝦池數量
  "poolsData": {
    "pool-N": {
      "PoolID": "pool-ID",                     // 蝦池名稱
      "Data_datetime": "2023-04-14 16:46:13",  // 資料更新時間
      "NO2_wash_volt": 1.7,                    // 亞硝酸鹽清洗電壓
      "NO2_test_volt": 1.68,                   // 亞硝酸鹽測試電壓
      "NO2": 8.72,                             // 亞硝酸鹽濃度
      "NH4_wash_volt": 0.43,                   // 氨氮清洗電壓
      "NH4_test_volt": 103.68,                 // 氨氮測試電壓
      "NH4": 2.29,                             // 氨氮濃度
      "pH_volt": 857.65,                       // 酸鹼電壓
      "pH": 8.41,                              // 酸鹼值
      "salinity_volt": 411.63,                 // 鹽度電壓
      "salinity": 654.26,                      // 鹽度濃度
      "Temperature": 23.9,                     // 溫度
      "DO_percentage": 22.59,                  // 溶氧百分比
      "DO": 0.67,                              // 溶氧濃度
      "residual_chlorine": 93.18,              // 餘氯
      "ca_mg": 38.98,                          // 鈣鎂離子濃度
      "vibrio": 787.34                         // 弧菌數
    }, ... 
  }
}
```
## "GetLatestInformation?<PoolID>"
> 獲得指定蝦池資訊
```
{
  "PoolID": "pool-ID",                     // 蝦池名稱
  "Data_datetime": "2023-04-14 16:46:13",  // 資料更新時間
  "NO2_wash_volt": 1.7,                    // 亞硝酸鹽清洗電壓
  "NO2_test_volt": 1.68,                   // 亞硝酸鹽測試電壓
  "NO2": 8.72,                             // 亞硝酸鹽濃度
  "NH4_wash_volt": 0.43,                   // 氨氮清洗電壓
  "NH4_test_volt": 103.68,                 // 氨氮測試電壓
  "NH4": 2.29,                             // 氨氮濃度
  "pH_volt": 857.65,                       // 酸鹼電壓
  "pH": 8.41,                              // 酸鹼值
  "salinity_volt": 411.63,                 // 鹽度電壓
  "salinity": 654.26,                      // 鹽度濃度
  "Temperature": 23.9,                     // 溫度
  "DO_percentage": 22.59,                  // 溶氧百分比
  "DO": 0.67,                              // 溶氧濃度
  "residual_chlorine": 93.18,              // 餘氯
  "ca_mg": 38.98,                          // 鈣鎂離子濃度
  "vibrio": 787.34                         // 弧菌數
}
```