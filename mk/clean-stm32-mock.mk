include mk/stm32-mock.mk

# Clean build artifacts
.PHONY: clean-stm32-mock
clean-stm32-mock:
	@echo "Cleaning build artifacts..."
	rm -rf $(MOCK_BUILD_DIR)
	rm -rf $(MOCK_BUILD_INCLUDE_DIR)
