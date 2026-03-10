#include "i2c.hpp"

#include "base.hpp"

namespace Embys::Stm32::Mock
{

namespace I2C
{

I2C_TypeDef *i2c = &i2c1_instance; // Default to i2c1

void
reset()
{
  i2c = &i2c1_instance;
  runtime.reset();
}

void
simulate_busy()
{
  i2c->SR2 = I2C_SR2_BUSY; // Bus busy
}

}; // namespace I2C

}; // namespace Embys::Stm32::Mock
