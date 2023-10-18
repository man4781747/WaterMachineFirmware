
# 儀器傳出格式
>注意! 此為一定會有的項目，實際可能會有更多內容
```
{
  "device_no": "Test",                  // 儀器編號、名稱
  "mode": "Mode_Master",                // 儀器模式(主、從)
  "firmware_version": "V2.23.52.0",     // 韌體版本號
  "cmd": "PeristalticMotor",            // 指令類型
  "internet": {                         
    "remote": {                         // wifi連線 (若有的話)
      "ip": "192.168.20.45",
      "rssi": -40,
      "ssid": "IDWATER",
      "mac_address": "7C:DF:A1:E1:18:18"
    },
    "net": {                            // 有線網路連線 (若有的話)
      "ip": "192.168.20.45",
      "mac_address": "7C:DF:A1:E1:18:18"
    }
  },
  "time": "2023-05-12 10:51:49",         // 儀器發送出資訊的時間
  "utc": "+8",                           // 時區
  "action": {                            // 儀器指令行為
    "message": "找不到伺服馬達: 15151",   // 指令結果說明
    "status": "FAIL"                     // OK 或是 FAIL
  },
  "parameter": {
    // 各項回傳參數
  }
}
```

# 初次連線傳出  (2023-08-29 版本實際傳出)
```
{
  "device_no": "Akira-Zero",
  "mode": "Mode_Slave",
  "firmware_version": "V2.23.84.1",
  "cmd": "CONNECTED",
  "cmd_detail": "",
  "internet": {
    "remote": {
      "ip": "192.168.20.38",
      "rssi": -29,
      "mac_address": "F4:12:FA:E3:55:D0"
    }
  },
  "time": "2023-08-29 11:34:20",
  "utc": "+8",
  "action": {
    "message": "OK",
    "status": "CONNECTED"
  },
  "parameter": {},
  "device_status": "Busy"           // Busy代表儀器正在執行流程，Idle代表待機
}
```

# 儀器LOG資訊

```
{
  "device_no": "Akira-Zero",
  "mode": "Mode_Slave",
  "firmware_version": "V2.23.84.1",
  "cmd": "log",
  "internet": {
    "remote": {
      "ip": "192.168.20.38",
      "rssi": -29,
      "mac_address": "F4:12:FA:E3:55:D0"
    }
  },
  "time": "2023-08-29 12:03:45",
  "utc": "+8",
  "action": {
    "message": "執行事件: [清洗用] 抽取試劑水 (40秒) 至混合室",
    "status": "OK",
    "target": "Log",
    "method": "INFO",
    "desp": "流程: RO水清洗儀器"
  },
  "parameter": {},
  "cmd_detail": "LOG",
  "device_status": "Busy",
  "status": "OK"
}
```

# 可隨時輸入之指令
## [GET]/api/PoolData
```
{
  "device_no": "Akira-Zero",
  "mode": "Mode_Slave",
  "firmware_version": "V2.23.84.1",
  "cmd": "poolData",
  "cmd_detail": "[GET]/api/PoolData",
  "internet": {
    "remote": {
      "ip": "192.168.20.38",
      "rssi": -29,
      "mac_address": "F4:12:FA:E3:55:D0"
    }
  },
  "time": "2023-08-29 11:48:17",
  "utc": "+8",
  "action": {
    "message": "OK",
    "status": "OK",
    "target": "PoolData",
    "method": "Update"
  },
  "parameter": {
    "pool-1": {
      "PoolID": "pool-1",
      "PoolName": "蝦池1",
      "PoolDescription": "蝦池1sdf",
      "NO2_wash_volt": 30,
      "NO2_test_volt": 50890.65517,
      "NO2": -0.395006331,
      "NH4_wash_volt": 50044,
      "NH4_test_volt": 55924.89655,
      "NH4": -2.185424895,
      "pH_volt": 3289.172414,
      "pH": 8.98597931,
      "Data_datetime": "2023-08-29 10:00:56"
    },
    "pool-2": {
      "PoolID": "pool-2",
      "PoolName": "蝦池2",
      "PoolDescription": "蝦池2",
      "NO2_wash_volt": 50981.5,
      "NO2_test_volt": 30,
      "NO2": -0.079303731,
      "NH4_wash_volt": 50042,
      "NH4_test_volt": 50338.41379,
      "NH4": -0.061403081,
      "pH_volt": 2861.103448,
      "pH": 8.044227586,
      "Data_datetime": "2023-08-29 10:54:09"
    },
    "pool-3": {
      "PoolID": "pool-3",
      "PoolName": "蝦池3",
      "PoolDescription": "蝦池3",
      "NO2_wash_volt": 46747.42857,
      "NO2_test_volt": 70,
      "NO2": -0.057812045,
      "NH4_wash_volt": 50027,
      "NH4_test_volt": 25483.44828,
      "NH4": -0.056746731,
      "pH_volt": 2438.8,
      "pH": 7.11516,
      "Data_datetime": "2023-07-31 15:16:01"
    },
    "pool-4": {
      "PoolID": "pool-4",
      "PoolName": "蝦池4",
      "PoolDescription": "蝦池4",
      "NO2_wash_volt": 71,
      "NO2_test_volt": 71,
      "NO2": 2.918194055,
      "NH4_wash_volt": 50056,
      "NH4_test_volt": 50056,
      "NH4": 0.650233267,
      "pH_volt": 151.6785714,
      "pH": 333.4840267,
      "Data_datetime": "2023-07-31 15:23:56"
    }
  },
  "device_status": "Busy",
  "status": "OK"
}
```