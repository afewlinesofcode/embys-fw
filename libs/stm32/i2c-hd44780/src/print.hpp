/**
 * @file print.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief HD44780 LCD print string operation
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class State;
class Send;
class Clear;
class SetCursor;

class Print
{
public:
  Print(State *state, Send *send, Clear *clear, SetCursor *set_cursor);

  void
  set_auto_clear(bool enable)
  {
    auto_clear = enable;
  }

  inline bool
  is_auto_clear() const
  {
    return auto_clear;
  }

  void
  print_string(const char *text, Cb cb);

  void
  print_string_at(uint8_t row, uint8_t col, const char *text, Cb cb);

  void
  print_line(uint8_t row, const char *text, Cb cb);

  void
  clear_line(uint8_t row, Cb cb);

private:
  State *state;
  Send *send;
  Clear *clear;
  SetCursor *set_cursor;
  bool auto_clear = false;
  const char *text = nullptr;
  char current_char = '\0';
  uint8_t row = 0;
  uint8_t col = 0;
  Cb cb;
  uint16_t index = 0;
  bool text_ended = false;

  enum
  {
    String,
    StringAt,
    Line
  } request = String;

  enum
  {
    Start,
    SetCursorCmd,
    PrintChar,
    Done
  } stage = Start;

  void
  start();

  void
  started();

  void
  set_cursor_cmd();

  void
  print_next_char();

  void
  print_char();

  void
  advance_position();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
