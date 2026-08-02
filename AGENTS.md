# Repository Guidelines

## Project Structure & Module Organization

This repository is a modular C++ firmware foundation for STM32 devices. Drivers and protocols live under `libs/stm32/` (for example, `gpio`, `uart`, `i2c`, and `modbus-rtu`). MCU startup code and linker scripts are in `arch/<target>/`. Firmware examples are in `examples/<name>/src/`, while simulator and unit-test code is in `tests/`. Shared build logic is in `mk/`; CMSIS and device headers are vendored in `third_party/`. Generated objects, libraries, and includes belong in `build/` and should not be committed.

## Build, Test, and Development Commands

Initialize dependencies before building:

```sh
git submodule update --init --recursive
```

From the repository root:

- `make` or `make TC=arm` builds libraries for the default `stm32f103xb` target.
- `make TC=sim` builds the libraries for the host simulator.
- `make test` runs the Doctest suite in the simulator; use `make test-only` or `make test-only-suite` for focused `ONLY*` cases.
- `make examples TC=sim` builds all examples for simulation; in an example directory, `make TC=sim run` runs that example.
- `make examples TC=arm MCU=stm32f411xe` builds examples for a selected MCU.
- `make clean` and `make clean-tests` remove corresponding build artifacts.
- `make format-check` checks formatting on the host; use `make format` to apply it. Build, test, and firmware targets use the Docker toolchain automatically.

## Coding Style & Naming Conventions

Use C++ with two-space indentation, spaces (never tabs), an 80-column limit, and Allman braces. Keep headers beside their implementation files, use descriptive lowercase filenames, and follow existing class/member naming in the surrounding module. Run `clang-format` and, where practical, `clang-tidy` before submitting changes.

## Testing Guidelines

Tests use Doctest and are organized under `tests/stm32/` by library or feature. Add focused coverage for new behavior, naming files after the feature (for example, `tests/stm32/gpio.cpp`), and run `make test`. Verify simulator behavior and ARM compilation when changing portable or hardware-facing code.

## Commit & Pull Request Guidelines

Recent commits use short, lowercase summaries such as `readme update` and `ci update`. Keep commits focused and describe the user-visible or build-related change briefly. Pull requests should explain the motivation, list validation commands and target/toolchain combinations, note hardware-dependent limitations, and link related issues. Include logs or screenshots when they clarify simulator or example behavior.

## Security & Configuration Tips

Do not commit credentials, device secrets, or generated firmware artifacts. Keep MCU and toolchain selection explicit in commands (`TC=sim`, `MCU=stm32f103xb`), and use the provided Docker workflow when reproducing CI dependencies or toolchain differences.
