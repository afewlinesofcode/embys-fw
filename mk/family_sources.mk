# Select only the backend source directory for the configured MCU family.
# Shared implementation files outside hal/f1 and hal/f4 remain included.

include $(PROJECT_ROOT)/mk/toolchain/mcu.mk

SRC_DIR ?= src
ALL_APP_SRC := $(shell find $(SRC_DIR) -type f -name '*.cpp')
APP_SRC := $(filter-out $(SRC_DIR)/hal/f1/% $(SRC_DIR)/hal/f4/%,$(ALL_APP_SRC))
APP_SRC += $(filter $(SRC_DIR)/hal/$(HAL_FAMILY)/%,$(ALL_APP_SRC))
