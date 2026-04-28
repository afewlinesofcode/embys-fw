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
};

}; // namespace Embys::Stm32::Uart
