#include "clock_init.h"

// Configure PLL to achieve 168 MHz (F407 max) from 8 MHz HSE.
// HSE / M(8) * N(336) / P(2) = 168 MHz. USB PLL: / Q(7) = 48 MHz.
void
clock_init_168mhz(void)
{
  // Enable HSE and wait for it to be ready
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY)) {}

  // Enable power controller and set voltage scale 1 (needed for 168 MHz)
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  PWR->CR |= PWR_CR_VOS;

  // Configure flash latency for 168 MHz (5 wait states) with cache & prefetch
  FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN
             | FLASH_ACR_LATENCY_5WS;

  // AHB/APB prescalers: AHB=1, APB1=/4 (42 MHz), APB2=/2 (84 MHz)
  RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;

  // Configure and enable main PLL
  RCC->PLLCFGR = (8U)                      // M = 8 (HSE 8 MHz / 8 = 1 MHz VCO input)
               | (336U << RCC_PLLCFGR_PLLN_Pos) // N = 336
               | (0U << RCC_PLLCFGR_PLLP_Pos)   // P = 2 (00 = /2)
               | RCC_PLLCFGR_PLLSRC_HSE
               | (7U << RCC_PLLCFGR_PLLQ_Pos);  // Q = 7 (USB 48 MHz)
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY)) {}

  // Switch system clock to PLL
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

  SystemCoreClockUpdate();
}
