#pragma once

#include <stdint.h>

#include <embys/stm32/def.hpp>

#include "def.hpp"
#include "stm32f1xx.hpp"

namespace Embys::Stm32::Uart
{

/**
 * @brief Error flag mask covering all USART receive error bits.
 */
static constexpr uint32_t err_mask =
    USART_SR_PE | USART_SR_FE | USART_SR_NE | USART_SR_ORE;

/**
 * @brief Enable USART clock, reset peripheral, and configure CR1/CR2/BRR.
 * Enables RXNE interrupt; TXE and TC interrupts are disabled.
 * @return 0 on success, negative error code on failure.
 */
int
enable_uart(USART_TypeDef *usart, uint32_t baud_rate, WordLength word_length,
            StopBits stop_bits, Parity parity);

/**
 * @brief Disable all USART interrupts, clear UE, disable peripheral clock.
 * @return 0 on success, negative error code on failure.
 */
int
disable_uart(USART_TypeDef *usart);

/**
 * @brief Return total frame bits for a given configuration.
 * Includes start bit, data bits, and stop bits. Parity is encoded inside
 * the data word on STM32F1 (uses one of the data bits), so it is not
 * counted separately.
 */
uint32_t
calc_frame_bits(WordLength word_length, StopBits stop_bits);

// ── interrupt-enable helpers ─────────────────────────────────────────────

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

// ── status register helpers ──────────────────────────────────────────────

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
