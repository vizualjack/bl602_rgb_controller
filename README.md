# RGB/W Controller Firmware for BL602
### Web UI
![Screenshot 1](doc_images/1.png)
![Screenshot 2](doc_images/2.png)

### Features
- Control RGB/W stripe via http / rest or udp server
- Pin to color mapping
- Set device hostname (e.g. used in the router)
- Set wifi settings
- Create a wifi hotspot for initial configuration
- Updates via OTA

### Install onto your device
You can find the several firmware formats in the release tab, depending on your board, firmware and the method you wanna use.

For further help you can take a look at [this documentation](https://pine64.github.io/bl602-docs/).

The flash tool can also be found here: .../this/repo/tools/flash_tool

This is the recommended flash configuration:
![alt text](doc_images/flash_tool_config.png)

### Updating the firmware
If you wanna update this firmware on you device i would recommend the OTA method which uses the .bin.xz format.


## Development
### Building the firmware
Requirements: [cmake](https://cmake.org/)

!! Whitespaces in your path / folder names could be mess up the building !!

Windows:
- 
	cd ...\this\repo\customer_app\custom_app
	build.bat

Linux / Msys:
- 
	cd .../this/repo/customer_app/custom_app
	./build
Output firmware file: .../this/repo/customer_app/custom_app/build_out/custom_app.bin

### Building the OTA versions of the firmware
Requirements: Pythom, Building the firmware

!! Whitespaces in your path / folder names could be mess up the building !!

Windows:
- 
	cd ...\this\repo\customer_app\custom_app
	build_ota.bat

Linux / Msys:
- 
	cd .../this/repo/customer_app/custom_app
	./build_ota
Output directory: .../this/repo/customer_app/custom_app/build_out/ota/dts40M_pt2M_boot2release_c84015