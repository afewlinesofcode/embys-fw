/**
 * @file diag.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus RTU diagnostics and error codes
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace Embys::Stm32::Modbus::Rtu
{

enum Diag : int
{
  BASE_ERROR = -6000,
  SEND_FAILED,
  SEND_IN_PROGRESS,
  FRAME_BUFFER_OVERFLOW,
  EXPECTING_RESPONSE,
  UNEXPECTED_QUANTITY,
  INVALID_BAUD_RATE,
};

}; // namespace Embys::Stm32::Modbus::Rtu
