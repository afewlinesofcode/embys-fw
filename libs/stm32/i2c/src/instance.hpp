#pragma once

#include <cstdint>
#include <type_traits>

#include <embys/stm32/mcu.hpp>

#include "stm32xx.hpp"

namespace Embys::Stm32::I2c
{

enum class Instance : uint8_t
{
  I2c1,
  I2c2,
  I2c3,
};

template <typename Device, Instance Peripheral>
inline constexpr bool instance_available =
    (std::is_same_v<Device, Stm32::Stm32f103xb> &&
     (Peripheral == Instance::I2c1 || Peripheral == Instance::I2c2)) ||
    ((std::is_same_v<Device, Stm32::Stm32f407xx> ||
      std::is_same_v<Device, Stm32::Stm32f411xe>) &&
     (Peripheral == Instance::I2c1 || Peripheral == Instance::I2c2 ||
      Peripheral == Instance::I2c3));

template <Instance Peripheral>
[[nodiscard]] inline I2C_TypeDef *
peripheral_address() noexcept
{
  static_assert(instance_available<Stm32::TargetDevice, Peripheral>,
                "Selected I2C instance is not available on this target");

  if constexpr (Peripheral == Instance::I2c1)
    return I2C1;
  else if constexpr (Peripheral == Instance::I2c2)
    return I2C2;
#ifdef I2C3
  else if constexpr (Peripheral == Instance::I2c3)
    return I2C3;
#endif
  else
    return nullptr;
}

} // namespace Embys::Stm32::I2c
