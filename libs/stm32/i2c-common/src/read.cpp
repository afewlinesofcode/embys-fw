#include "read.hpp"

namespace Embys::Stm32::I2c::Dev
{

Read::Read(I2c::Bus *bus) : bus(bus)
{
}

void
Read::exec(uint8_t addr, uint8_t *buf, uint16_t len, Cb cb)
{
  this->cb = cb;
  int rc = bus->read(addr, buf, len, {i2c_callback, this});

  if (rc != 0)
    cb(rc);
}

void
Read::exec(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len, Cb cb)
{
  this->cb = cb;
  int rc = bus->read(addr, reg, buf, len, {i2c_callback, this});

  if (rc != 0)
    cb(rc);
}

void
Read::i2c_callback(void *ctx, int result)
{
  static_cast<Read *>(ctx)->cb(result);
}

}; // namespace Embys::Stm32::I2c::Dev
