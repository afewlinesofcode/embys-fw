#include "write.hpp"

namespace Embys::Stm32::I2c::Dev
{

Write::Write(I2c::Bus *bus) : bus(bus)
{
}

void
Write::exec(uint8_t addr, const uint8_t *buf, uint16_t len, Cb cb)
{
  this->cb = cb;
  int rc = bus->write(addr, buf, len, {i2c_callback, this});
  if (rc != 0)
    cb(rc);
}

void
Write::i2c_callback(void *ctx, int result)
{
  static_cast<Write *>(ctx)->cb(result);
}

}; // namespace Embys::Stm32::I2c::Dev
