#pragma once

namespace Embys::Stm32
{

struct Stm32f1
{
};

struct Stm32f4
{
};

#if defined(STM32F1xx)

using Family = Stm32f1;

#elif defined(STM32F4xx)

using Family = Stm32f4;

#else

#error "Unsupported or unspecified STM32 family"

#endif

}; // namespace Embys::Stm32
