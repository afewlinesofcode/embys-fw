/**
 * @file pulse_enable.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief HD44780 LCD enable pulse operation
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

class Write;

class PulseEnable
{
public:
  PulseEnable(Write *write, I2c::Dev::Delay *delay);

  inline Write *
  get_write()
  {
    return write;
  }

  void
  exec(uint8_t data, Cb cb);

private:
  Write *write;
  I2c::Dev::Delay *delay;
  uint8_t high = 0;
  uint8_t low = 0;
  Cb cb;

  enum Stage
  {
    Timeout1,
    WriteENHigh,
    Timeout2,
    WriteENLow,
    Timeout3,
  } stage = Timeout1;

  void
  timeout_us(uint32_t us, Stage st);

  void
  write_en_high();

  void
  write_en_low();

  static void
  command_callback(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::Hd44780
