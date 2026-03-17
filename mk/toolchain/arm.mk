CC      := arm-none-eabi-gcc
CXX     := arm-none-eabi-g++
LD      := arm-none-eabi-g++
AS      := arm-none-eabi-as
AR      := arm-none-eabi-ar
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size
RANLIB  := arm-none-eabi-ranlib

APP_NAME     ?= embys
PROJECT_ROOT ?= $(shell pwd)

ARCH_DIR      := $(PROJECT_ROOT)/arch/stm32f103xb
SRC_DIR       ?= src
BUILD_DIR     ?= build
BIN_DIR       ?= $(BUILD_DIR)/arm
OBJ_DIR       := $(BUILD_DIR)/obj/$(APP_NAME)/arm
APP_OBJ_DIR   := $(OBJ_DIR)/app
ARCH_OBJ_DIR	:= $(OBJ_DIR)/arch

APP_SRC      ?= $(shell find $(SRC_DIR) -type f -name '*.cpp')
ARCH_SRC     ?= $(shell find $(ARCH_DIR) -type f -name '*.c')
LINKER_LD    := $(ARCH_DIR)/linker.ld
APP_OBJ      := $(patsubst $(SRC_DIR)/%.cpp,$(APP_OBJ_DIR)/%.o,$(APP_SRC))
ARCH_OBJ     := $(patsubst $(ARCH_DIR)/%.c,$(ARCH_OBJ_DIR)/%.o,$(ARCH_SRC))

TARGET_LIB  := $(BIN_DIR)/lib$(APP_NAME).a
TARGET      := $(BIN_DIR)/$(APP_NAME).elf
HEX         := $(BIN_DIR)/$(APP_NAME).hex
BIN         := $(BIN_DIR)/$(APP_NAME).bin
MAP			 	  := $(BIN_DIR)/$(APP_NAME).map

INCLUDES     += -I$(PROJECT_ROOT)/third_party/CMSIS_6/CMSIS/Core/Include \
							  -I$(PROJECT_ROOT)/third_party/cmsis-device-f1/Include \
								-I$(PROJECT_ROOT)/build/include
DEFINES      += -DSTM32F103xB
MCUFLAGS     += -mcpu=cortex-m3 -mthumb
CPPFLAGS     += $(MCUFLAGS) $(INCLUDES) $(DEFINES)
CFLAGS       += -Wall -Wextra -Werror \
			          -Os -flto \
			          -ffunction-sections -fdata-sections -fno-builtin \
			          -std=c11 -MMD -MP
CXXFLAGS     += -Wall -Wextra -Werror -Wundef \
			          -Os -g3 -flto -ffunction-sections -fdata-sections \
			          -std=c++20 -fno-rtti -fno-exceptions -fno-threadsafe-statics \
			          -fno-unwind-tables -fno-asynchronous-unwind-tables \
			          -ffreestanding -fno-builtin -fstack-usage \
			          -fno-use-cxa-atexit -MMD -MP
LDFLAGS      += $(MCUFLAGS) -flto -nostartfiles -nostdlib -Wl,--gc-sections -Wl,-Map,$(MAP) -T $(LINKER_LD) -Wl,--print-memory-usage -L$(PROJECT_ROOT)/build/arm
LDLIBS       += -lgcc

$(APP_OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(ARCH_OBJ_DIR)/%.o: $(ARCH_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(TARGET_LIB): $(APP_OBJ) $(ARCH_OBJ)
	mkdir -p $(dir $@)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(TARGET): $(APP_OBJ) $(ARCH_OBJ)
	mkdir -p $(dir $@)
	$(CC) $^ $(LDFLAGS) -Wl,--start-group $(LDLIBS) -Wl,--end-group -o $@

$(HEX): $(TARGET)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(TARGET)
	$(OBJCOPY) -O binary $< $@

.size: $(TARGET)
	$(SIZE) -A $<

.lib: $(TARGET_LIB)

.bin: $(TARGET) $(HEX) $(BIN)

flash:
	@echo "Flashing $(BIN) to device..."
	st-flash write $(BIN) 0x8000000

chipinfo:
	st-info --chipid
	st-info --flash
	st-info --serial

-include $(APP_OBJ:.o=.d) $(ARCH_OBJ:.o=.d)
