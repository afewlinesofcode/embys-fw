// H7 UART HAL implementation (UART V2 peripheral, ISR/RDR/TDR/ICR layout).
// USART1/6 are on APB2; USART2/3/4/5 are on APB1L.
// PCLK1 (APB1L) is derived from: SystemCoreClock → D1CFGR HPRE[3:0] → D2CFGR
// D2PPRE1[6:4]. PCLK2 (APB2) is derived from:  SystemCoreClock → D1CFGR
// HPRE[3:0] → D2CFGR D2PPRE2[10:8]. AHB encoding: 0xxx→/1, 1000→/2, 1001→/4,
// 1010→/8, 1011→/16,
//               1100→/64, 1101→/128, 1110→/256, 1111→/512.
// APB encoding: 0xx→/1, 100→/2, 101→/4, 110→/8, 111→/16.

#ifdef STM32H7xx

#include "../v2/hal.hpp"

#include <embys/stm32/def.hpp>

#include "../../diag.hpp"

namespace Embys::Stm32::Uart
{

static uint32_t
ahb_div_from_hpre(uint32_t hpre)
{
  static const uint32_t table[16] = {1U, 1U, 1U, 1U,  1U,  1U,   1U,   1U,
                                     2U, 4U, 8U, 16U, 64U, 128U, 256U, 512U};
  return table[hpre & 0xFU];
}

static uint32_t
apb_div_from_ppre(uint32_t ppre)
{
  return (ppre & 0x4U) ? (1U << ((ppre & 0x3U) + 1U)) : 1U;
}

static uint32_t
ahb_hz()
{
  return SystemCoreClock / ahb_div_from_hpre((RCC->D1CFGR >> 0U) & 0xFU);
}

static uint32_t
pclk1_hz()
{
  return ahb_hz() / apb_div_from_ppre((RCC->D2CFGR >> 4U) & 0x7U);
}

static uint32_t
pclk2_hz()
{
  return ahb_hz() / apb_div_from_ppre((RCC->D2CFGR >> 8U) & 0x7U);
}

static void
configure_cr(USART_TypeDef *usart, uint32_t baud_rate, WordLength word_length,
             StopBits stop_bits, Parity parity, uint32_t pclk)
{
  // Word length (CR1.M0): 0 = 8-bit, 1 = 9-bit
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

  // Enable TX + RX, then USART (UE must be set before BRR write on V2)
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
    SET_BIT_V(RCC->APB1LENR, RCC_APB1LENR_USART2EN);
    SET_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_USART2RST);
    CLEAR_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_USART2RST);
    pclk = pclk1_hz();
  }
  else if (usart == USART3)
  {
    SET_BIT_V(RCC->APB1LENR, RCC_APB1LENR_USART3EN);
    SET_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_USART3RST);
    CLEAR_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_USART3RST);
    pclk = pclk1_hz();
  }
  else if (usart == UART4)
  {
    SET_BIT_V(RCC->APB1LENR, RCC_APB1LENR_UART4EN);
    SET_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_UART4RST);
    CLEAR_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_UART4RST);
    pclk = pclk1_hz();
  }
  else if (usart == UART5)
  {
    SET_BIT_V(RCC->APB1LENR, RCC_APB1LENR_UART5EN);
    SET_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_UART5RST);
    CLEAR_BIT_V(RCC->APB1LRSTR, RCC_APB1LRSTR_UART5RST);
    pclk = pclk1_hz();
  }
  else if (usart == USART6)
  {
    SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_USART6EN);
    SET_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_USART6RST);
    CLEAR_BIT_V(RCC->APB2RSTR, RCC_APB2RSTR_USART6RST);
    pclk = pclk2_hz();
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
    CLEAR_BIT_V(RCC->APB1LENR, RCC_APB1LENR_USART2EN);
  else if (usart == USART3)
    CLEAR_BIT_V(RCC->APB1LENR, RCC_APB1LENR_USART3EN);
  else if (usart == UART4)
    CLEAR_BIT_V(RCC->APB1LENR, RCC_APB1LENR_UART4EN);
  else if (usart == UART5)
    CLEAR_BIT_V(RCC->APB1LENR, RCC_APB1LENR_UART5EN);
  else if (usart == USART6)
    CLEAR_BIT_V(RCC->APB2ENR, RCC_APB2ENR_USART6EN);
  else
    return INVALID_USART;

  return 0;
}

}; // namespace Embys::Stm32::Uart

#endif // STM32H7xx
