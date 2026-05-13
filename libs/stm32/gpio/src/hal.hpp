/**
 * @file hal.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief GPIO HAL API — family-agnostic declarations implemented per-family
 * in src/hal/<family>/hal.cpp.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "def.hpp"

namespace Embys::Stm32::Gpio
{

// ── GPIO port clock
// ───────────────────────────────────────────────────────────

int
enable_gpio(GPIO_TypeDef *port);

int
disable_gpio(GPIO_TypeDef *port);

// ── EXTI source clock (AFIO on F1, SYSCFG on F4/F7/H7) ───────────────────────

int
enable_exti_source_clock();

int
disable_exti_source_clock();

// ── Pin configuration
// ─────────────────────────────────────────────────────────

int
configure_pin(GPIO_TypeDef *port, uint8_t index, Mode mode, Cnf cnf);

int
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index);

int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index);

int
reset_pin(GPIO_TypeDef *port, uint8_t index);

// ── EXTI interrupt routing
// ────────────────────────────────────────────────────

int
enable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index);

int
disable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index);

/**
 * @brief Check whether the EXTI pending flag is set for a pin and clear it.
 * @param pin_index Pin number (0–15).
 * @return true if the flag was set (and has now been cleared), false otherwise.
 */
bool
exti_get_and_clear_pending(uint8_t pin_index);

// ── Pin I/O (identical register layout across all families) ──────────────────

int
read_pin(GPIO_TypeDef *port, uint8_t index, uint8_t *value);

int
write_pin(GPIO_TypeDef *port, uint8_t index, uint8_t value);

}; // namespace Embys::Stm32::Gpio
