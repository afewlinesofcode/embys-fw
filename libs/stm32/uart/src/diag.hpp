/**
 * @file diag.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief UART diagnostics and error codes
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace Embys::Stm32::Uart
{

enum Diag : int
{
  BASE_ERROR = -2000,
  INVALID_USART,
  BUS_NOT_ENABLED,
  TX_BUSY,
  RX_OVERFLOW,
  TX_TIMEOUT,
  BUFFER_TOO_SMALL,
};

}; // namespace Embys::Stm32::Uart
