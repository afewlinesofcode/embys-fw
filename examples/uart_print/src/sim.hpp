#pragma once

#ifdef STM32_SIM
#include <embys/stm32/sim/sim.hpp>

#define SIM_LOG(str) std::cout << str << std::endl

// Forward-declare the handlers so the sim can call them
extern "C"
{
  void
  TIM2_IRQHandler();
  void
  USART1_IRQHandler();
}

inline void
SIM_RESET()
{
  Embys::Stm32::Sim::reset();
  Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
  Embys::Stm32::Sim::USART1_IRQHandler_ptr = USART1_IRQHandler;
  // The simulator defaults to usart1_instance which maps to USART1
  Embys::Stm32::Sim::register_int_signal();
}
#else
#define SIM_RESET()
#endif
