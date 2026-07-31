#pragma once

#include <cstdint>
#include <type_traits>

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
enum class PinCfg : uint32_t
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

constexpr PinCfg
operator|(PinCfg l, PinCfg r) noexcept
{
  using U = std::underlying_type_t<PinCfg>;
  return static_cast<PinCfg>(static_cast<U>(l) | static_cast<U>(r));
}

constexpr PinCfg
operator&(PinCfg l, PinCfg r) noexcept
{
  using U = std::underlying_type_t<PinCfg>;
  return static_cast<PinCfg>(static_cast<U>(l) & static_cast<U>(r));
}

constexpr PinCfg
operator|=(PinCfg cfg, PinCfg flags) noexcept
{
  return operator|(cfg, flags);
}

constexpr static PinCfg all_roles_mask =
    PinCfg::I2C | PinCfg::UART | PinCfg::SPI | PinCfg::PWM | PinCfg::ANALOG;

constexpr static PinCfg all_speed_mask =
    PinCfg::LOW | PinCfg::MEDIUM | PinCfg::HIGH | PinCfg::VHIGH;

constexpr bool
has_cfg(PinCfg cfg, PinCfg flag) noexcept
{
  return (cfg & flag) != PinCfg::NONE;
}

constexpr bool
has_any_role(PinCfg cfg) noexcept
{
  return (cfg & all_roles_mask) != PinCfg::NONE;
}

constexpr bool
has_many_roles(PinCfg cfg) noexcept
{
  using U = std::underlying_type_t<PinCfg>;
  U roles = static_cast<U>(cfg & all_roles_mask);
  return (roles & (roles - 1)) != 0;
}

constexpr bool
has_many_speeds(PinCfg cfg) noexcept
{
  using U = std::underlying_type_t<PinCfg>;
  U speeds = static_cast<U>(cfg & all_speed_mask);
  return (speeds & (speeds - 1)) != 0;
}


}; // namespace Embys::Stm32::Gpio
