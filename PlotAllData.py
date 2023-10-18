import requests
import datetime 
import re
import matplotlib.pyplot as plt
import numpy as np


S_BaseURL = "http://192.168.20.38/static/SD/datas/{}_data.csv"

DT_today = datetime.datetime.now()


# for i in range(30):
#     DT_dayChose = DT_today - datetime.timedelta(days=i)
#     S_date = DT_dayChose.strftime("%Y%m%d")
#     data = requests.get(S_BaseURL.format(S_date))
#     with open("./SensorData/{}_data.csv".format(S_date), "w", encoding="utf-8-sig") as f:
#         data.encoding = 'utf-8-sig'
#         f.write(data.text)

D_data = {}

S_reString = r"(?P<datetime>[0-9: -]*), .*?,(?P<poolnum>[0-9-a-z]*),.*?,.*?,(?P<type>[0-9-a-z_A-Z]*),.*?,(?P<value>[0-9.]*),"

for i in range(6):
    DT_dayChose = DT_today - datetime.timedelta(days=i)
    S_date = DT_dayChose.strftime("%Y%m%d")
    data = requests.get(S_BaseURL.format(S_date))
    with open("./SensorData/{}_data.csv".format(S_date), "r", encoding="utf-8-sig") as f:
        for S_lineString in f.readlines():
            RE_match = re.search(S_reString, S_lineString)
            if RE_match == None:
                continue
            DT_time = datetime.datetime.strptime(RE_match["datetime"], "%Y-%m-%d %H:%M:%S")
            S_pool = RE_match["poolnum"]
            S_type = RE_match["type"]
            F_value = float(RE_match["value"])
            if D_data.get(S_pool, None) == None:
                D_data[S_pool] = {}
            if D_data[S_pool].get(S_type, None) == None:
                D_data[S_pool][S_type] = {
                    "value": [],
                    "time": []
                }
            D_data[S_pool][S_type]["value"].append(F_value)
            D_data[S_pool][S_type]["time"].append(DT_time)


plt.plot(
  D_data["pool-1"]["NO2_wash_volt"]["time"], 
  D_data["pool-1"]["NO2_wash_volt"]["value"],'.'
)
plt.plot(
  D_data["pool-1"]["NO2_test_volt"]["time"], 
  D_data["pool-1"]["NO2_test_volt"]["value"],'.'
)

plt.plot(
  D_data["pool-1"]["NH4_wash_volt"]["time"], 
  D_data["pool-1"]["NH4_wash_volt"]["value"],'.'
)
plt.plot(
  D_data["pool-1"]["NH4_test_volt"]["time"], 
  D_data["pool-1"]["NH4_test_volt"]["value"],'.'
)


plt.legend(loc='best')
plt.show()