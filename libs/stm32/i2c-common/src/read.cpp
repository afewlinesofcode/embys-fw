#include "read.hpp"

#include <cstring>

namespace Embys::Stm32::I2c::Dev
{

Read::Read(I2c::BusCore &bus) : bus(bus)
{
}

void
Read::exec(uint8_t addr, std::span<uint8_t> destination, Cb cb)
{
  this->cb = cb;
  if (destination.size() > UINT16_MAX)
  {
    cb(I2c::BUFFER_TOO_SMALL);
    return;
  }
  this->destination = destination;
  int rc = bus.read(addr, static_cast<uint16_t>(destination.size()),
                    {i2c_callback, this});

  if (rc != 0)
    cb(rc);
}

void
Read::exec(uint8_t addr, uint8_t reg, std::span<uint8_t> destination, Cb cb)
{
  this->cb = cb;
  if (destination.size() > UINT16_MAX)
  {
    cb(I2c::BUFFER_TOO_SMALL);
    return;
  }
  this->destination = destination;
  int rc = bus.read(addr, reg, static_cast<uint16_t>(destination.size()),
                    {i2c_callback, this});

  if (rc != 0)
    cb(rc);
}

void
Read::i2c_callback(void *ctx, int result, std::span<const uint8_t> data)
{
  auto *self = static_cast<Read *>(ctx);
  if (result == 0 && data.size() == self->destination.size())
    std::memcpy(self->destination.data(), data.data(), data.size());
  self->cb(result);
}

}; // namespace Embys::Stm32::I2c::Dev
