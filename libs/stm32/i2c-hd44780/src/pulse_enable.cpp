#include "pulse_enable.hpp"

#include "write.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

PulseEnable::PulseEnable(Write *write, I2c::Dev::Delay *delay)
  : write(write), delay(delay)
{
}

void
PulseEnable::exec(uint8_t data, Cb cb)
{
  this->cb = cb;
  high = data | LCD_EN;
  low = static_cast<uint8_t>(data & ~LCD_EN);
  timeout_us(1, Timeout1);
}

void
PulseEnable::timeout_us(uint32_t us, Stage st)
{
  stage = st;
  delay->exec(us, {command_callback, this});
}

void
PulseEnable::write_en_high()
{
  stage = WriteENHigh;
  write->exec(high, {command_callback, this});
}

void
PulseEnable::write_en_low()
{
  stage = WriteENLow;
  write->exec(low, {command_callback, this});
}

void
PulseEnable::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<PulseEnable *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case Timeout1:
      cmd->write_en_high();
      break;
    case WriteENHigh:
      cmd->timeout_us(1, Timeout2);
      break;
    case Timeout2:
      cmd->write_en_low();
      break;
    case WriteENLow:
      cmd->timeout_us(50, Timeout3);
      break;
    case Timeout3:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
