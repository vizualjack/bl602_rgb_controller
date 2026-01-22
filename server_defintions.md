# 🌐 HTTP / REST Server
🔗 **Port: 80**
### 🎯 GET /
Normal web page to configure the device.

### 🎯 POST /new_duty
To change the color.<br/>
Value range for all parameters: 0 - 100
#### JSON body:
```
{
    "r": 12.34,
    "g": 10.3,
    "b": 50.2,
    "w": 40.40
}
```

# 🔌 UDP Server
🔗 **Port: 1337**<br/>
This server supports **only one packet** and it's to **change the color**.<br/>
## 📦 Packet definition
All values are in little endian style.<br/>
Value range for all parameters: 0 - 100<br/>
Total packet size: 16 bytes

		RED - float (4 bytes)
		GREEN - float (4 bytes)
		BLUE - float (4 bytes)
		WHITE - float (4 bytes)

#### Python example:
```python
import socket
import struct

UDP_IP = "XXX.XXX.XXX.XXX"  # Replace with the target device IP
UDP_PORT = 1337

red = 12.34
green = 10.3
blue = 50.2
white = 40.40

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
data = struct.pack(
	"<4f",
	*(red, green, blue, white),
)
sock.sendto(data, (UDP_IP, UDP_PORT))
```
