#include "gpio.hpp"

namespace Embys::Stm32::Sim
{

namespace Gpio
{

void
trigger_pin(GPIO_TypeDef *port, uint8_t pin_index, uint8_t value)
{
  uint32_t pin_bit = (1 << pin_index);

  if (value)
  {
    SET_BIT_V(port->IDR, pin_bit);
  }
  else
  {
    CLEAR_BIT_V(port->IDR, pin_bit);
  }

  // Check EXTI configuration and set pending register if needed
  if (exti_instance.IMR & pin_bit)
  {
    bool trigger_interrupt = false;

    if ((exti_instance.RTSR & pin_bit) && value)
    {
      trigger_interrupt = true;
    }

    if ((exti_instance.FTSR & pin_bit) && !value)
    {
      trigger_interrupt = true;
    }

    if (trigger_interrupt)
    {
      CLEAR_BIT_V(exti_instance.PR, pin_bit);
    }
  }
}

}; // namespace Gpio

}; // namespace Embys::Stm32::Sim
