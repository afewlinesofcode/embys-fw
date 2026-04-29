#include "delay.hpp"

namespace Embys::Stm32::I2c::Dev
{

Delay::Delay(Base::Loop *loop) : ev(loop, 0, {fired, this})
{
}

void
Delay::exec(uint32_t us, Cb cb)
{
  this->cb = cb;
  int rc = ev.enable(us);

  if (rc != 0)
    cb(rc);
}

void
Delay::fired(void *ctx)
{
  static_cast<Delay *>(ctx)->cb(0);
}

}; // namespace Embys::Stm32::I2c::Dev
