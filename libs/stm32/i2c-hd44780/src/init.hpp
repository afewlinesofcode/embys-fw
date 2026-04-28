#pragma once

#include <stdint.h>

#include <embys/stm32/i2c-common/delay.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class State;
class Write;
class WriteBits;
class Send;
class Clear;

class Init
{
public:
  Init(State *state, Write *write, WriteBits *write_bits, Send *send,
       Clear *clear, I2c::Dev::Delay *timeout);

  void
  exec(Cb cb);

private:
  State *state;
  Write *write;
  WriteBits *write_bits;
  Send *send;
  Clear *clear;
  I2c::Dev::Delay *timeout;
  Cb cb;

  enum Stage : uint8_t
  {
    Timeout1 = 0,
    InitStateMode,
    Timeout2,
    Mode8Bit1,
    Timeout3,
    Mode8Bit2,
    Timeout4,
    Mode8Bit3,
    Timeout5,
    Mode4Bit,
    Timeout6,
    FunctionSet,
    DisplayControlInit,
    ClearDisplay,
    EntryModeSet,
    DisplayOn,
    BacklightOn
  } stage = Timeout1;

  void
  timeout_us(uint32_t us, Stage st);

  void
  write_cmd(uint8_t data, Stage st);

  void
  write_bits_cmd(uint8_t data, Stage st);

  void
  send_cmd(uint8_t command, Stage st);

  void
  clear_display();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
