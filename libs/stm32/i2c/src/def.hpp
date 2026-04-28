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
