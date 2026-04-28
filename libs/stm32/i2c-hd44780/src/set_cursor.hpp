#pragma once

#include <stdint.h>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class State;
class Send;

class SetCursor
{
public:
  SetCursor(State *state, Send *send);

  void
  exec(uint8_t col, uint8_t row, Cb cb);

private:
  State *state;
  Send *send;
  uint8_t col = 0;
  uint8_t row = 0;
  Cb cb;

  enum
  {
    SendCommand,
    UpdateState
  } stage = SendCommand;

  void
  send_command();

  void
  update_state();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
