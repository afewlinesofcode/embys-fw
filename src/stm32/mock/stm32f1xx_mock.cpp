#include "stm32f1xx_mock.hpp"

namespace Embys::Stm32::Mock
{

void
reset()
{
  Core::reset();
  Base::reset();
  I2C::reset();
  Uart::reset();
}

}; // namespace Embys::Stm32::Mock
