// F1 UART HAL implementation (classic SR/DR register layout).
// USART1 is on APB2; USART2/3 are on APB1.
// PPRE2 (APB2 prescaler) is RCC_CFGR bits [13:11] on F1.
// PPRE1 (APB1 prescaler) is RCC_CFGR bits [10:8]  on F1.
// Encoding: 0xx → /1, 100 → /2, 101 → /4, 110 → /8, 111 → /16.

#ifdef STM32F1xx

#include "../common/registers.hpp"

#include <embys/stm32/def.hpp>

#include "../../diag.hpp"

namespace Embys::Stm32::Uart
{

static uint32_t
ppre_div(uint32_t field3)
{
  return (field3 & 0x4U) ? (1U << ((field3 & 0x3U) + 1U)) : 1U;
}

static uint32_t
pclk1_hz()
{
  return SystemCoreClock / ppre_div((RCC->CFGR >> 8U) & 0x7U);
}

static uint32_t
pclk2_hz()
{
  return SystemCoreClock / ppre_div((RCC->CFGR >> 11U) & 0x7U);
}

static void
configure_cr(USART_TypeDef *usart, uint32_t baud_rate, WordLength word_length,
             StopBits stop_bits, Parity parity, uint32_t pclk)
{
  // Word length (CR1.M): 0 = 8-bit, 1 = 9-bit
  if (word_length == WordLength::W9)
    SET_BIT_V(usart->CR1, USART_CR1_M);
  else
    CLEAR_BIT_V(usart->CR1, USART_CR1_M);

  // Parity
  if (parity == Parity::None)
  {
    CLEAR_BIT_V(usart->CR1, USART_CR1_PCE);
  }
  else
  {
    SET_BIT_V(usart->CR1, USART_CR1_PCE);
    if (parity == Parity::Odd)
      SET_BIT_V(usart->CR1, USART_CR1_PS);
    else
      CLEAR_BIT_V(usart->CR1, USART_CR1_PS);
  }

  // Stop bits (CR2.STOP[13:12])
  usart->CR2 = (usart->CR2 & ~USART_CR2_STOP) |
               (static_cast<uint32_t>(stop_bits) << USART_CR2_STOP_Pos);

  // Enable TX + RX, then USART
  SET_BIT_V(usart->CR1, USART_CR1_TE | USART_CR1_RE);
  SET_BIT_V(usart->CR1, USART_CR1_UE);

  // BRR = PCLK / baud (16x oversampling, OVER8=0 after reset)
  usart->BRR = pclk / baud_rate;

  // Enable RXNE interrupt; TXE and TC are enabled on demand
  enable_rxne_irq(usart);
  disable_txe_irq(usart);
  disable_tc_irq(usart);
}

int
enable_uart(USART_TypeDef *usart, uint32_t baud_rate, WordLength word_length,
            StopBits stop_bits, Parity parity)
{
  uint32_t pclk;

  if (usart == USART1)
  {
    SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
    SET_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_USART1RST);
    CLEAR_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_USART1RST);
    pclk = pclk2_hz();
  }
  else if (usart == USART2)
  {
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_USART2EN);
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART2RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART2RST);
    pclk = pclk1_hz();
  }
  else if (usart == USART3)
  {
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_USART3EN);
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART3RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART3RST);
    pclk = pclk1_hz();
  }
  else
  {
    return INVALID_USART;
  }

  (void)RCC->APB2ENR; // read-back for bus completion
  __DSB();            // ensure clock is stable before accessing peripheral

  configure_cr(usart, baud_rate, word_length, stop_bits, parity, pclk);
  return 0;
}

int
disable_uart(USART_TypeDef *usart)
{
  disable_rxne_irq(usart);
  disable_txe_irq(usart);
  disable_tc_irq(usart);

  CLEAR_BIT_V(usart->CR1, USART_CR1_UE);

  if (usart == USART1)
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
  else if (usart == USART2)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_USART2EN);
  else if (usart == USART3)
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_USART3EN);
  else
    return INVALID_USART;

  return 0;
}

}; // namespace Embys::Stm32::Uart

#endif // STM32F1xx
