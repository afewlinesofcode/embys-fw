#include "clock_init.h"

// Configure PLL to achieve 100 MHz (F411 max) from 8 MHz HSE.
// HSE / M(4) * N(100) / P(2) = 100 MHz.  VCO = 200 MHz.
// USB PLL Q=4 gives 50 MHz (not exact 48 MHz; disable USB or use HSI48 if needed).
void
clock_init_100mhz(void)
{
  // Enable HSE and wait for it to be ready
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY)) {}

  // Enable power controller and set voltage scale 1 (required for 100 MHz)
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  PWR->CR |= PWR_CR_VOS;

  // Configure flash latency for 100 MHz (3 wait states) with cache & prefetch
  FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN
             | FLASH_ACR_LATENCY_3WS;

  // AHB/APB prescalers: AHB=1, APB1=/2 (50 MHz), APB2=/1 (100 MHz)
  RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

  // Configure and enable main PLL
  RCC->PLLCFGR = (4U)                      // M = 4 (HSE 8 MHz / 4 = 2 MHz VCO input)
               | (100U << RCC_PLLCFGR_PLLN_Pos) // N = 100 (VCO = 200 MHz)
               | (0U << RCC_PLLCFGR_PLLP_Pos)   // P = 2 (00 = /2 → 100 MHz)
               | RCC_PLLCFGR_PLLSRC_HSE
               | (4U << RCC_PLLCFGR_PLLQ_Pos);  // Q = 4 (OTG = 50 MHz)
  RCC->CR |= RCC_CR_PLLON;
  while (!(RCC->CR & RCC_CR_PLLRDY)) {}

  // Switch system clock to PLL
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

  SystemCoreClockUpdate();
}
