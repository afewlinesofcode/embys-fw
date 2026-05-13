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
	cd examples/gpio_blink && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd examples/gpio_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd examples/uart_print && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd examples/uart_echo && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd examples/i2c_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd examples/i2c_aht20 && $(MAKE) TC=$(TC) MCU=$(MCU) all

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
	cd examples/gpio_blink && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd examples/gpio_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd examples/uart_print && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd examples/uart_echo && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd examples/i2c_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd examples/i2c_aht20 && $(MAKE) TC=$(TC) MCU=$(MCU) clean

.PHONY: all test clean-tests examples clean
