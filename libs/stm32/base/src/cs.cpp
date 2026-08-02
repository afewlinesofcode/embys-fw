#include "cs.hpp"

#include "stm32xx.hpp"

namespace Embys::Stm32
{

IrqGuard::IrqGuard() : primask(__get_PRIMASK())
{
  __disable_irq();
}

IrqGuard::~IrqGuard()
{
  __set_PRIMASK(primask);
}

} // namespace Embys::Stm32
