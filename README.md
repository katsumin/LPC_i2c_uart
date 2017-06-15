# LPC_i2c_uart
- I2C-UART Convertor on LPC810
  - Confirmed with Raspberry Pi 3

# Build
- LPCXpresso IDE
- Require projects (need to import on workspace these projects)
  - CMSIS_CORE_LPC8xx
  - lpc800_driver_lib

# Usage
- Command

|I2C Address|R/W|Action|
|:-:|:-:|:--|
|0x40|R|UART Receive Count|
||W|UART Initialize(see the table below.)|
|0x41|R|UART Data Receive|
||W|UART Data Send|

- UART Initialize
  - 8bit, Non Parity, StopBit1(fixed)

|code|bitrate|
|---|--:|
|0x00|300bps|
|0x01|600bps|
|0x02|1200bps|
|0x03|2400bps|
|0x04|4800bps|
|0x05|9600bps|
|0x06|19200bps|
|0x07|38400bps|
|0x08|57600bps|
|0x09|115200bps|


```python
import smbus
i2c = smbus.SMBus(1)
i2c.write_byte(0x40, 0x07) # Initialize(38400bps)
i2c.write_byte(0x41, 0x55) # Data(0x55) Send
count = i2c.read_byte(0x40) # UART Recieve Count
data = i2c.read_byte(0x41) # UART Receive Data
```

# Sample
- i2cdetect
```
pi@raspberrypi:~ $ sudo i2cdetect -y 1
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: 40 41 -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- --                         
pi@raspberrypi:~ $
```

- Python(i2c_test.py)

```
2017/06/15 20:33:24.093166 << (26)
2017/06/15 20:33:24.200640 << (26)
2017/06/15 20:33:24.308111 << (26)
2017/06/15 20:33:24.415586 << (26)
2017/06/15 20:33:24.523099 << (26)
2017/06/15 20:33:24.630604 << (26)
2017/06/15 20:33:24.738076 << (26)
2017/06/15 20:33:24.845614 << (26)
2017/06/15 20:33:24.953105 << (26)
2017/06/15 20:33:25.060603 << (26)
2017/06/15 20:33:25.168058 << (26)
2017/06/15 20:33:25.275573 << (26)

receive exit
receive 0 less 26.
receive exit
send exit
pi@raspberrypi:~ $
```
