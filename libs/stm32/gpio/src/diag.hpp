/**
 * @file diag.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief GPIO diagnostics and error codes
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace Embys::Stm32::Gpio
{

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

};
