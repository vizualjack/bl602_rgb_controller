#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := custom_app
PROJECT_PATH := $(abspath .)
PROJECT_BOARD := evb
export PROJECT_PATH PROJECT_BOARD
#CONFIG_TOOLPREFIX :=

HTML = page.html
JS = script.js
HEADER = page.c
BUILD_COUNTER = build_counter
HTML_FILE_PATH := $(PROJECT_PATH)/$(PROJECT_NAME)/$(HTML)
JS_FILE_PATH := $(PROJECT_PATH)/$(PROJECT_NAME)/$(JS)
HEADER_FILE_PATH := $(PROJECT_PATH)/$(PROJECT_NAME)/$(HEADER)
BUILD_COUNTER_PATH := $(PROJECT_PATH)/$(PROJECT_NAME)/$(BUILD_COUNTER)
PREV_BUILD_COUNTER := $(shell [ -f $(BUILD_COUNTER_PATH) ] && cat $(BUILD_COUNTER_PATH) || echo 0)
CUR_BUILD_COUNTER = $(shell echo $$(($(PREV_BUILD_COUNTER) + 1)))


$(HEADER):
	$(shell echo $(CUR_BUILD_COUNTER) > $(BUILD_COUNTER_PATH))
	@echo "Build counter $(CUR_BUILD_COUNTER)"
	@echo "Generating $(HEADER) from $(HTML)"
	@echo 'const char *html_page = "\' > $(HEADER_FILE_PATH)
	@sed 's/"/\\"/g; s/$$/\\/' $(HTML_FILE_PATH) >> $(HEADER_FILE_PATH)
	@sed -i "s/%BUILD_COUNT%/$(CUR_BUILD_COUNTER)/" $(HEADER_FILE_PATH)
	@echo '";' >> $(HEADER_FILE_PATH)
	@echo 'const char *js_script = "\' >> $(HEADER_FILE_PATH)
	@sed 's/"/\\"/g; s/$$/\\/' $(JS_FILE_PATH) >> $(HEADER_FILE_PATH)
	@echo '";' >> $(HEADER_FILE_PATH)

-include ./proj_config.mk

ifeq ($(origin BL60X_SDK_PATH), undefined)
BL60X_SDK_PATH_GUESS ?= $(shell pwd)
BL60X_SDK_PATH ?= $(BL60X_SDK_PATH_GUESS)/../..
$(info ****** Please SET BL60X_SDK_PATH ******)
$(info ****** Trying SDK PATH [$(BL60X_SDK_PATH)])
endif

COMPONENTS_NETWORK := sntp dns_server
COMPONENTS_BLSYS   := bltime blfdt blmtd blota bloop loopadc looprt loopset
COMPONENTS_VFS     := romfs

INCLUDE_COMPONENTS += freertos_riscv_ram bl602 bl602_std bl602_wifi bl602_wifidrv hal_drv lwip lwip_dhcpd mbedtls vfs yloop utils cli aws-iot httpc netutils blog blog_testc
INCLUDE_COMPONENTS += easyflash4
INCLUDE_COMPONENTS += $(COMPONENTS_NETWORK)
INCLUDE_COMPONENTS += $(COMPONENTS_BLSYS)
INCLUDE_COMPONENTS += $(COMPONENTS_VFS)
INCLUDE_COMPONENTS += $(PROJECT_NAME)
include $(BL60X_SDK_PATH)/make_scripts_riscv/project.mk

