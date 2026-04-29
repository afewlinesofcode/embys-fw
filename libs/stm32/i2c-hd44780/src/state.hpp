/**
 * @file state.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief HD44780 LCD driver internal state
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

struct State
{
  uint8_t display_control = 0;
  uint8_t display_mode = 0;
  uint8_t cursor_col = 0;
  uint8_t cursor_row = 0;
  bool backlight = false;

  void
  advance_position();

  inline bool
  is_valid_position() const
  {
    return cursor_col < LCD_COLS && cursor_row < LCD_ROWS;
  }
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
