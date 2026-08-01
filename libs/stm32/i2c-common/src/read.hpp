/**
 * @file read.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C device read operation helper
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>
#include <span>

#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev
{

class Read
{
public:
  explicit Read(I2c::BusCore *bus);

  void
  exec(uint8_t addr, uint8_t *buf, uint16_t len, Cb cb);

  void
  exec(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len, Cb cb);

private:
  I2c::BusCore *bus;
  Cb cb;
  uint8_t *destination = nullptr;
  uint16_t destination_len = 0;

  static void
  i2c_callback(void *ctx, int result, std::span<const uint8_t> data);
};

}; // namespace Embys::Stm32::I2c::Dev
