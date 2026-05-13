#pragma once

#include <stdint.h>

#include "stm32xx.hpp"

namespace Embys::Stm32::Gpio
{

/**
 * @brief GPIO pin mode.
 *
 * On F1 the values match CRL/CRH MODE bits directly.
 * On F4/F7/H7 the HAL maps these to OSPEEDR values; the MODER value is
 * derived from Cnf (output vs alternate-function).
 */
enum Mode : uint8_t
{
  IN,     // Input (reset state)
  OUT_10, // Output mode, max speed 10 MHz  (F4+: medium speed)
  OUT_2,  // Output mode, max speed 2 MHz   (F4+: low speed)
  OUT_50  // Output mode, max speed 50 MHz  (F4+: high speed)
};

/**
 * @brief GPIO pin configuration (type and alternate-function flag).
 *
 * On F1 the values match CRL/CRH CNF bits directly.
 * On F4/F7/H7 the HAL interprets these to set OTYPER (PP vs OD) and MODER
 * (output vs AF).  IN_AN maps to analog mode (MODER=11).
 */
enum Cnf : uint8_t
{
  // Input variants — lower 2 bits are F1 CRL/CRH CNF bits for input mode
  IN_AN,     // Analog             (F1 CNF=00, MODER=11)
  IN_FL,     // Floating           (F1 CNF=01, MODER=00)
  IN_PU,     // Pull-up/Pull-down  (F1 CNF=10, MODER=00)
             // Output variants — bit2=1; lower 2 bits are F1 CRL/CRH CNF
             // bits for output
  OUT_PP,    // Push-Pull output       (F1 CNF=00, MODER=01)
  OUT_OD,    // Open-Drain output      (F1 CNF=01, MODER=01)
  OUT_PP_AF, // Push-Pull Alt-function (F1 CNF=10, MODER=10)
  OUT_OD_AF  // Open-Drain Alt-function(F1 CNF=11, MODER=10)
};

/**
 * @brief GPIO pin additional configuration flags.
 */
struct PinCfg
{
  static constexpr uint8_t NONE = 0b0000;
  static constexpr uint8_t PULL_UP = 0b0001;
  static constexpr uint8_t PULL_DOWN = 0b0010;
  static constexpr uint8_t IRQ = 0b0100;
};


}; // namespace Embys::Stm32::Gpio
