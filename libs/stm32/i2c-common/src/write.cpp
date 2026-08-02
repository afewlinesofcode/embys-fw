#include "write.hpp"

namespace Embys::Stm32::I2c::Dev
{

Write::Write(I2c::BusCore &bus) : bus(bus)
{
}

void
Write::exec(uint8_t addr, std::span<const uint8_t> data, Cb cb)
{
  this->cb = cb;
  const I2c::Status result = bus.write(addr, data, {i2c_callback, this});

  if (!result)
    cb(-1);
}

void
Write::i2c_callback(void *ctx, I2c::Status result) noexcept
{
  static_cast<Write *>(ctx)->cb(result ? 0 : -1);
}

}; // namespace Embys::Stm32::I2c::Dev
