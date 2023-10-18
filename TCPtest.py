from pyModbusTCP.client import ModbusClient

c = ModbusClient(host="192.168.20.220", port=1030, auto_open=True, auto_close=True, timeout=2)
# Address: 31000 , bit: 1x16 bit
s = c.read_holding_registers(0x1000, 8)
print(s)

##? IO09 ~ IO16 當前狀態
s = c.read_holding_registers(0x1040, 8)
print(s)

##? IO16 當前狀態
s = c.read_holding_registers(0x1047, 1)
print(s)


##? IO16 關閉
s = c.write_single_coil(7, 0)
c.read_coils(7,1)

s = c.write_single_coil(7, 1)
c.read_coils(7,1)

