/**
 * @file hal/v2/hal.hpp
 * @brief UART V2 register-level helpers (STM32F7 / STM32H7).
 *
 * V2 layout: ISR (status, read-only), RDR (read data), TDR (write data),
 * ICR (interrupt-clear flags).
 * Included by src/hal.hpp when UART_HAL_V2 is defined and by the family
 * HAL implementation files (hal/f7/hal.cpp, hal/h7/hal.cpp).
 */
#pragma once

#include "../../stm32xx.hpp"

#ifdef UART_HAL_V2

#include <stdint.h>

#include <embys/stm32/def.hpp>

#include "../../def.hpp"

namespace Embys::Stm32::Uart
{

// ── error mask ────────────────────────────────────────────────────────────
static constexpr uint32_t err_mask =
    USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE;

// ── flag constants (referenced by bus.cpp) ────────────────────────────────
// H7 CMSIS renames TXE to TXE_TXFNF in FIFO mode; provide a fallback.
static constexpr uint32_t SR_RXNE = USART_ISR_RXNE;
#ifdef USART_ISR_TXE_TXFNF
static constexpr uint32_t SR_TXE = USART_ISR_TXE_TXFNF;
#else
static constexpr uint32_t SR_TXE = USART_ISR_TXE;
#endif
static constexpr uint32_t SR_TC = USART_ISR_TC;

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
  return usart->ISR;
}

inline uint8_t
read_dr(USART_TypeDef *usart)
{
  return static_cast<uint8_t>(usart->RDR); // reading RDR clears RXNE
}

inline void
write_dr(USART_TypeDef *usart, uint8_t data)
{
  usart->TDR = static_cast<uint32_t>(data);
}

inline bool
is_rxne(USART_TypeDef *usart)
{
  return (usart->ISR & USART_ISR_RXNE) != 0;
}

inline bool
is_txe(USART_TypeDef *usart)
{
  return (usart->ISR & SR_TXE) != 0;
}

inline bool
is_tc(USART_TypeDef *usart)
{
  return (usart->ISR & USART_ISR_TC) != 0;
}

inline void
clear_tc(USART_TypeDef *usart)
{
  usart->ICR = USART_ICR_TCCF;
}

inline bool
has_error(USART_TypeDef *usart)
{
  return (usart->ISR & err_mask) != 0;
}

}; // namespace Embys::Stm32::Uart

#endif // UART_HAL_V2
