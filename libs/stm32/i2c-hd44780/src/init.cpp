#include "init.hpp"

#include "clear.hpp"
#include "send.hpp"
#include "state.hpp"
#include "write.hpp"
#include "write_bits.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Init::Init(State *state, Write *write, WriteBits *write_bits, Send *send,
           Clear *clear, I2c::Dev::Delay *delay)
  : state(state), write(write), write_bits(write_bits), send(send),
    clear(clear), delay(delay)
{
}

void
Init::exec(Cb cb)
{
  this->cb = cb;
  state->backlight = false;
  delay_us(50000, Delay1);
}

void
Init::delay_us(uint32_t us, Stage st)
{
  stage = st;
  delay->exec(us, {command_callback, this});
}

void
Init::write_cmd(uint8_t data, Stage st)
{
  stage = st;
  write->exec(data, {command_callback, this});
}

void
Init::write_bits_cmd(uint8_t data, Stage st)
{
  stage = st;
  write_bits->exec(data, LCD_COMMAND, {command_callback, this});
}

void
Init::send_cmd(uint8_t command, Stage st)
{
  stage = st;
  send->exec(command, LCD_COMMAND, {command_callback, this});
}

void
Init::clear_display()
{
  stage = ClearDisplay;
  clear->exec({command_callback, this});
}

void
Init::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<Init *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case Delay1:
      cmd->write_cmd(0x00, InitStateMode);
      break;
    case InitStateMode:
      cmd->delay_us(1000, Delay2);
      break;
    case Delay2:
      cmd->write_bits_cmd(0x03, Mode8Bit1);
      break;
    case Mode8Bit1:
      cmd->delay_us(5000, Delay3);
      break;
    case Delay3:
      cmd->write_bits_cmd(0x03, Mode8Bit2);
      break;
    case Mode8Bit2:
      cmd->delay_us(5000, Delay4);
      break;
    case Delay4:
      cmd->write_bits_cmd(0x03, Mode8Bit3);
      break;
    case Mode8Bit3:
      cmd->delay_us(150, Delay5);
      break;
    case Delay5:
      cmd->write_bits_cmd(0x02, Mode4Bit);
      break;
    case Mode4Bit:
      cmd->delay_us(150, Delay6);
      break;
    case Delay6:
      cmd->send_cmd(LCD_FUNCTION_SET | LCD_4BIT_MODE | LCD_2LINE | LCD_5x8DOTS,
                    FunctionSet);
      break;
    case FunctionSet:
      cmd->send_cmd(LCD_DISPLAY_CONTROL | LCD_DISPLAY_OFF | LCD_CURSOR_OFF |
                        LCD_BLINK_OFF,
                    DisplayControlInit);
      break;
    case DisplayControlInit:
      cmd->clear_display();
      break;
    case ClearDisplay:
      cmd->send_cmd(LCD_ENTRY_MODE_SET | LCD_ENTRY_LEFT |
                        LCD_ENTRY_SHIFT_DECREMENT,
                    EntryModeSet);
      break;
    case EntryModeSet:
      cmd->send_cmd(LCD_DISPLAY_CONTROL | LCD_DISPLAY_ON | LCD_CURSOR_OFF |
                        LCD_BLINK_OFF,
                    DisplayOn);
      break;
    case DisplayOn:
      cmd->state->backlight = true;
      cmd->write_cmd(0x00, BacklightOn);
      break;
    case BacklightOn:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
