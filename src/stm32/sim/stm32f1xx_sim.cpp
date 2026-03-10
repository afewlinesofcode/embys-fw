#include "stm32f1xx_sim.hpp"

namespace Embys::Stm32::Sim
{

void
reset()
{
  Core::reset();
  Base::reset();
  I2C::reset();
  Uart::reset();
}

}; // namespace Embys::Stm32::Sim
