#pragma once

#include <stdint.h>

#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev
{

class Read
{
public:
  explicit Read(I2c::Bus *bus);

  void
  exec(uint8_t addr, uint8_t *buf, uint16_t len, Cb cb);

  void
  exec(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len, Cb cb);

private:
  I2c::Bus *bus;
  Cb cb;

  static void
  i2c_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev
