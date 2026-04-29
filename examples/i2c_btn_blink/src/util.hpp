#pragma once

#include <stdint.h>

namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
{

static void
write_counter(char *buf, uint32_t n)
{
  static const char prefix[] = "Blink counter: ";
  uint8_t i = 0;
  for (; prefix[i]; ++i)
    buf[i] = prefix[i];

  char digits[11];
  uint8_t dlen = 0;
  if (n == 0)
  {
    digits[dlen++] = '0';
  }
  else
  {
    while (n)
    {
      digits[dlen++] = static_cast<char>('0' + n % 10);
      n /= 10;
    }
  }
  for (int8_t j = static_cast<int8_t>(dlen) - 1; j >= 0; --j)
    buf[i++] = digits[j];
  while (i < 20)
    buf[i++] = ' ';
  buf[i] = '\0';
}

}; // namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
