TC ?= arm
MCU ?= stm32f411xe
.DEFAULT_GOAL := all
$(info Using toolchain: $(TC))

APP_NAME		 := gpio_blink_f4
PROJECT_ROOT := $(realpath $(shell pwd)/../..)
WORK_DIR     := /work/examples/$(APP_NAME)

LDLIBS = -lstm32-base

ifeq ($(TC),sim)
	LDLIBS += -lstm32-sim
	INCLUDES += -I$(PROJECT_ROOT)/build/include_sim
endif

include $(PROJECT_ROOT)/mk/toolchain/$(TC).mk
include $(PROJECT_ROOT)/mk/docker.mk

all: .bin-in-docker

run: .run-in-docker

clean:
	rm -rf $(TARGET) $(HEX) $(BIN) $(MAP) $(OBJ_DIR)
