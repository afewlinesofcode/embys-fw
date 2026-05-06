TC ?= arm
.DEFAULT_GOAL := all
$(info Using toolchain: $(TC))

APP_NAME     := i2c_aht20
PROJECT_ROOT := $(realpath $(shell pwd)/../..)
WORK_DIR     := /work/examples/i2c_aht20
DOCKER_NAME  := i2c_aht20_container

LIBS = stm32-i2c-aht20 stm32-i2c-hd44780 stm32-i2c-common stm32-i2c \
       stm32-gpio stm32-base stm32-common
LDLIBS = $(LIBS:%=-l%)
TARGET_DEPS = $(addprefix $(BUILD_LIB_DIR)/lib,$(addsuffix .a,$(LIBS)))

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
