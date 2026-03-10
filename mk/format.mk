CLANG_FORMAT = clang-format
CLANG_TIDY = clang-tidy

# Code formatting targets
.PHONY: format
format:
	@echo "Formatting source code..."
	find $(SRC_DIR) -name "*.cpp" -o -name "*.hpp" | xargs $(CLANG_FORMAT) -i

.PHONY: format-check
format-check:
	@echo "Checking code formatting..."
	find $(SRC_DIR) -name "*.cpp" -o -name "*.hpp" | xargs $(CLANG_FORMAT) --dry-run --Werror

# Code linting targets
.PHONY: lint
lint:
	@echo "Linting source code..."
	find $(SRC_DIR) -name "*.cpp" | xargs $(CLANG_TIDY) --

.PHONY: lint-fix
lint-fix:
	@echo "Linting and fixing source code..."
	find $(SRC_DIR) -name "*.cpp" | xargs $(CLANG_TIDY) --fix --

# Generate compile commands database for IDEs/tools
.PHONY: compile-db
compile-db:
	@echo "Generating compile_commands.json..."
	bear -- make stm32-mock
