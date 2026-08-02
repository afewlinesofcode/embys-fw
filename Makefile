TC  ?= arm
MCU ?= stm32f103xb
.DEFAULT_GOAL := all

PROJECT_ROOT := $(realpath $(shell pwd))
WORK_DIR := /work

# -include $(foreach target,$(MAKECMDGOALS),mk/$(target).mk)

include mk/format.mk
include mk/docker.mk

ifeq ($(INSIDE_DOCKER),true)
all: all-local
test: test-local
test-only: test-only-local
test-only-suite: test-only-suite-local
examples: examples-local
firmware-check: firmware-check-local
format: format-local
format-check: format-check-local
lint: lint-local
lint-fix: lint-fix-local
compile-db: compile-db-local
else
all: all-in-docker
test: test-in-docker
test-only: test-only-in-docker
test-only-suite: test-only-suite-in-docker
examples: examples-in-docker
firmware-check: firmware-check-in-docker
format: format-local
format-check: format-check-local
lint: lint-local
lint-fix: lint-fix-local
compile-db: compile-db-local
endif

sim:
	$(MAKE) TC=sim all

test-local:
	cd tests && $(MAKE) all

test-only-local:
	cd tests && $(MAKE) only

test-only-suite-local:
	cd tests && $(MAKE) only-suite

clean-tests:
	cd tests && $(MAKE) clean

all-local:
	cd libs/stm32/common && $(MAKE) TC=$(TC) MCU=$(MCU) all
	$(if $(filter sim,$(TC)),cd libs/stm32/sim && $(MAKE) TC=$(TC) MCU=$(MCU) all)
	cd libs/stm32/base && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/gpio && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/uart && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/i2c && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/i2c-common && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/i2c-hd44780 && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/i2c-aht20 && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/modbus && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd libs/stm32/modbus-rtu && $(MAKE) TC=$(TC) MCU=$(MCU) all

examples-local: all-local
	cd examples && $(MAKE) TC=$(TC) MCU=$(MCU) clean all

firmware-check-local:
	@test "$(TC)" = "arm" || \
		(echo "firmware-check requires TC=arm" >&2; exit 2)
	tools/check-firmware-symbols.sh $(MCU)

clean:
	cd libs/stm32/common && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/sim && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/base && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/gpio && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/uart && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/i2c && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/i2c-common && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/i2c-hd44780 && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/i2c-aht20 && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/modbus && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd libs/stm32/modbus-rtu && $(MAKE) TC=$(TC) MCU=$(MCU) clean

.PHONY: all all-local test test-local test-only test-only-local \
	test-only-suite test-only-suite-local clean-tests examples examples-local \
	firmware-check firmware-check-local format format-check lint lint-fix \
	compile-db clean
