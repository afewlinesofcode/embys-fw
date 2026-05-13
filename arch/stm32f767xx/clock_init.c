#include "clock_init.h"

// Configure PLL to achieve 216 MHz (F767 max) from 8 MHz HSE.
// HSE / M(8) * N(432) / P(2) = 216 MHz. USB PLL: / Q(9) = 48 MHz.
void
clock_init_216mhz(void)
{
  // Enable HSE
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY)) {}

  // Enable PWR and set voltage scale 1 (required for 216 MHz)
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  PWR->CR1 = (PWR->CR1 & ~PWR_CR1_VOS) | PWR_CR1_VOS;

  // Flash: 7 wait states, ART cache + prefetch enabled
  FLASH->ACR = FLASH_ACR_ARTEN | FLASH_ACR_PRFTEN | FLASH_ACR_LATENCY_7WS;

  // AHB=1, APB1=/4 (54 MHz), APB2=/2 (108 MHz)
  RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;

  // PLL: M=8, N=432, P=2, Q=9
  RCC->PLLCFGR = (8U)
               | (432U << RCC_PLLCFGR_PLLN_Pos)
               | (0U << RCC_PLLCFGR_PLLP_Pos)
               | RCC_PLLCFGR_PLLSRC_HSE
               | (9U << RCC_PLLCFGR_PLLQ_Pos);
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY)) {}

  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

  SystemCoreClockUpdate();
}
