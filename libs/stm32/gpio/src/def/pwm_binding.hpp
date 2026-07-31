#pragma once

#include <embys/stm32/base/timer.hpp>

namespace Embys::Stm32::Gpio
{

struct PwmBinding
{
  Base::Timer *timer = nullptr;
  uint8_t channel = 0;
};

}; // namespace Embys::Stm32::Gpio
