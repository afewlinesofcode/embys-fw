#include "api.hpp"

#include <embys/stm32/def.hpp>

#include "diag.hpp"

namespace Embys::Stm32::Uart
{

int
enable_uart(USART_TypeDef *usart, uint32_t baud_rate, WordLength word_length,
            StopBits stop_bits, Parity parity)
{
  // Enable peripheral clock and reset
  if (usart == USART1)
  {
    SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_USART1EN);
    SET_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_USART1RST);
    CLEAR_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_USART1RST);
  }
  else if (usart == USART2)
  {
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_USART2EN);
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART2RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART2RST);
  }
  else if (usart == USART3)
  {
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_USART3EN);
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART3RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_USART3RST);
  }
  else
  {
    return INVALID_USART;
  }

  (void)RCC->APB2ENR; // Read back for completion
  __DSB();            // Ensure clock stability

  // Word length (CR1.M): 0 = 8 data bits, 1 = 9 data bits
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

  // Enable TX + RX
  SET_BIT_V(usart->CR1, USART_CR1_TE | USART_CR1_RE);

  // Enable USART
  SET_BIT_V(usart->CR1, USART_CR1_UE);

  // Baud rate — USART1 is on APB2 (same as PCLK2), USART2/3 on APB1
  uint32_t pclk = (usart == USART1) ? SystemCoreClock : SystemCoreClock / 2;
  usart->BRR = pclk / baud_rate;

  // Enable only RXNE interrupt; TXE and TC are enabled on demand
  enable_rxne_irq(usart);
  disable_txe_irq(usart);
  disable_tc_irq(usart);

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

uint32_t
calc_frame_bits(WordLength word_length, StopBits stop_bits)
{
  uint32_t bits = 1; // Start bit
  bits += (word_length == WordLength::W9) ? 9u : 8u;
  bits += (stop_bits == StopBits::Two) ? 2u : 1u;
  return bits;
}

}; // namespace Embys::Stm32::Uart
