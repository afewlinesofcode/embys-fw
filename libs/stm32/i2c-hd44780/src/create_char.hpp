#pragma once

#include <stdint.h>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class Send;

class CreateChar
{
public:
  explicit CreateChar(Send *send);

  void
  exec(uint8_t location, const uint8_t char_map[8], Cb cb);

private:
  Send *send;
  Cb cb;
  uint8_t location = 0;
  const uint8_t *char_map = nullptr;
  uint8_t byte_index = 0;

  enum
  {
    SetCGRAM,
    WriteBytes,
    SetDDRAM,
  } stage = SetCGRAM;

  void
  set_cgram_address();

  void
  write_bytes();

  void
  set_ddram_address();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
