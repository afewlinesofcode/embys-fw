TC ?= arm
.DEFAULT_GOAL := all

APP_NAME     := stm32-common
PROJECT_ROOT := $(realpath $(shell pwd)/../../..)
WORK_DIR     := /work/libs/stm32/common
BUILD_DIR    := $(PROJECT_ROOT)/build
BIN_DIR      := $(BUILD_DIR)/$(TC)
SRC_DIR      := .
APP_SRC      := ./debug.cpp
INCLUDE_DIR  := $(BUILD_DIR)/include/embys/stm32
INCLUDES     += -Iinclude -I$(PROJECT_ROOT)/build/include

ifeq ($(TC),sim)
	LDLIBS = -lstm32-sim
	INCLUDES += -I$(PROJECT_ROOT)/build/include_sim
endif

include $(PROJECT_ROOT)/mk/toolchain/$(TC).mk
include $(PROJECT_ROOT)/mk/docker.mk

APP_HEADERS = $(shell find include -name "*.hpp")
APP_INSTALLED_HEADERS = $(patsubst include/%,$(INCLUDE_DIR)/%,$(APP_HEADERS))

$(INCLUDE_DIR)/%.hpp: include/%.hpp
	@mkdir -p $(dir $@)
	@echo "Installing header: $@"
	@cp $< $@

.PHONY: all clean

all: .lib-in-docker $(APP_INSTALLED_HEADERS)

clean:
	rm -rf $(TARGET_LIB) $(OBJ_DIR) $(APP_INSTALLED_HEADERS)
