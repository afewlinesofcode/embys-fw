#pragma once

#if defined(STM32_SIM)
#include <embys/stm32/sim/sim/stm32xx.hpp>
#elif defined(STM32F1xx)
#include <stm32f1xx.h>
#elif defined(STM32F4xx)
#include <stm32f4xx.h>
#else
#error "Unsupported STM32 family. Supported families: STM32F1 and STM32F4"
#endif
