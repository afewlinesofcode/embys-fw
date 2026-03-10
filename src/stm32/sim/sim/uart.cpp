#include "uart.hpp"

#include "base.hpp"

namespace Embys::Stm32::Sim
{

namespace Uart
{

USART_TypeDef *usart = &usart1_instance; // Default to usart1

void
reset()
{
  usart = &usart1_instance;
  runtime.reset();
}

} // namespace Uart

} // namespace Embys::Stm32::Sim
