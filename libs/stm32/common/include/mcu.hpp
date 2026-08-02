#pragma once

#include <cstdint>

namespace Embys::Stm32
{

struct Stm32f1
{
};

struct Stm32f4
{
};

struct Stm32f103xb
{
};

struct Stm32f407xx
{
};

struct Stm32f411xe
{
};

template <typename Device>
struct DeviceTraits;

template <>
struct DeviceTraits<Stm32f103xb>
{
  using Family = Stm32f1;
  static constexpr const char *name = "stm32f103xb";
  static constexpr bool has_data_cache = false;
  static constexpr bool has_instruction_cache = false;
  static constexpr bool gpio_clock_ahb1 = false;
  static constexpr bool has_gpio_afr = false;
  static constexpr bool has_afio = true;
  static constexpr bool has_syscfg_exti = false;
  static constexpr uint32_t timer2_arr_max = UINT16_MAX;
};

template <>
struct DeviceTraits<Stm32f407xx>
{
  using Family = Stm32f4;
  static constexpr const char *name = "stm32f407xx";
  static constexpr bool has_data_cache = false;
  static constexpr bool has_instruction_cache = false;
  static constexpr bool gpio_clock_ahb1 = true;
  static constexpr bool has_gpio_afr = true;
  static constexpr bool has_afio = false;
  static constexpr bool has_syscfg_exti = true;
  static constexpr uint32_t timer2_arr_max = UINT32_MAX;
};

template <>
struct DeviceTraits<Stm32f411xe>
{
  using Family = Stm32f4;
  static constexpr const char *name = "stm32f411xe";
  static constexpr bool has_data_cache = false;
  static constexpr bool has_instruction_cache = false;
  static constexpr bool gpio_clock_ahb1 = true;
  static constexpr bool has_gpio_afr = true;
  static constexpr bool has_afio = false;
  static constexpr bool has_syscfg_exti = true;
  static constexpr uint32_t timer2_arr_max = UINT32_MAX;
};

template <typename Device>
using FamilyOf = typename DeviceTraits<Device>::Family;

#if defined(STM32F103xB)

using TargetDevice = Stm32f103xb;

#elif defined(STM32F407xx)

using TargetDevice = Stm32f407xx;

#elif defined(STM32F411xE)

using TargetDevice = Stm32f411xe;

#else

#error "Unsupported STM32 target. Supported: STM32F103xB, STM32F407xx, STM32F411xE"

#endif

using Family = FamilyOf<TargetDevice>;

}; // namespace Embys::Stm32
