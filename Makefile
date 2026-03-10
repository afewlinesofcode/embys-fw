# Default target
.PHONY: all
all:
	@echo "No target specified"

-include $(foreach target,$(MAKECMDGOALS),mk/$(target).mk)

include mk/docker.mk
include mk/format.mk
