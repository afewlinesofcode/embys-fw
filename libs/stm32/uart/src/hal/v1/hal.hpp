/**
 * @file hal/v1/hal.hpp
 * @brief UART V1 register-level helpers (STM32F1 / STM32F4 / sim).
 *
 * V1 layout: SR (status), DR (read/write data).
 * Included by src/hal.hpp when UART_HAL_V1 is defined and by the family
 * HAL implementation files (hal/f1/hal.cpp, hal/f4/hal.cpp).
 */
#pragma once

#include "../../stm32xx.hpp"

#ifdef UART_HAL_V1

#include <stdint.h>

#include <embys/stm32/def.hpp>

#include "../../def.hpp"

namespace Embys::Stm32::Uart
{

// ── error mask ────────────────────────────────────────────────────────────
static constexpr uint32_t err_mask =
    USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE;

// ── flag constants (referenced by bus.cpp) ────────────────────────────────
static constexpr uint32_t SR_RXNE = USART_SR_RXNE;
static constexpr uint32_t SR_TXE = USART_SR_TXE;
static constexpr uint32_t SR_TC = USART_SR_TC;

// ── interrupt-enable helpers ──────────────────────────────────────────────

inline void
enable_rxne_irq(USART_TypeDef *usart)
{
  SET_BIT_V(usart->CR1, USART_CR1_RXNEIE);
}

inline void
disable_rxne_irq(USART_TypeDef *usart)
{
  CLEAR_BIT_V(usart->CR1, USART_CR1_RXNEIE);
}

inline void
enable_txe_irq(USART_TypeDef *usart)
{
  SET_BIT_V(usart->CR1, USART_CR1_TXEIE);
}

inline void
disable_txe_irq(USART_TypeDef *usart)
{
  CLEAR_BIT_V(usart->CR1, USART_CR1_TXEIE);
}

inline void
enable_tc_irq(USART_TypeDef *usart)
{
  SET_BIT_V(usart->CR1, USART_CR1_TCIE);
}

inline void
disable_tc_irq(USART_TypeDef *usart)
{
  CLEAR_BIT_V(usart->CR1, USART_CR1_TCIE);
}

// ── register accessors ────────────────────────────────────────────────────

inline uint32_t
read_sr(USART_TypeDef *usart)
{
  uint32_t val = usart->SR;
#ifdef STM32_SIM
  ::Embys::Stm32::Sim::Base::trigger_test_hook("uart_read_sr");
#endif
  return val;
}

inline uint8_t
read_dr(USART_TypeDef *usart)
{
  uint8_t val = static_cast<uint8_t>(usart->DR);
#ifdef STM32_SIM
  ::Embys::Stm32::Sim::Base::trigger_test_hook("uart_read_dr");
#endif
  return val;
}

inline void
write_dr(USART_TypeDef *usart, uint8_t data)
{
  usart->DR = static_cast<uint32_t>(data);
#ifdef STM32_SIM
  ::Embys::Stm32::Sim::Base::trigger_test_hook("uart_write_dr");
#endif
}

inline bool
is_rxne(USART_TypeDef *usart)
{
  return (usart->SR & USART_SR_RXNE) != 0;
}

inline bool
is_txe(USART_TypeDef *usart)
{
  return (usart->SR & USART_SR_TXE) != 0;
}

inline bool
is_tc(USART_TypeDef *usart)
{
  return (usart->SR & USART_SR_TC) != 0;
}

inline void
clear_tc(USART_TypeDef *usart)
{
  CLEAR_BIT_V(usart->SR, USART_SR_TC);
}

inline bool
has_error(USART_TypeDef *usart)
{
  return (usart->SR & err_mask) != 0;
}

}; // namespace Embys::Stm32::Uart

#endif // UART_HAL_V1
