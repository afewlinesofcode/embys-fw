#pragma once

#include <cstdint>
#include <type_traits>

#include <embys/stm32/mcu.hpp>

#include "def/pin_cfg.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::Gpio
{

enum class Port : uint8_t
{
  A,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
};

template <typename Device, Port SelectedPort>
inline constexpr bool port_available =
    (std::is_same_v<Device, Stm32::Stm32f103xb> &&
     (SelectedPort == Port::A || SelectedPort == Port::B ||
      SelectedPort == Port::C || SelectedPort == Port::D)) ||
    (std::is_same_v<Device, Stm32::Stm32f407xx> &&
     (SelectedPort == Port::A || SelectedPort == Port::B ||
      SelectedPort == Port::C || SelectedPort == Port::D ||
      SelectedPort == Port::E || SelectedPort == Port::F ||
      SelectedPort == Port::G || SelectedPort == Port::H ||
      SelectedPort == Port::I)) ||
    (std::is_same_v<Device, Stm32::Stm32f411xe> &&
     (SelectedPort == Port::A || SelectedPort == Port::B ||
      SelectedPort == Port::C || SelectedPort == Port::D ||
      SelectedPort == Port::E || SelectedPort == Port::H));

template <PinCfg Config>
inline constexpr bool config_valid =
    !has_many_roles(Config) && !has_many_speeds(Config) &&
    !(has_cfg(Config, PinCfg::PU) && has_cfg(Config, PinCfg::PD)) &&
    (has_any_role(Config) || has_cfg(Config, PinCfg::IN) ||
     has_cfg(Config, PinCfg::OUT) || has_cfg(Config, PinCfg::AF));

template <Port SelectedPort>
[[nodiscard]] inline GPIO_TypeDef *
port_address() noexcept
{
  static_assert(port_available<Stm32::TargetDevice, SelectedPort>,
                "Selected GPIO port is not available on this target");

  if constexpr (SelectedPort == Port::A)
    return GPIOA;
  else if constexpr (SelectedPort == Port::B)
    return GPIOB;
  else if constexpr (SelectedPort == Port::C)
    return GPIOC;
  else if constexpr (SelectedPort == Port::D)
    return GPIOD;
#ifdef GPIOE
  else if constexpr (SelectedPort == Port::E)
    return GPIOE;
#endif
#ifdef GPIOF
  else if constexpr (SelectedPort == Port::F)
    return GPIOF;
#endif
#ifdef GPIOG
  else if constexpr (SelectedPort == Port::G)
    return GPIOG;
#endif
#ifdef GPIOH
  else if constexpr (SelectedPort == Port::H)
    return GPIOH;
#endif
#ifdef GPIOI
  else if constexpr (SelectedPort == Port::I)
    return GPIOI;
#endif
  else
    return nullptr;
}

} // namespace Embys::Stm32::Gpio
