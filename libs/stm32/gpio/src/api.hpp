/**
 * @file api.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief GPIO public API types and structures
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

int
enable_gpio(GPIO_TypeDef *port);

int
disable_gpio(GPIO_TypeDef *port);

int
enable_afio();

int
disable_afio();

int
configure_pin(GPIO_TypeDef *port, uint8_t index, uint32_t gpio_cfg);

int
configure_pin_pull_up(GPIO_TypeDef *port, uint8_t index);

int
configure_pin_pull_down(GPIO_TypeDef *port, uint8_t index);

int
reset_pin(GPIO_TypeDef *port, uint8_t index);

int
enable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index);

int
disable_pin_irq(GPIO_TypeDef *port, uint8_t pin_index);

int
read_pin(GPIO_TypeDef *port, uint8_t index, uint8_t *value);

int
write_pin(GPIO_TypeDef *port, uint8_t index, uint8_t value);

}; // namespace Embys::Stm32::Gpio
