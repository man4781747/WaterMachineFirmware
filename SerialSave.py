import serial
import datetime

COM_PORT = 'COM5'    # 指定通訊埠名稱
BAUD_RATES = 115200    # 設定傳輸速率
ser = serial.Serial(COM_PORT, BAUD_RATES)   # 初始化序列通訊埠

try:
    while True:
        while ser.in_waiting:          # 若收到序列資料…
            data_raw = ser.readline()  # 讀取一行
            try:
                data = data_raw.decode()   # 用預設的UTF-8解碼
                if data[-1] == '\n':
                    data = data[:-1]
            except:
                data = str(data_raw)

            S_fileName = "{}.log".format(datetime.datetime.now().strftime("%Y%m%d"))
            S_fullFilePath = "./SerialLogs/{}".format(S_fileName)
            S_logContent = "{},{}\n".format(
                datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                data
            )
            with open(S_fullFilePath, "a+", encoding='utf-8') as f:
                f.write(S_logContent)
            print(S_logContent)


            # print('接收到的原始資料：', data_raw)
            # print('接收到的資料：', data)

except KeyboardInterrupt:
    ser.close()    # 清除序列通訊物件
    print('再見！')