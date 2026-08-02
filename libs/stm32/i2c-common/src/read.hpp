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

#include <cstdint>
#include <span>

#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"

namespace Embys::Stm32::I2c::Dev
{

class Read
{
public:
  explicit Read(I2c::BusCore &bus);

  void
  exec(uint8_t addr, std::span<uint8_t> destination, Cb cb);

  void
  exec(uint8_t addr, uint8_t reg, std::span<uint8_t> destination, Cb cb);

private:
  I2c::BusCore &bus;
  Cb cb;
  std::span<uint8_t> destination;

  static void
  i2c_callback(void *ctx, I2c::ReadResult result) noexcept;
};

}; // namespace Embys::Stm32::I2c::Dev
