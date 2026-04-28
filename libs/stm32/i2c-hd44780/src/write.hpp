#pragma once

#include <stdint.h>

#include <embys/stm32/i2c-common/write.hpp>
#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

// Single-byte write to the LCD backpack at a fixed I2C address.
class Write
{
public:
  Write(I2c::Bus *bus, uint8_t addr7);

  void
  exec(uint8_t data, Cb cb);

private:
  I2c::Dev::Write write;
  uint8_t addr7;
  uint8_t data_byte = 0;
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
