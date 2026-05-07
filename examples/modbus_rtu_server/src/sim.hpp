#pragma once

#ifdef STM32_SIM

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <stdint.h>

#include <embys/stm32/sim/sim.hpp>
#include <embys/stm32/types.hpp>

#include "def.hpp"

#define SIM_LOG(str) std::cout << str << std::endl

extern "C"
{
  void
  TIM2_IRQHandler();

  void
  USART1_IRQHandler();

  void
  I2C1_EV_IRQHandler();

  void
  I2C1_ER_IRQHandler();
}

void
SIM_RESET(AppContext *context);

#else
#define SIM_LOG(str)
#define SIM_RESET(context)
#endif
