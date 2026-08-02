#include "util.hpp"

namespace Embys::Stm32::I2c::Dev::I2cAht20
{

static uint8_t
write_centi_1dp(char *buf, int32_t centi)
{
  uint8_t i = 0;
  int32_t deci = centi >= 0 ? (centi + 5) / 10 : -((-centi + 5) / 10);

  if (deci < 0)
  {
    buf[i++] = '-';
    deci = -deci;
  }

  uint32_t integer = static_cast<uint32_t>(deci) / 10U;
  uint32_t frac = static_cast<uint32_t>(deci) % 10U;

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
write_temperature(char *buf, Aht20::Temperature value)
{
  static const char prefix[] = "Temp: ";
  uint8_t i = 0;

  for (; prefix[i]; ++i)
    buf[i] = prefix[i];

  i += write_centi_1dp(&buf[i], value.centi_celsius);
  buf[i++] = ' ';
  buf[i++] = 'C';
  pad_to(buf, i, 20);
}

void
write_humidity(char *buf, Aht20::RelativeHumidity value)
{
  static const char prefix[] = "Hum:  ";
  uint8_t i = 0;

  for (; prefix[i]; ++i)
    buf[i] = prefix[i];

  i += write_centi_1dp(&buf[i], value.centi_percent);
  buf[i++] = ' ';
  buf[i++] = '%';
  pad_to(buf, i, 20);
}

}; // namespace Embys::Stm32::I2c::Dev::I2cAht20
