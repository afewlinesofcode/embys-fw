/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C definitions and error codes
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <cstdint>
#include <span>

#include <embys/stm32/result.hpp>

namespace Embys::Stm32::I2c
{

enum class Error : uint8_t
{
  InvalidInstance,
  InvalidClock,
  InvalidTiming,
  NotEnabled,
  Busy,
  InvalidState,
  InvalidBuffer,
  Nack,
  BusError,
  ArbitrationLost,
  Overrun,
  Timeout,
  BusBusy,
  BusStuck,
  StopStuck,
  BufferTooSmall,
  ModuleCapacity,
  Schedule,
};

using Status = Embys::Result<void, Error>;
using ReadResult = Embys::Result<std::span<const uint8_t>, Error>;

}; // namespace Embys::Stm32::I2c
