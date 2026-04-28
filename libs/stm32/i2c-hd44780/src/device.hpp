#pragma once

#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c-common/delay.hpp>
#include <embys/stm32/i2c/bus.hpp>

#include "clear.hpp"
#include "create_char.hpp"
#include "def.hpp"
#include "home.hpp"
#include "init.hpp"
#include "print.hpp"
#include "pulse_enable.hpp"
#include "send.hpp"
#include "set_cursor.hpp"
#include "state.hpp"
#include "write.hpp"
#include "write_bits.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

class Device
{
public:
  Device() = delete;
  Device(const Device &) = delete;
  Device(Device &&) = delete;
  Device &
  operator=(const Device &) = delete;
  Device &
  operator=(Device &&) = delete;

  Device(Base::Loop *loop, I2c::Bus *bus, uint8_t addr7 = 0x27u);

  void
  enable(Cb cb);

  void
  clear(Cb cb);

  void
  home(Cb cb);

  void
  set_cursor(uint8_t col, uint8_t row, Cb cb);

  void
  set_auto_clear(bool enable)
  {
    cmd_print.set_auto_clear(enable);
  }

  void
  print(const char *str, Cb cb);

  void
  print_at(uint8_t row, uint8_t col, const char *str, Cb cb);

  void
  print_line(uint8_t row, const char *str, Cb cb);

  void
  clear_line(uint8_t line, Cb cb);

  void
  display(bool on, Cb cb);

  void
  cursor(bool on, Cb cb);

  void
  blink(bool on, Cb cb);

  void
  backlight(bool on, Cb cb);

  void
  scroll_left(Cb cb);

  void
  scroll_right(Cb cb);

  void
  set_text_direction(bool ltr, Cb cb);

  void
  autoscroll(bool on, Cb cb);

  void
  create_char(uint8_t location, const uint8_t char_map[8], Cb cb);

  inline bool
  is_initialized() const
  {
    return initialized;
  }

  uint8_t
  get_cursor_col() const
  {
    return state.cursor_col;
  }

  uint8_t
  get_cursor_row() const
  {
    return state.cursor_row;
  }

  bool
  is_backlight_on() const
  {
    return state.backlight;
  }

private:
  I2c::Dev::Delay cmd_delay;
  Write cmd_write;
  PulseEnable cmd_pulse_enable;
  WriteBits cmd_write_bits;
  Send cmd_send;
  SetCursor cmd_set_cursor;
  Clear cmd_clear;
  Home cmd_home;
  Print cmd_print;
  CreateChar cmd_create_char;
  Init cmd_init;

  State state;
  Cb enable_cb;
  bool initialized = false;

  static void
  initialized_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
