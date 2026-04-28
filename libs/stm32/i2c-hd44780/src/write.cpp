#include "write.hpp"

#include <embys/stm32/debug.hpp>

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Write::Write(I2c::Bus *bus, uint8_t addr7) : write(bus), addr7(addr7)
{
}

void
Write::exec(uint8_t data, Cb cb)
{
  data_byte = data;
  write.exec(addr7, &data_byte, 1, cb);
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
