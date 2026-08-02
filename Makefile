TC  ?= arm
MCU ?= stm32f103xb
.DEFAULT_GOAL := all

# -include $(foreach target,$(MAKECMDGOALS),mk/$(target).mk)

include mk/format.mk

sim:
	$(MAKE) TC=sim all

test:
	cd tests && $(MAKE) all

test-only:
	cd tests && $(MAKE) only

test-only-suite:
	cd tests && $(MAKE) only-suite

clean-tests:
	cd tests && $(MAKE) clean

all:
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

examples:
	cd examples && $(MAKE) TC=$(TC) MCU=$(MCU) clean all

firmware-check:
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

.PHONY: all test clean-tests examples firmware-check clean
