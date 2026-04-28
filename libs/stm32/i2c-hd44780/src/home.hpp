#pragma once

#include <stdint.h>

#include <embys/stm32/i2c-common/delay.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class State;
class Send;

class Home
{
public:
  Home(State *state, Send *send, I2c::Dev::Delay *timeout);

  void
  exec(Cb cb);

private:
  State *state;
  Send *send;
  I2c::Dev::Delay *timeout;
  Cb cb;

  enum
  {
    SendCommand,
    Wait,
    UpdateState
  } stage = SendCommand;

  void
  send_command();

  void
  wait();

  void
  update_state();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
