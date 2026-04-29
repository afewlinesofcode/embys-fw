#include "send.hpp"

#include "write_bits.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Send::Send(WriteBits *write_bits) : write_bits(write_bits)
{
}

void
Send::exec(uint8_t value, uint8_t mode, Cb cb)
{
  this->cb = cb;
  this->mode = mode;
  high = static_cast<uint8_t>(value >> 4);
  low = static_cast<uint8_t>(value & 0x0F);
  write_high();
}

void
Send::write_high()
{
  stage = WriteHigh;
  write_bits->exec(high, mode, {command_callback, this});
}

void
Send::write_low()
{
  stage = WriteLow;
  write_bits->exec(low, mode, {command_callback, this});
}

void
Send::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<Send *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case WriteHigh:
      cmd->write_low();
      break;
    case WriteLow:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
