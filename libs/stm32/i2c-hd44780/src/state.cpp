#include "state.hpp"

namespace Embys::Stm32::I2c::Dev::Hd44780
{

void
State::advance_position()
{
  if (cursor_col < LCD_COLS - 1)
  {
    cursor_col++;
  }
  else if (cursor_row < LCD_ROWS - 1)
  {
    cursor_col = 0;
    cursor_row++;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
