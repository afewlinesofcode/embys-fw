TC ?= arm
.DEFAULT_GOAL := all

# -include $(foreach target,$(MAKECMDGOALS),mk/$(target).mk)

include mk/format.mk

test:
	cd tests && $(MAKE) all

clean-tests:
	cd tests && $(MAKE) clean

all:
	cd libs/stm32/common && $(MAKE) TC=$(TC) all
	$(if $(filter sim,$(TC)),cd libs/stm32/sim && $(MAKE) TC=$(TC) all)
	cd libs/stm32/base && $(MAKE) TC=$(TC) all

examples:
	cd examples/gpio_blink && $(MAKE) TC=$(TC) all

clean:
	cd libs/stm32/common && $(MAKE) TC=$(TC) clean
	cd libs/stm32/sim && $(MAKE) TC=$(TC) clean
	cd libs/stm32/base && $(MAKE) TC=$(TC) clean
	cd examples/gpio_blink && $(MAKE) TC=$(TC) clean
