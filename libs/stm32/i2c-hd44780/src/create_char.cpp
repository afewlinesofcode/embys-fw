#include "create_char.hpp"

#include "send.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

CreateChar::CreateChar(Send *send) : send(send)
{
}

void
CreateChar::exec(uint8_t location, const uint8_t char_map[8], Cb cb)
{
  this->location = location;
  this->char_map = char_map;
  this->cb = cb;
  byte_index = 0;
  set_cgram_address();
}

void
CreateChar::set_cgram_address()
{
  stage = SetCGRAM;
  uint8_t address = static_cast<uint8_t>((location & 0x07u) << 3);
  send->exec(static_cast<uint8_t>(LCD_SET_CGRAM_ADDR | address), LCD_COMMAND,
             {command_callback, this});
}

void
CreateChar::write_bytes()
{
  stage = WriteBytes;

  if (byte_index >= 8)
  {
    set_ddram_address();
    return;
  }

  send->exec(char_map[byte_index++], LCD_DATA, {command_callback, this});
}

void
CreateChar::set_ddram_address()
{
  stage = SetDDRAM;
  send->exec(LCD_SET_DDRAM_ADDR, LCD_COMMAND, {command_callback, this});
}

void
CreateChar::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<CreateChar *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case SetCGRAM:
      cmd->write_bytes();
      break;
    case WriteBytes:
      cmd->write_bytes();
      break;
    case SetDDRAM:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
