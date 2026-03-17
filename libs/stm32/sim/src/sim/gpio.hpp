/**
 * @file gpio.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief GPIO simulation for the STM32 simulated environment.
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core.hpp"

namespace Embys::Stm32::Sim
{

namespace Gpio
{

/**
 * @brief Simulate an external event on a GPIO pin by setting the corresponding
 * bit in the IDR register and triggering an EXTI interrupt if configured.
 * @param port Pointer to the GPIO port instance (e.g., gpioa, gpiob from
 * core.hpp).
 * @param pin_index Index of the pin to trigger (0-15).
 * @param value Value to set on the pin (0 or 1).
 */
void
trigger_pin(GPIO_TypeDef *port, uint8_t pin_index, uint8_t value);

}; // namespace Gpio

}; // namespace Embys::Stm32::Sim
