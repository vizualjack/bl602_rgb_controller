## 🔴🟢🔵 RGB/W Controller Firmware for BL602 🔴🟢🔵
### 📸 Screenshots of the Web UI
<img src="doc_images/1.png" alt="Screenshot 1" width="300"> <img src="doc_images/2.png" alt="Screenshot 2" width="300">

### 📝 Features
- Control RGB/W strip via HTTP (REST) or UDP ([API details](server_defintions.md))
- Flexible pin-to-color mapping
- Configurable device hostname (e.g. visible in your router)
- Wi-Fi configuration
- Built-in Wi-Fi hotspot for initial setup
- Over-the-air (OTA) firmware updates


## 📥 Downloading the firmware
Several firmware formats are available in the [Releases](https://github.com/vizualjack/bl602_rgb_controller/releases) section, depending on your board, firmware and flashing method.

## ⚙️ Installing onto your device via UART (Windows)
📋 **Requirements**: [Downloading the firmware (.bin)](#-downloading-the-firmware)<br/>
### 1. 🔌 **Connecting the board**
**Required pins (and state):** BOOT (**HIGH**), RX, TX, GROUND
#### 📱 **Pine64 BL602 EVB ver 1.1** · **Pine64 Pinenut-01S** · **Boufallo Lab BL602 Dev Module**
Refer to the official documentation [here](https://pine64.github.io/bl602-docs/Quickstart_Guide/connecting_the_hardware.html).<br/>
#### 📱 **Other boards**
Pin locations are usually labeled directly on the board, or available via onboard connectors (e.g. USB-C) and boot switches.
Depending on the board, you may also need additional hardware such as a USB-to-serial adapter.

Please consult the documentation of your specific board for details.

### 2. 🛠️ **Set up the flash tool**
The flash tool can be found here: `.../this/repo/tools/flash_tool`<br/>
Use the following **recommended flash configuration**:<br/>
<img src="doc_images/flash_tool_config.png" alt="Config screenshot" width="250"><br/>
The only setting you need to change is the **COM Port**.<br/>
You can find the correct port in **Device Manager**.<br/>
The device will appear under **Ports (COM & LPT)** when connected to your PC.

### 3. 🚀 **Flashing**
Once everything is connected and the **BOOT** pin is set to **HIGH**, power up the device and click **Create & Download** in the flash tool.


## 🔄 Updating the firmware
📋 **Requirements**: [Downloading the firmware (.bin.xz)](#-downloading-the-firmware)<br/><br/>
For firmware updates, using the **OTA method** is recommended.<br/>
It is available under the **Firmware Upload** section in the Web UI and uses the **.bin.xz** format.


## 💻 Development
⚠️ **Avoid whitespaces in path names. This may cause build issues**
### 📦 **Building the firmware**
📋 **Requirements**: [cmake](https://cmake.org/)
#### **Windows**
	cd ...\this\repo\customer_app\custom_app
	build.bat
#### **Linux / MSYS**
	cd .../this/repo/customer_app/custom_app
	./build
📄 **Output firmware file**
```
.../this/repo/customer_app/custom_app/build_out/custom_app.bin
```

### 📦 **Building the OTA versions of the firmware**
📋 **Requirements**: [Python](https://www.python.org/downloads/), [Building the firmware](#-building-the-firmware)
#### **Windows**
	cd ...\this\repo\customer_app\custom_app
	build_ota.bat
#### **Linux / MSYS**
	cd .../this/repo/customer_app/custom_app
	./build_ota
📄 **Output directory** 
```
.../this/repo/customer_app/custom_app/build_out/ota/dts40M_pt2M_boot2release_c84015
```