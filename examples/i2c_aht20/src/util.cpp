#include "util.hpp"

namespace Embys::Stm32::I2c::Dev::I2cAht20
{

static uint8_t
write_float_1dp(char *buf, float v)
{
  uint8_t i = 0;

  if (v < 0.0f)
  {
    buf[i++] = '-';
    v = -v;
  }

  uint32_t integer = static_cast<uint32_t>(v);
  uint32_t frac =
      static_cast<uint32_t>((v - static_cast<float>(integer)) * 10.0f + 0.5f);

  if (frac >= 10u)
  {
    integer++;
    frac = 0u;
  }

  char digits[10];
  uint8_t dlen = 0;

  if (integer == 0u)
  {
    digits[dlen++] = '0';
  }
  else
  {
    uint32_t n = integer;
    while (n)
    {
      digits[dlen++] = static_cast<char>('0' + n % 10u);
      n /= 10u;
    }
  }

  for (int8_t j = static_cast<int8_t>(dlen) - 1; j >= 0; --j)
    buf[i++] = digits[j];

  buf[i++] = '.';
  buf[i++] = static_cast<char>('0' + frac);

  return i;
}

static void
pad_to(char *buf, uint8_t start, uint8_t total)
{
  for (uint8_t i = start; i < total; ++i)
    buf[i] = ' ';
  buf[total] = '\0';
}

void
write_temperature(char *buf, float v)
{
  static const char prefix[] = "Temp: ";
  uint8_t i = 0;

  for (; prefix[i]; ++i)
    buf[i] = prefix[i];

  i += write_float_1dp(&buf[i], v);
  buf[i++] = ' ';
  buf[i++] = 'C';
  pad_to(buf, i, 20);
}

void
write_humidity(char *buf, float v)
{
  static const char prefix[] = "Hum:  ";
  uint8_t i = 0;

  for (; prefix[i]; ++i)
    buf[i] = prefix[i];

  i += write_float_1dp(&buf[i], v);
  buf[i++] = ' ';
  buf[i++] = '%';
  pad_to(buf, i, 20);
}

}; // namespace Embys::Stm32::I2c::Dev::I2cAht20
