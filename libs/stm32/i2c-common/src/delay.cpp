#include "delay.hpp"

namespace Embys::Stm32::I2c::Dev
{

Delay::Delay(Base::LoopCore &loop)
  : ev(loop, Base::EventMode::Deferred, {fired, this})
{
}

void
Delay::exec(std::chrono::microseconds delay, Cb cb)
{
  this->cb = cb;
  if (!ev.enable(delay))
    cb(-1);
}

void
Delay::fired(void *ctx) noexcept
{
  static_cast<Delay *>(ctx)->cb(0);
}

}; // namespace Embys::Stm32::I2c::Dev
