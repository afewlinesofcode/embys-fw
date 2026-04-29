/**
 * @file clear.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief HD44780 LCD clear display operation
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/i2c-common/delay.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class Send;
class SetCursor;

class Clear
{
public:
  Clear(Send *send, SetCursor *set_cursor, I2c::Dev::Delay *timeout);

  void
  exec(Cb cb);

private:
  Send *send;
  SetCursor *set_cursor;
  I2c::Dev::Delay *timeout;
  Cb cb;

  enum
  {
    SendClearCommand,
    WaitClear,
    SetCursorHome
  } stage = SendClearCommand;

  void
  send_clear_command();

  void
  wait_clear();

  void
  set_cursor_home();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
