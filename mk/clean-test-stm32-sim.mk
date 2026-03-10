include mk/test-stm32-sim.mk

# Clean build artifacts
.PHONY: clean-test-stm32-sim
clean-test-stm32-sim:
	@echo "Cleaning build artifacts..."
	rm -rf $(TESTS_BUILD_DIR)
