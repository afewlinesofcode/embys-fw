TC ?= arm
.DEFAULT_GOAL := all

APP_NAME := stm32-gpio
PROJECT_ROOT := $(realpath $(shell pwd)/../../..)
WORK_DIR := /work/libs/stm32/gpio
BUILD_DIR := $(PROJECT_ROOT)/build
BIN_DIR := $(BUILD_DIR)/$(TC)
INCLUDE_DIR := $(BUILD_DIR)/include/embys/stm32/gpio
INCLUDES += -Iinclude -I$(PROJECT_ROOT)/build/include
LDLIBS = -lstm32-base

ifeq ($(TC),sim)
	LDLIBS = -lstm32-sim
# The following include path is needed to provide arm_acle.h for the simulation toolchain
	INCLUDES += -I$(PROJECT_ROOT)/build/include_sim
endif

# Select the family-specific GPIO HAL. Sim always uses F1 (it simulates F1 hardware).
MCU ?= stm32f103xb

ifeq ($(TC),sim)
  HAL_FAMILY := f1
else
  HAL_FAMILY := $(shell echo $(MCU) | cut -c6-7)
endif

# Explicitly list sources so the toolchain's APP_SRC ?= find(...) is preempted.
# This avoids picking up all hal/<family>/ variants.
# SRC_DIR defaults to 'src' in the toolchain makefiles; use that literal here.
APP_SRC := src/hal.cpp \
           src/bus.cpp \
           src/pin.cpp \
           src/hal/$(HAL_FAMILY)/hal.cpp

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

