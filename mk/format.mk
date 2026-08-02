CLANG_FORMAT ?= clang-format
CLANG_TIDY ?= clang-tidy

FORMAT_SOURCES = $(shell git ls-files -- '*.c' '*.h' '*.cpp' '*.hpp' \
	':!:future/**' ':!:arch/stm32f767xx/**' ':!:arch/stm32h743xx/**' \
	':!:tests/include/doctest.hpp')
TIDY_SOURCES = $(shell git ls-files -- 'libs/stm32/**/*.cpp' ':!:future/**')

# Code formatting targets
.PHONY: format-local
format-local:
	@echo "Formatting source code..."
	$(CLANG_FORMAT) -i $(FORMAT_SOURCES)

.PHONY: format-check-local
format-check-local:
	@echo "Checking code formatting..."
	$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_SOURCES)

# Code linting targets
.PHONY: lint-local
lint-local: compile-db-local
	@echo "Linting source code..."
	$(CLANG_TIDY) -p . $(TIDY_SOURCES)

.PHONY: lint-fix-local
lint-fix-local: compile-db-local
	@echo "Linting and fixing source code..."
	$(CLANG_TIDY) -p . --fix $(TIDY_SOURCES)

# Generate compile commands database for IDEs/tools
.PHONY: compile-db-local
compile-db-local:
	@echo "Generating compile_commands.json..."
	bear --output compile_commands.json -- $(MAKE) -B TC=sim all-local
