TC ?= arm
.DEFAULT_GOAL := all

# -include $(foreach target,$(MAKECMDGOALS),mk/$(target).mk)

include mk/format.mk

test:
	$(MAKE) TC=sim all && cd tests && $(MAKE) all

clean-tests:
	cd tests && $(MAKE) clean

all:
	cd libs/stm32/common && $(MAKE) TC=$(TC) all
	$(if $(filter sim,$(TC)),cd libs/stm32/sim && $(MAKE) TC=$(TC) all)
	cd libs/stm32/base && $(MAKE) TC=$(TC) all
	cd libs/stm32/gpio && $(MAKE) TC=$(TC) all
	cd libs/stm32/uart && $(MAKE) TC=$(TC) all

examples:
	cd examples/gpio_blink && $(MAKE) TC=$(TC) all
	cd examples/gpio_btn_blink && $(MAKE) TC=$(TC) all
	cd examples/uart_print && $(MAKE) TC=$(TC) all
	cd examples/uart_echo && $(MAKE) TC=$(TC) all

clean:
	cd libs/stm32/common && $(MAKE) TC=$(TC) clean
	cd libs/stm32/sim && $(MAKE) TC=$(TC) clean
	cd libs/stm32/base && $(MAKE) TC=$(TC) clean
	cd libs/stm32/gpio && $(MAKE) TC=$(TC) clean
	cd libs/stm32/uart && $(MAKE) TC=$(TC) clean
	cd examples/gpio_blink && $(MAKE) TC=$(TC) clean
	cd examples/gpio_btn_blink && $(MAKE) TC=$(TC) clean
	cd examples/uart_print && $(MAKE) TC=$(TC) clean
	cd examples/uart_echo && $(MAKE) TC=$(TC) clean

.PHONY: all test clean-tests examples clean
