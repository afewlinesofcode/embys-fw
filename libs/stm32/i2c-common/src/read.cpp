#include "read.hpp"

#include <cstring>

namespace Embys::Stm32::I2c::Dev
{

Read::Read(I2c::BusCore *bus) : bus(bus)
{
}

void
Read::exec(uint8_t addr, uint8_t *buf, uint16_t len, Cb cb)
{
  this->cb = cb;
  destination = buf;
  destination_len = len;
  int rc = bus->read(addr, len, {i2c_callback, this});

  if (rc != 0)
    cb(rc);
}

void
Read::exec(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len, Cb cb)
{
  this->cb = cb;
  destination = buf;
  destination_len = len;
  int rc = bus->read(addr, reg, len, {i2c_callback, this});

  if (rc != 0)
    cb(rc);
}

void
Read::i2c_callback(void *ctx, int result, std::span<const uint8_t> data)
{
  auto *self = static_cast<Read *>(ctx);
  if (result == 0 && data.size() == self->destination_len)
    std::memcpy(self->destination, data.data(), data.size());
  self->cb(result);
}

}; // namespace Embys::Stm32::I2c::Dev
