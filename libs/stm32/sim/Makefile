TC := sim
.DEFAULT_GOAL := all
APP_NAME := stm32-sim
PROJECT_ROOT := $(realpath $(shell pwd)/../../..)
WORK_DIR := /work/libs/stm32/sim
BUILD_DIR := $(PROJECT_ROOT)/build
BIN_DIR := $(BUILD_DIR)/$(TC)
INCLUDE_DIR := $(BUILD_DIR)/include/embys/stm32/sim
INCLUDES = -Iinclude

include $(PROJECT_ROOT)/mk/toolchain/$(TC).mk
include $(PROJECT_ROOT)/mk/docker.mk

APP_HEADERS = $(shell find $(SRC_DIR) -name "*.hpp")
APP_INSTALLED_HEADERS = $(patsubst $(SRC_DIR)/%.hpp,$(INCLUDE_DIR)/%.hpp,$(APP_HEADERS))
APP_INSTALLED_HEADERS += $(BUILD_DIR)/include_sim/arm_acle.h

$(INCLUDE_DIR)/%.hpp: $(SRC_DIR)/%.hpp
	@mkdir -p $(dir $@)
	@echo "Installing header: $@"
	@cp $< $@

$(BUILD_DIR)/include_sim/arm_acle.h: include/arm_acle.h
	@mkdir -p $(dir $@)
	@echo "Installing header: $@"
	@cp $< $@

all: .lib-in-docker $(APP_INSTALLED_HEADERS)

clean:
	rm -rf $(TARGET_LIB) $(OBJ_DIR) $(APP_INSTALLED_HEADERS) $(BUILD_DIR)/include_sim/arm_acle.h
