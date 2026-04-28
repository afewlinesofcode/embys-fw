#include "clear.hpp"

#include "send.hpp"
#include "set_cursor.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Clear::Clear(Send *send, SetCursor *set_cursor, I2c::Dev::Delay *timeout)
  : send(send), set_cursor(set_cursor), timeout(timeout)
{
}

void
Clear::exec(Cb cb)
{
  this->cb = cb;
  send_clear_command();
}

void
Clear::send_clear_command()
{
  stage = SendClearCommand;
  send->exec(LCD_CLEAR_DISPLAY, LCD_COMMAND, {command_callback, this});
}

void
Clear::wait_clear()
{
  stage = WaitClear;
  timeout->exec(2000, {command_callback, this});
}

void
Clear::set_cursor_home()
{
  stage = SetCursorHome;
  set_cursor->exec(0, 0, {command_callback, this});
}

void
Clear::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<Clear *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case SendClearCommand:
      cmd->wait_clear();
      break;
    case WaitClear:
      cmd->set_cursor_home();
      break;
    case SetCursorHome:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
