#include "write.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Write::Write(I2c::BusCore &bus, uint8_t addr7) : write(bus), addr7(addr7)
{
}

void
Write::exec(uint8_t data, Cb cb)
{
  data_byte = data;
  write.exec(addr7, std::span{&data_byte, 1U}, cb);
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
