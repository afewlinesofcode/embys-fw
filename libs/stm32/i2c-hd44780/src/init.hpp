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
       Clear *clear, I2c::Dev::Delay *delay);

  void
  exec(Cb cb);

private:
  State *state;
  Write *write;
  WriteBits *write_bits;
  Send *send;
  Clear *clear;
  I2c::Dev::Delay *delay;
  Cb cb;

  enum Stage : uint8_t
  {
    Delay1 = 0,
    InitStateMode,
    Delay2,
    Mode8Bit1,
    Delay3,
    Mode8Bit2,
    Delay4,
    Mode8Bit3,
    Delay5,
    Mode4Bit,
    Delay6,
    FunctionSet,
    DisplayControlInit,
    ClearDisplay,
    EntryModeSet,
    DisplayOn,
    BacklightOn
  } stage = Delay1;

  void
  delay_us(uint32_t us, Stage st);

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
