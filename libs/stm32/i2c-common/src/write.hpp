#pragma once

#include <stdint.h>

#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev
{

class Write
{
public:
  explicit Write(I2c::Bus *bus);

  void
  exec(uint8_t addr, const uint8_t *buf, uint16_t len, Cb cb);

private:
  I2c::Bus *bus;
  Cb cb;

  static void
  i2c_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev
