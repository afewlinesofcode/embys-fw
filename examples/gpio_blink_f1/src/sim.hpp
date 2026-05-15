#pragma once

#ifdef STM32_SIM

#include <embys/stm32/sim/sim.hpp>

#define SIM_LOG(str) std::cout << str << std::endl

// Forward declarations for interrupt handlers
extern "C"
{
  void
  TIM2_IRQHandler();
}

/**
 * @brief Reset the simulation environment
 */
inline void
SIM_RESET()
{
  // Reset simulation environment
  Embys::Stm32::Sim::reset();
  // Set global pointers for interrupt handlers
  Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;

  // Register signal handler for graceful shutdown
  Embys::Stm32::Sim::register_int_signal();
}
#else
#define SIM_LOG(str)
#define SIM_RESET()
#endif
