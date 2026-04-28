#pragma once

#include <stdint.h>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class State;
class PulseEnable;
class Write;

class WriteBits
{
public:
  WriteBits(State *state, PulseEnable *pulse_enable);

  void
  exec(uint8_t value, uint8_t mode, Cb cb);

private:
  State *state;
  PulseEnable *pulse_enable;
  Write *write;
  Cb cb;
  uint8_t data = 0;

  enum
  {
    WriteI2cCmd,
    WritePulse
  } stage = WriteI2cCmd;

  void
  write_i2c_cmd();

  void
  write_pulse();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
