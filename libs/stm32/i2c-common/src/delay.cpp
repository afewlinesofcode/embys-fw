#include "delay.hpp"

namespace Embys::Stm32::I2c::Dev
{

Delay::Delay(Base::LoopCore *loop) : ev(*loop, 0, {fired, this})
{
}

void
Delay::exec(std::chrono::microseconds delay, Cb cb)
{
  this->cb = cb;
  int rc = ev.enable(delay);

  if (rc != 0)
    cb(rc);
}

void
Delay::fired(void *ctx)
{
  static_cast<Delay *>(ctx)->cb(0);
}

}; // namespace Embys::Stm32::I2c::Dev
