# 離岸式水質機台WebSocket API格式紀錄

由於使用HTTP通訊協議時，會需要事先得知主機之IP與Port，導致各機器會需要多好幾項參數來紀錄這些參數，增加儀器管理的困難度，因此當前採用WebSocket通訊協議，讓每台機器都成為Server主機，讓NodeRed以及各網頁做為客戶端來與儀器連線，如此一來所有儀器的IP位置皆可以統一由單一個NodeRed來做紀錄與管理，儀器本身不需要紀錄任何其他外部服務的位置資訊，只需等待其他使用者連線進來即可

## 儀器訊息

儀器發出之訊息皆為 UTF-8 編碼的 JSON 格式文字，本文中以Javascript的角度來描述此資料，並以 DATA 代稱此資料

## 儀器發出之訊息通用格式
>注意! 此為基礎內容，實際可能會有更多內容

資料內容範例:
```
{
  "device_no": "Test",                  // 儀器編號、名稱
  "mode": "Mode_Master",                // 儀器模式(主、從)
  "firmware_version": "V2.23.52.0",     // 韌體版本號
  "cmd": "test",            // 指令類型
  "internet": {                         
    "remote": {                         // wifi連線資訊 (若有的話)
      "ip": "192.168.20.45",
      "rssi": -40,
      "ssid": "IDWATER",
      "mac_address": "7C:DF:A1:E1:18:18"
    },
    "net": {                            // 有線網路連線資訊 (若有的話)
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
其中
* DATA.cmd 為指令類型  ***最為重要***
* DATA.device_no 為儀器編號、名稱
* DATA.mode 為儀器模式(主、從)  *目前離岸式水質機尚無用到此參數
* DATA.firmware_version 為儀器韌體版本號
* DATA.internet 為 儀器連線的資訊
* DATA.time 為 儀器連線的資訊
* DATA.action 為 儀器指令行為
* DATA.parameter 為 各項回傳參數

# 儀器廣播之訊息列表
## 儀器連線成功 (DATA.cmd === "CONNECTED")
* 觸發場合: 客戶端與儀器成功連線後，儀器主動發出
* 訊息對像: 只有該客戶端收的到

