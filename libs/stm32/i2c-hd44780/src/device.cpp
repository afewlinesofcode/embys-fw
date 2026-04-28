#include "device.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Device::Device(Base::Loop *loop, I2c::Bus *bus, uint8_t addr7)
  : cmd_timeout(loop), cmd_write(bus, addr7),
    cmd_pulse_enable(&cmd_write, &cmd_timeout),
    cmd_write_bits(&state, &cmd_pulse_enable), cmd_send(&cmd_write_bits),
    cmd_set_cursor(&state, &cmd_send),
    cmd_clear(&cmd_send, &cmd_set_cursor, &cmd_timeout),
    cmd_home(&state, &cmd_send, &cmd_timeout),
    cmd_print(&state, &cmd_send, &cmd_clear, &cmd_set_cursor),
    cmd_create_char(&cmd_send), cmd_init(&state, &cmd_write, &cmd_write_bits,
                                         &cmd_send, &cmd_clear, &cmd_timeout)
{
}

void
Device::enable(Cb cb)
{
  enable_cb = cb;
  cmd_init.exec({initialized_callback, this});
}

void
Device::clear(Cb cb)
{
  cmd_clear.exec(cb);
}

void
Device::home(Cb cb)
{
  cmd_home.exec(cb);
}

void
Device::set_cursor(uint8_t col, uint8_t row, Cb cb)
{
  cmd_set_cursor.exec(col, row, cb);
}

void
Device::print(const char *str, Cb cb)
{
  cmd_print.print_string(str, cb);
}

void
Device::print_at(uint8_t row, uint8_t col, const char *str, Cb cb)
{
  cmd_print.print_string_at(row, col, str, cb);
}

void
Device::print_line(uint8_t row, const char *str, Cb cb)
{
  cmd_print.print_line(row, str, cb);
}

void
Device::clear_line(uint8_t line, Cb cb)
{
  cmd_print.clear_line(line, cb);
}

void
Device::display(bool on, Cb cb)
{
  if (on)
    state.display_control =
        static_cast<uint8_t>(state.display_control | LCD_DISPLAY_ON);
  else
    state.display_control =
        static_cast<uint8_t>(state.display_control & ~LCD_DISPLAY_ON);

  cmd_send.exec(state.display_control, LCD_COMMAND, cb);
}

void
Device::cursor(bool on, Cb cb)
{
  if (on)
    state.display_control =
        static_cast<uint8_t>(state.display_control | LCD_CURSOR_ON);
  else
    state.display_control =
        static_cast<uint8_t>(state.display_control & ~LCD_CURSOR_ON);

  cmd_send.exec(state.display_control, LCD_COMMAND, cb);
}

void
Device::blink(bool on, Cb cb)
{
  if (on)
    state.display_control =
        static_cast<uint8_t>(state.display_control | LCD_BLINK_ON);
  else
    state.display_control =
        static_cast<uint8_t>(state.display_control & ~LCD_BLINK_ON);

  cmd_send.exec(state.display_control, LCD_COMMAND, cb);
}

void
Device::backlight(bool on, Cb cb)
{
  state.backlight = on;
  cmd_write.exec(0x00, cb);
}

void
Device::scroll_left(Cb cb)
{
  cmd_send.exec(
      static_cast<uint8_t>(LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE | LCD_MOVE_LEFT),
      LCD_COMMAND, cb);
}

void
Device::scroll_right(Cb cb)
{
  cmd_send.exec(static_cast<uint8_t>(LCD_CURSOR_SHIFT | LCD_DISPLAY_MOVE |
                                     LCD_MOVE_RIGHT),
                LCD_COMMAND, cb);
}

void
Device::set_text_direction(bool ltr, Cb cb)
{
  if (ltr)
    state.display_mode =
        static_cast<uint8_t>(state.display_mode | LCD_ENTRY_LEFT);
  else
    state.display_mode =
        static_cast<uint8_t>(state.display_mode & ~LCD_ENTRY_LEFT);

  cmd_send.exec(state.display_mode, LCD_COMMAND, cb);
}

void
Device::autoscroll(bool on, Cb cb)
{
  if (on)
    state.display_mode =
        static_cast<uint8_t>(state.display_mode | LCD_ENTRY_SHIFT_INCREMENT);
  else
    state.display_mode =
        static_cast<uint8_t>(state.display_mode & ~LCD_ENTRY_SHIFT_INCREMENT);

  cmd_send.exec(state.display_mode, LCD_COMMAND, cb);
}

void
Device::create_char(uint8_t location, const uint8_t char_map[8], Cb cb)
{
  cmd_create_char.exec(location, char_map, cb);
}

void
Device::initialized_callback(void *ctx, int result)
{
  auto device = static_cast<Device *>(ctx);
  if (result != 0)
  {
    device->enable_cb(result);
    return;
  }
  device->initialized = true;
  device->enable_cb(0);
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
