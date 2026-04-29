TC ?= arm
.DEFAULT_GOAL := all
$(info Using toolchain: $(TC))

APP_NAME     := i2c_btn_blink
PROJECT_ROOT := $(realpath $(shell pwd)/../..)
WORK_DIR     := /work/examples/i2c_btn_blink
DOCKER_NAME  := i2c_btn_blink_container

LDLIBS = -lstm32-i2c-hd44780 -lstm32-i2c-common -lstm32-i2c \
         -lstm32-gpio -lstm32-base -lstm32-common

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

btn-toggle:
	docker exec -it $(DOCKER_NAME) bash -c \
		'echo "btn_toggle" > /tmp/embys_stm32_sim_pipe'
