#pragma once

#include "def/pin_cfg.hpp"
#include "def/pwm_binding.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::Gpio
{

/**
 * @brief GPIO diagnostics and error codes
 */
enum Diag : int
{
  BASE_ERROR = -1000,
  INVALID_PORT,
  PIN_CONFIG_FAILED,
  PIN_PULLUP_CONFIG_FAILED,
  PIN_PULLDOWN_CONFIG_FAILED,
  PIN_CNF_CONFIG_FAILED,
  EXTI_CONFIG_FAILED,
  PIN_CONFIG_CONFLICT,
  BUS_FULL,
  BUS_NOT_ENABLED
};

}; // namespace Embys::Stm32::Gpio
