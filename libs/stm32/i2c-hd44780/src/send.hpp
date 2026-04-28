#pragma once

#include <stdint.h>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class WriteBits;

class Send
{
public:
  explicit Send(WriteBits *write_bits);

  void
  exec(uint8_t value, uint8_t mode, Cb cb);

private:
  WriteBits *write_bits;
  Cb cb;
  uint8_t mode = 0;
  uint8_t high = 0;
  uint8_t low = 0;

  enum
  {
    WriteHigh,
    WriteLow
  } stage = WriteHigh;

  void
  write_high();

  void
  write_low();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
