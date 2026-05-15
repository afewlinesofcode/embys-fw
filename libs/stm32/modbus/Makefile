TC ?= arm
.DEFAULT_GOAL := all

APP_NAME := stm32-modbus
PROJECT_ROOT := $(realpath $(shell pwd)/../../..)
WORK_DIR := /work/libs/stm32/modbus
BUILD_DIR := $(PROJECT_ROOT)/build
INCLUDE_DIR := $(BUILD_DIR)/include/embys/stm32/modbus
INCLUDES += -Iinclude -I$(PROJECT_ROOT)/build/include
LDLIBS =

ifeq ($(TC),sim)
	LDLIBS = -lstm32-sim
	INCLUDES += -I$(PROJECT_ROOT)/build/include_sim
endif

include $(PROJECT_ROOT)/mk/toolchain/$(TC).mk
include $(PROJECT_ROOT)/mk/docker.mk

APP_HEADERS = $(shell find $(SRC_DIR) -name "*.hpp")
APP_INSTALLED_HEADERS = $(patsubst $(SRC_DIR)/%.hpp,$(INCLUDE_DIR)/%.hpp,$(APP_HEADERS))

$(INCLUDE_DIR)/%.hpp: $(SRC_DIR)/%.hpp
	@mkdir -p $(dir $@)
	@echo "Installing header: $@"
	@cp $< $@

all: .lib-in-docker $(APP_INSTALLED_HEADERS)

clean:
	rm -rf $(TARGET_LIB) $(OBJ_DIR) $(APP_INSTALLED_HEADERS)
