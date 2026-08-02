#pragma once

#include <cstdint>
#include <type_traits>

#include <embys/stm32/mcu.hpp>

#include "stm32xx.hpp"

namespace Embys::Stm32::Uart
{

enum class Instance : uint8_t
{
  Usart1,
  Usart2,
  Usart3,
  Usart6,
};

template <typename Device, Instance Peripheral>
inline constexpr bool instance_available =
    (std::is_same_v<Device, Stm32::Stm32f103xb> &&
     (Peripheral == Instance::Usart1 || Peripheral == Instance::Usart2 ||
      Peripheral == Instance::Usart3)) ||
    (std::is_same_v<Device, Stm32::Stm32f407xx> &&
     (Peripheral == Instance::Usart1 || Peripheral == Instance::Usart2 ||
      Peripheral == Instance::Usart3 || Peripheral == Instance::Usart6)) ||
    (std::is_same_v<Device, Stm32::Stm32f411xe> &&
     (Peripheral == Instance::Usart1 || Peripheral == Instance::Usart2 ||
      Peripheral == Instance::Usart6));

template <Instance Peripheral>
[[nodiscard]] inline USART_TypeDef *
peripheral_address() noexcept
{
  static_assert(instance_available<Stm32::TargetDevice, Peripheral>,
                "Selected UART instance is not available on this target");

  if constexpr (Peripheral == Instance::Usart1)
    return USART1;
  else if constexpr (Peripheral == Instance::Usart2)
    return USART2;
#ifdef USART3
  else if constexpr (Peripheral == Instance::Usart3)
    return USART3;
#endif
#ifdef USART6
  else if constexpr (Peripheral == Instance::Usart6)
    return USART6;
#endif
  else
    return nullptr;
}

} // namespace Embys::Stm32::Uart
