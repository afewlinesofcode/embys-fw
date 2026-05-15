TC ?= arm
MCU ?= stm32f103xb
.DEFAULT_GOAL := all

clean:
	cd gpio_blink_f1 && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd gpio_blink_f4 && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd gpio_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd uart_print && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd uart_echo && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd i2c_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd i2c_aht20 && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd uart_print && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd uart_print_rede && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd uart_echo && $(MAKE) TC=$(TC) MCU=$(MCU) clean
	cd modbus_rtu_server && $(MAKE) TC=$(TC) MCU=$(MCU) clean

all:
	cd gpio_blink_f1 && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd gpio_blink_f4 && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd gpio_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd uart_print && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd uart_echo && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd i2c_btn_blink && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd i2c_aht20 && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd uart_print && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd uart_print_rede && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd uart_echo && $(MAKE) TC=$(TC) MCU=$(MCU) all
	cd modbus_rtu_server && $(MAKE) TC=$(TC) MCU=$(MCU) all
