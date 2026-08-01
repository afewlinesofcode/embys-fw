#include "cs.hpp"

#include "stm32xx.hpp"

namespace Embys::Stm32
{

uint32_t cs_primask = 0;
uint32_t cs_stack = 0;

IrqGuard::IrqGuard()
{
  if (cs_stack == 0)
  {
    cs_primask = __get_PRIMASK();
    __disable_irq();
  }
  ++cs_stack;
}

IrqGuard::~IrqGuard()
{
  if (cs_stack > 0)
  {
    --cs_stack;
    if (cs_stack == 0)
      __set_PRIMASK(cs_primask);
  }
}

} // namespace Embys::Stm32
