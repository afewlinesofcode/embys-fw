/**
 * @file def.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief UART configuration definitions and enumerations
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <cstdint>

#include <embys/stm32/result.hpp>

namespace Embys::Stm32::Uart
{

enum class Parity : uint32_t
{
  None = 0,
  Even = 1,
  Odd = 2,
};

enum class StopBits : uint32_t
{
  One = 0,
  Two = 2,
};

enum class WordLength : uint32_t
{
  W8 = 0,
  W9 = 1,
};

enum class Error : uint8_t
{
  InvalidInstance,
  InvalidBaudRate,
  NotEnabled,
  TransmitBusy,
  ReceiveOverflow,
  TransmitTimeout,
  BufferTooSmall,
  ModuleCapacity,
  Schedule,
};

using Status = Embys::Result<void, Error>;
using ReceiveResult = Embys::Result<uint8_t, Error>;

}; // namespace Embys::Stm32::Uart
