#include "print.hpp"

#include "clear.hpp"
#include "send.hpp"
#include "set_cursor.hpp"
#include "state.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

Print::Print(State *state, Send *send, Clear *clear, SetCursor *set_cursor)
  : state(state), send(send), clear(clear), set_cursor(set_cursor)
{
}

void
Print::print_string(const char *text, Cb cb)
{
  request = String;
  this->text = text;
  this->cb = cb;

  if (text == nullptr)
  {
    cb(0);
    return;
  }

  start();
}

void
Print::print_string_at(uint8_t row, uint8_t col, const char *text, Cb cb)
{
  request = StringAt;
  this->text = text;
  this->row = row;
  this->col = col;
  this->cb = cb;

  if (text == nullptr)
  {
    cb(0);
    return;
  }

  start();
}

void
Print::print_line(uint8_t row, const char *text, Cb cb)
{
  request = Line;
  this->text = text;
  this->row = row;
  this->col = 0;
  this->cb = cb;

  if (text == nullptr)
  {
    cb(0);
    return;
  }

  start();
}

void
Print::clear_line(uint8_t row, Cb cb)
{
  print_line(row, "", cb);
}

void
Print::start()
{
  index = 0;
  text_ended = false;
  stage = Start;

  if (auto_clear)
    clear->exec({command_callback, this});
  else
    command_callback(this, 0);
}

void
Print::started()
{
  if (request == String)
  {
    print_next_char();
  }
  else if (request == StringAt || request == Line)
  {
    set_cursor_cmd();
  }
  else
  {
    cb(0);
  }
}

void
Print::set_cursor_cmd()
{
  stage = SetCursorCmd;
  set_cursor->exec(col, row, {command_callback, this});
}

void
Print::print_next_char()
{
  switch (request)
  {
    case String:
    case StringAt:
      current_char = text[index];
      break;
    case Line:
      current_char = text_ended ? ' ' : text[index];
      if (current_char == '\n')
        current_char = ' ';
      if (current_char == '\0' && index < LCD_COLS)
      {
        current_char = ' ';
        text_ended = true;
      }
      if (index >= LCD_COLS)
        current_char = '\0';
      break;
  }

  ++index;
  print_char();
}

void
Print::print_char()
{
  stage = PrintChar;

  if (current_char == '\0')
  {
    stage = Done;
    command_callback(this, 0);
    return;
  }

  if (current_char == '\n')
  {
    if (state->cursor_row < LCD_ROWS - 1)
    {
      state->cursor_row++;
      state->cursor_col = 0;
      set_cursor->exec(state->cursor_col, state->cursor_row,
                       {command_callback, this});
    }
    else
    {
      stage = Done;
      command_callback(this, 0);
    }
  }
  else if (current_char == '\r')
  {
    state->cursor_col = 0;
    set_cursor->exec(state->cursor_col, state->cursor_row,
                     {command_callback, this});
  }
  else
  {
    send->exec(static_cast<uint8_t>(current_char), LCD_DATA,
               {command_callback, this});
  }
}

void
Print::advance_position()
{
  if (current_char != '\n' && current_char != '\r')
    state->cursor_col++;

  print_next_char();
}

void
Print::command_callback(void *ctx, int result)
{
  auto cmd = static_cast<Print *>(ctx);

  if (result != 0)
  {
    cmd->cb(result);
    return;
  }

  switch (cmd->stage)
  {
    case Start:
      cmd->started();
      break;
    case SetCursorCmd:
      cmd->print_next_char();
      break;
    case PrintChar:
      cmd->advance_position();
      break;
    case Done:
      cmd->cb(0);
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
