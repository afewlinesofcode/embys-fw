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

#include <stdint.h>

namespace Embys::Stm32::I2c
{

enum Diag : int
{
  BASE_ERROR = -3000,
  INVALID_I2C,
  BUS_NOT_ENABLED,
  BUSY,
  INVALID_STATE,
  INVALID_BUFFER,
  INVALID_PCLK,
  INVALID_CCR,
  NACK,
  BUS_ERROR,
  ARBITRATION_LOST,
  OVERRUN,
  TIMEOUT,
  BUS_BUSY,
  BUS_STUCK,
  STOP_STUCK,
};

}; // namespace Embys::Stm32::I2c
