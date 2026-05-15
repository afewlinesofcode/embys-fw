#pragma once

#include <stdint.h>

#include "stm32xx.hpp"

namespace Embys::Stm32::Gpio
{

/**
 * @brief Unified GPIO pin configuration and role bitmask.
 *
 * This single enum encodes all pin direction, electrical, pull, alternate,
 * analog, and role configuration. Roles (I2C, UART, SPI, PWM, ANALOG) are
 * mutually exclusive and override all other config bits. LISTEN enables input
 * interrupt subscription.
 *
 * Usage example:
 *   PinCfg cfg = PinCfg::OUT | PinCfg::PU; // Output with pull-up
 *   PinCfg cfg = PinCfg::UART;             // UART role (overrides all others)
 */
enum PinCfg : uint32_t
{
  // Direction and electrical config
  NONE = 0,
  IN = 1U << 0,  ///< Input
  OUT = 1U << 1, ///< Output
  AF = 1U << 2,  ///< Alternate function
  OD = 1U << 3,  ///< Open-drain
  PU = 1U << 4,  ///< Pull-up
  PD = 1U << 5,  ///< Pull-down

  // Output/AF speed config
  LOW = 1U << 7,
  MEDIUM = 1U << 8,
  HIGH = 1U << 9,
  VHIGH = 1U << 10,

  // Special features
  LISTEN = 1U << 6, ///< Input interrupt subscription

  // Roles (mutually exclusive, override all other config)
  I2C = 1U << 16,
  UART = 1U << 17,
  SPI = 1U << 18,
  PWM = 1U << 19,
  ANALOG = 1U << 20,
};

constexpr static uint32_t all_roles_mask =
    static_cast<uint32_t>(PinCfg::I2C) | static_cast<uint32_t>(PinCfg::UART) |
    static_cast<uint32_t>(PinCfg::SPI) | static_cast<uint32_t>(PinCfg::PWM) |
    static_cast<uint32_t>(PinCfg::ANALOG);

constexpr static uint32_t all_speed_mask =
    static_cast<uint32_t>(PinCfg::LOW) | static_cast<uint32_t>(PinCfg::MEDIUM) |
    static_cast<uint32_t>(PinCfg::HIGH) | static_cast<uint32_t>(PinCfg::VHIGH);

inline constexpr PinCfg
operator|(PinCfg a, PinCfg b)
{
  return static_cast<PinCfg>(static_cast<uint32_t>(a) |
                             static_cast<uint32_t>(b));
}

inline constexpr PinCfg
operator&(PinCfg a, PinCfg b)
{
  return static_cast<PinCfg>(static_cast<uint32_t>(a) &
                             static_cast<uint32_t>(b));
}

inline constexpr PinCfg
operator~(PinCfg a)
{
  return static_cast<PinCfg>(~static_cast<uint32_t>(a));
}

inline constexpr bool
has_cfg(PinCfg cfg, PinCfg flag)
{
  return (cfg & flag) != 0;
}

inline constexpr bool
has_any_role(PinCfg cfg)
{
  return (static_cast<uint32_t>(cfg) & all_roles_mask) != 0;
}

}; // namespace Embys::Stm32::Gpio
