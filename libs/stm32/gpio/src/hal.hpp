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

/**
 * @brief Enable the clock for a GPIO port.
 * @param port GPIO port (GPIOA, GPIOB, …).
 * @return 0 on success, negative error code on failure.
 */
int
enable_gpio(GPIO_TypeDef *port);

/**
 * @brief Disable the clock for a GPIO port.
 * @param port GPIO port (GPIOA, GPIOB, …).
 * @return 0 on success, negative error code on failure.
 */
int
disable_gpio(GPIO_TypeDef *port);

/**
 * @brief Enable the clock for the EXTI source routing peripheral (AFIO on F1).
 * Must be called before configuring any pin IRQs.
 * @return 0 on success, negative error code on failure.
 */
int
enable_exti();

/**
 * @brief Disable the clock for the EXTI source routing peripheral (AFIO on F1).
 * @return 0 on success, negative error code on failure.
 */
int
disable_exti();

/**
 * @brief Validate the configuration of a pin.
 * @param port GPIO port (GPIOA, GPIOB, …).
 * @param index Pin number (0–15).
 * @param cfg Pin configuration flags (PinCfg bitmask).
 * @param pwm PWM binding information.
 * @return 0 on success, negative error code on failure.
 */
int
validate_pin_config(GPIO_TypeDef *port, uint8_t index, PinCfg cfg,
                    const PwmBinding *pwm);

int
configure_pin(GPIO_TypeDef *port, uint8_t index, PinCfg cfg,
              const PwmBinding *pwm);

int
reset_pin(GPIO_TypeDef *port, uint8_t index, PinCfg cfg, const PwmBinding *pwm);

/**
 * @brief Check whether the EXTI pending flag is set for a pin and clear it.
 * @param pin_index Pin number (0–15).
 * @return true if the flag was set (and has now been cleared), false otherwise.
 */
bool
exti_get_and_clear_pending(uint8_t pin_index);

/**
 * @brief Read the current input value of a pin.
 * @param port GPIO port (GPIOA, GPIOB, …).
 * @param index Pin number (0–15).
 * @param value Output parameter for the pin value (0 or 1).
 * @return 0 on success, negative error code on failure.
 */
int
read_pin(GPIO_TypeDef *port, uint8_t index, uint8_t *value);

/**
 * @brief Set the output value of a pin.
 * @param port GPIO port (GPIOA, GPIOB, …).
 * @param index Pin number (0–15).
 * @param value Value to set (0 for low, non-zero for high).
 * @return 0 on success, negative error code on failure.
 */
int
write_pin(GPIO_TypeDef *port, uint8_t index, uint8_t value);

PinCfg
get_effective_pin_cfg(const PinCfg &cfg);

}; // namespace Embys::Stm32::Gpio
