MCU ?= stm32f103xb

HAL_FAMILY := $(shell echo $(MCU) | cut -c6-7)
ARCH_DIR   := $(PROJECT_ROOT)/arch/$(MCU)
ARCH_NAME  := stm32$(HAL_FAMILY)xx
CMSIS_DEV  := $(PROJECT_ROOT)/third_party/cmsis-device-$(HAL_FAMILY)/Include
MCU_UPPER  := $(shell echo $(MCU) | tr 'a-wyz' 'A-WYZ')
HAL_UPPER  := $(shell echo $(HAL_FAMILY) | tr 'a-z' 'A-Z')
MCU_DEFINES := -D$(MCU_UPPER) -DSTM32$(HAL_UPPER)xx
