MCU ?= stm32f103xb

SUPPORTED_MCUS := stm32f103xb stm32f407xx stm32f411xe
ifeq ($(MCU),stm32f103xb)
  HAL_FAMILY := f1
  MCU_DEFINES := -DSTM32F103xB -DSTM32F1xx
else ifeq ($(MCU),stm32f407xx)
  HAL_FAMILY := f4
  MCU_DEFINES := -DSTM32F407xx -DSTM32F4xx
else ifeq ($(MCU),stm32f411xe)
  HAL_FAMILY := f4
  MCU_DEFINES := -DSTM32F411xE -DSTM32F4xx
else
  $(error Unsupported MCU '$(MCU)'; supported MCUs: $(SUPPORTED_MCUS))
endif

ARCH_DIR  := $(PROJECT_ROOT)/arch/$(MCU)
ARCH_NAME := $(MCU)
CMSIS_DEV := $(PROJECT_ROOT)/third_party/cmsis-device-$(HAL_FAMILY)/Include
