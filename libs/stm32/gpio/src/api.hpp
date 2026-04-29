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

#include <stdint.h>

#include "stm32f1xx.hpp"

namespace Embys::Stm32::Gpio
{

/**
 * @brief GPIO pin modes
 */
enum Mode : uint32_t
{
  IN = 0b00,     // Input (reset state)
  OUT_10 = 0b01, // Output mode, max speed 10 MHz
  OUT_2 = 0b10,  // Output mode, max speed 2 MHz
  OUT_50 = 0b11  // Output mode, max speed 50 MHz
};

/**
 * @brief GPIO pin configuration
 */
enum Cnf : uint32_t
{
  IN_AN = 0b00,     // Analog
  IN_FL = 0b01,     // Floating
  IN_PU = 0b10,     // Pull-up/Pull-down
  OUT_PP = 0b00,    // Push-Pull
  OUT_OD = 0b01,    // Open-Drain
  OUT_PP_AF = 0b10, // Push-Pull Alternate Function
  OUT_OD_AF = 0b11  // Open-Drain Alternate Function
};

/**
 * @brief GPIO pin additional configuration
 *
 */
enum PinCfg : uint32_t
{
  NONE = 0b0000,
  PULL_UP = 0b0001,
  PULL_DOWN = 0b0010,
  IRQ = 0b0100
};

// Combine MODE + CNF into one 4-bit field
constexpr uint32_t
make_cfg(uint32_t mode, uint32_t cnf)
{
  return ((cnf & 0b11) << 2) | (mode & 0b11);
}

constexpr volatile uint32_t *
pin_cr(GPIO_TypeDef *port, uint8_t index)
{
  return (index < 8) ? &(port->CRL) : &(port->CRH);
}

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
