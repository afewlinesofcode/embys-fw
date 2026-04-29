#pragma once

#include <stdint.h>

#ifdef STM32_SIM
#include <string>
#include <vector>

#include <embys/stm32/sim/sim.hpp>

#define SIM_LOG(str) std::cout << str << std::endl

extern "C"
{
  void
  TIM2_IRQHandler();

  void
  EXTI0_IRQHandler();

  void
  I2C1_EV_IRQHandler();

  void
  I2C1_ER_IRQHandler();
}

inline void
SIM_RESET()
{
  Embys::Stm32::Sim::reset();
  Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
  Embys::Stm32::Sim::EXTI0_IRQHandler_ptr = EXTI0_IRQHandler;
  Embys::Stm32::Sim::I2C1_EV_IRQHandler_ptr = I2C1_EV_IRQHandler;
  Embys::Stm32::Sim::I2C1_ER_IRQHandler_ptr = I2C1_ER_IRQHandler;

  Embys::Stm32::Sim::register_int_signal();

  Embys::Stm32::Sim::input_pipe.register_command(
      "btn_toggle",
      [](const std::string &, const std::vector<std::string> &)
      {
        std::cout << "Simulating button click" << std::endl;
        Embys::Stm32::Sim::Gpio::trigger_pin(GPIOA, 0, 0);
        Embys::Stm32::Sim::Base::add_delayed_hook(
            5, [](uint32_t)
            { Embys::Stm32::Sim::Gpio::trigger_pin(GPIOA, 0, 1); });
      });
}
#else
#define SIM_LOG(str)
#define SIM_RESET()
#endif
