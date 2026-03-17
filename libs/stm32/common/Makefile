DEFAULT_GOAL := all
PROJECT_ROOT := $(realpath $(shell pwd)/../../..)

INCLUDE_DIR := include
BUILD_DIR   := $(PROJECT_ROOT)/build/include/embys/stm32

HEADERS            := $(shell find $(INCLUDE_DIR) -name "*.hpp")
INSTALLED_HEADERS  := $(patsubst $(INCLUDE_DIR)/%,$(BUILD_DIR)/%,$(HEADERS))

$(BUILD_DIR)/%.hpp: $(INCLUDE_DIR)/%.hpp
	@mkdir -p $(dir $@)
	@echo "Installing header: $@"
	@cp $< $@

.PHONY: all clean

all: $(INSTALLED_HEADERS)

clean:
	rm -rf $(INSTALLED_HEADERS)
