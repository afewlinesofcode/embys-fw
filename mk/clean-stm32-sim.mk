include mk/stm32-sim.mk

# Clean build artifacts
.PHONY: clean-stm32-sim
clean-stm32-sim:
	@echo "Cleaning build artifacts..."
	rm -rf $(SIM_BUILD_DIR)
	rm -rf $(SIM_BUILD_INCLUDE_DIR)
