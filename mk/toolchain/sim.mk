CC     := gcc
CXX    := g++
LD     := g++
SIZE   := size
AR	   := ar
RANLIB := ranlib

APP_NAME     ?= embys-fw
PROJECT_ROOT ?= $(shell pwd)

include $(PROJECT_ROOT)/mk/toolchain/mcu.mk

SRC_DIR       ?= src
BUILD_DIR     ?= build
BIN_DIR       ?= $(BUILD_DIR)/sim/$(ARCH_NAME)
OBJ_DIR       = $(BIN_DIR)/obj/$(APP_NAME)
APP_OBJ_DIR   = $(OBJ_DIR)/app
BUILD_LIB_DIR = $(PROJECT_ROOT)/build/sim/$(ARCH_NAME)

APP_SRC      ?= $(shell find $(SRC_DIR) -type f -name '*.cpp')
APP_OBJ      := $(patsubst $(SRC_DIR)/%.cpp,$(APP_OBJ_DIR)/%.o,$(APP_SRC))

TARGET_LIB   := $(BIN_DIR)/lib$(APP_NAME).a
TARGET       := $(BIN_DIR)/$(APP_NAME)

INCLUDES     += -I$(PROJECT_ROOT)/build/include \
							  -I$(PROJECT_ROOT)/third_party/CMSIS_6/CMSIS/Core/Include \
							  -I$(CMSIS_DEV)
CPPFLAGS     += $(INCLUDES) $(DEFINES) $(MCU_DEFINES) -DSTM32_SIM
CXXFLAGS     += -Wall -Wextra -Werror -std=c++20 -O0 -g3 -MMD -MP
LDFLAGS      += -L$(BUILD_LIB_DIR)

TARGET_DEPS  += $(APP_OBJ)

$(APP_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(TARGET): $(TARGET_DEPS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(TARGET_LIB): $(APP_OBJ)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	$(RANLIB) $@

.size: $(TARGET)
	$(SIZE) -A $<

.lib: $(TARGET_LIB)

.bin: $(TARGET)

.run: $(TARGET)
	@echo "Running simulation..."
	$<

-include $(APP_OBJ:.o=.d)
