#include "set_cursor.hpp"

#include "send.hpp"
#include "state.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

SetCursor::SetCursor(State *state, Send *send) : state(state), send(send)
{
}

void
SetCursor::exec(uint8_t col, uint8_t row, Cb cb)
{
  this->col = col;
  this->row = row;
  this->cb = cb;
  send_command();
}

void
SetCursor::send_command()
{
  stage = SendCommand;
  uint8_t address = static_cast<uint8_t>(col + ROW_ADDRESSES[row]);
  send->exec(static_cast<uint8_t>(LCD_SET_DDRAM_ADDR | address), LCD_COMMAND,
             {command_callback, this});
}

void
SetCursor::update_state()
{
  stage = UpdateState;
  state->cursor_col = col;
  state->cursor_row = row;
  command_callback(this, 0);
}

void
SetCursor::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<SetCursor *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case SendCommand:
      cmd->update_state();
      break;
    case UpdateState:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
