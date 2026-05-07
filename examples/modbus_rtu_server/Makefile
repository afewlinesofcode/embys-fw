TC ?= arm
.DEFAULT_GOAL := all
$(info Using toolchain: $(TC))

APP_NAME     := modbus_rtu_server
PROJECT_ROOT := $(realpath $(shell pwd)/../..)
WORK_DIR     := /work/examples/modbus_rtu_server
DOCKER_NAME  := modbus_rtu_server_container

LIBS = stm32-modbus-rtu stm32-modbus stm32-uart \
       stm32-i2c-hd44780 stm32-i2c-common stm32-i2c \
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

# Inject a Modbus RTU read-coils request (FC=0x01, addr=0x1000, qty=10, CRC)
# echo "modbus_request <hex bytes>" > /tmp/embys_stm32_sim_pipe
read-coils:
	docker exec -it $(DOCKER_NAME) bash -c \
		'echo "modbus_request 01 01 10 00 00 0A B8 CD" > /tmp/embys_stm32_sim_pipe'

write-coil:
	docker exec -it $(DOCKER_NAME) bash -c \
		'echo "modbus_request 01 05 10 00 FF 00 88 FA" > /tmp/embys_stm32_sim_pipe'

read-holding:
	docker exec -it $(DOCKER_NAME) bash -c \
		'echo "modbus_request 01 03 10 00 00 0A C1 0D" > /tmp/embys_stm32_sim_pipe'
