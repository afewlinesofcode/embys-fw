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
    cb(-1);
    return;
  }
  this->destination = destination;
  const I2c::Status result = bus.read(
      addr, static_cast<uint16_t>(destination.size()), {i2c_callback, this});

  if (!result)
    cb(-1);
}

void
Read::exec(uint8_t addr, uint8_t reg, std::span<uint8_t> destination, Cb cb)
{
  this->cb = cb;
  if (destination.size() > UINT16_MAX)
  {
    cb(-1);
    return;
  }
  this->destination = destination;
  const I2c::Status result =
      bus.read(addr, reg, static_cast<uint16_t>(destination.size()),
               {i2c_callback, this});

  if (!result)
    cb(-1);
}

void
Read::i2c_callback(void *ctx, I2c::ReadResult result) noexcept
{
  auto *self = static_cast<Read *>(ctx);
  if (!result)
  {
    self->cb(-1);
    return;
  }

  const std::span<const uint8_t> data = result.value();
  if (data.size() != self->destination.size())
  {
    self->cb(-1);
    return;
  }

  std::memcpy(self->destination.data(), data.data(), data.size());
  self->cb(0);
}

}; // namespace Embys::Stm32::I2c::Dev
