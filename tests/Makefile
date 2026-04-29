.DEFAULT_GOAL := all
TC := sim
PROJECT_ROOT := $(realpath $(shell pwd)/..)
WORK_DIR := /work/tests

SRC_DIR := .

LDLIBS = -lstm32-uart -lstm32-i2c -lstm32-gpio -lstm32-base -lstm32-sim
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
