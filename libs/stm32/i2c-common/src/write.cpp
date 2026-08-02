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
  int rc = bus.write(addr, data, {i2c_callback, this});

  if (rc != 0)
    cb(rc);
}

void
Write::i2c_callback(void *ctx, int result) noexcept
{
  static_cast<Write *>(ctx)->cb(result);
}

}; // namespace Embys::Stm32::I2c::Dev
