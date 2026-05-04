.DEFAULT_GOAL := all
TC := sim
PROJECT_ROOT := $(realpath $(shell pwd)/..)
WORK_DIR := /work/tests

SRC_DIR := .

LIBS = stm32-modbus-rtu stm32-modbus stm32-uart stm32-i2c stm32-gpio stm32-base stm32-sim
LDLIBS = $(addprefix -l,$(LIBS))
DEPS = $(addprefix $(BUILD_LIB_DIR)/lib,$(addsuffix .a,$(LIBS)))
TARGET_DEPS = $(DEPS)

INCLUDES = -Iinclude -I$(PROJECT_ROOT)/build/include_sim

include $(PROJECT_ROOT)/mk/toolchain/$(TC).mk
include $(PROJECT_ROOT)/mk/docker.mk

all: .run-in-docker

.only: $(TARGET)
	$(TARGET) --test-case="ONLY*"

only: .only-in-docker

.only-suite: $(TARGET)
	$(TARGET) --test-suite="ONLY*"

only-suite: .only-suite-in-docker

clean:
	rm -rf $(TARGET) $(OBJ_DIR)
