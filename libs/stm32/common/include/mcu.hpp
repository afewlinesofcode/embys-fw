#pragma once

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
};

template <>
struct DeviceTraits<Stm32f407xx>
{
  using Family = Stm32f4;
  static constexpr const char *name = "stm32f407xx";
};

template <>
struct DeviceTraits<Stm32f411xe>
{
  using Family = Stm32f4;
  static constexpr const char *name = "stm32f411xe";
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
