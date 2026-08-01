/**
 * @file create_char.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief HD44780 LCD custom character creation
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <cstdint>
#include <span>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class Send;

class CreateChar
{
public:
  explicit CreateChar(Send &send);

  void
  exec(uint8_t location, std::span<const uint8_t, 8> char_map, Cb cb);

private:
  Send &send;
  Cb cb;
  uint8_t location = 0;
  std::span<const uint8_t> char_map;
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
