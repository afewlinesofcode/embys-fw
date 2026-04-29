#include "home.hpp"

#include "send.hpp"
#include "state.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Home::Home(State *state, Send *send, I2c::Dev::Delay *timeout)
  : state(state), send(send), timeout(timeout)
{
}

void
Home::exec(Cb cb)
{
  this->cb = cb;
  send_command();
}

void
Home::send_command()
{
  stage = SendCommand;
  send->exec(LCD_RETURN_HOME, LCD_COMMAND, {command_callback, this});
}

void
Home::wait()
{
  stage = Wait;
  timeout->exec(2000, {command_callback, this});
}

void
Home::update_state()
{
  stage = UpdateState;
  state->cursor_col = 0;
  state->cursor_row = 0;
  command_callback(this, 0);
}

void
Home::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<Home *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case SendCommand:
      cmd->wait();
      break;
    case Wait:
      cmd->update_state();
      break;
    case UpdateState:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
