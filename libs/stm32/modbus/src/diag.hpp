/**
 * @file diag.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus protocol diagnostics and error codes
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace Embys::Stm32::Modbus
{

enum Diag : int
{
  BASE_ERROR = -5000,
  COIL_OUT_OF_RANGE,
  REGISTER_OUT_OF_RANGE,
  BUFFER_TOO_SMALL,
  INVALID_BUFFER_SIZE,
};

}; // namespace Embys::Stm32::Modbus
