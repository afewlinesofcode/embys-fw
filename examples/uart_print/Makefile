TC ?= arm
.DEFAULT_GOAL := all
$(info Using toolchain: $(TC))

APP_NAME     := uart_print
PROJECT_ROOT := $(realpath $(shell pwd)/../..)
WORK_DIR     := /work/examples/uart_print

LDLIBS = -lstm32-uart -lstm32-gpio -lstm32-base

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
