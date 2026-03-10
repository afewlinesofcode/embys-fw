include mk/test-stm32-mock.mk

# Clean build artifacts
.PHONY: clean-test-stm32-mock
clean-test-stm32-mock:
	@echo "Cleaning build artifacts..."
	rm -rf $(TESTS_BUILD_DIR)
