#include "write_bits.hpp"

#include <embys/stm32/debug.hpp>

#include "pulse_enable.hpp"
#include "state.hpp"
#include "write.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

WriteBits::WriteBits(State *state, PulseEnable *pulse_enable)
  : state(state), pulse_enable(pulse_enable), write(pulse_enable->get_write())
{
}

void
WriteBits::exec(uint8_t value, uint8_t mode, Cb cb)
{
  this->cb = cb;
  data = 0;
  data =
      static_cast<uint8_t>(data | ((value << LCD_DATA_SHIFT) & LCD_DATA_MASK));
  data = static_cast<uint8_t>(data | mode);

  if (state->backlight)
    data = static_cast<uint8_t>(data | LCD_BL);

  write_i2c_cmd();
}

void
WriteBits::write_i2c_cmd()
{
  stage = WriteI2cCmd;
  write->exec(data, {command_callback, this});
}

void
WriteBits::write_pulse()
{
  stage = WritePulse;
  pulse_enable->exec(data, {command_callback, this});
}

void
WriteBits::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<WriteBits *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case WriteI2cCmd:
      cmd->write_pulse();
      break;
    case WritePulse:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
