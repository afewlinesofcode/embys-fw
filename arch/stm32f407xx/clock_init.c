#include "clock_init.h"

#include <stm32f4xx.h>
#include "panic.h"

// Configure PLL to achieve 168 MHz (F407 max).
// Tries HSE (8 MHz): M=8, N=336, P=2 → 168 MHz, Q=7 → 48 MHz USB.
// Falls back to HSI (16 MHz): M=8, N=168, P=2 → 168 MHz, Q=7 → 48 MHz.
void
clock_init_168mhz(void)
{
  // ── Voltage scale 1 (required for 168 MHz; reset default is scale 2) ─────
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  (void)RCC->APB1ENR; // read-back ensures write commits before PWR access
  PWR->CR |= PWR_CR_VOS; // VOS[1:0] = 11 = scale 1

  // ── Ensure HSI is running (PLL source fallback) ──────────────────────
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY)) {}

  // ── Try HSE ────────────────────────────────────────────
  RCC->CR |= RCC_CR_HSEON;
  uint32_t timeout = 20000U;
  while (!(RCC->CR & RCC_CR_HSERDY) && --timeout) {}
  const char hse_ok = (timeout != 0U);

  if (!hse_ok)
    RCC->CR &= ~RCC_CR_HSEON;

  // ── Flash latency for 168 MHz (5 wait states) ────────────────────
  FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN
             | FLASH_ACR_LATENCY_5WS;
  while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_5WS) {}

  // ── AHB/APB prescalers: AHB=/1, APB1=/4 (42 MHz), APB2=/2 (84 MHz) ────
  RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;

  // ── PLL ────────────────────────────────────────────────
  RCC->CR &= ~RCC_CR_PLLON;
  while (RCC->CR & RCC_CR_PLLRDY) {}

  if (hse_ok)
  {
    // HSE 8 MHz / M=8 → 1 MHz VCO input, ×N=336 → 336 MHz, /P=2 → 168 MHz
    RCC->PLLCFGR = (8U)
                 | (336U << RCC_PLLCFGR_PLLN_Pos)
                 | (0U << RCC_PLLCFGR_PLLP_Pos)
                 | RCC_PLLCFGR_PLLSRC_HSE
                 | (7U << RCC_PLLCFGR_PLLQ_Pos);
  }
  else
  {
    // HSI 16 MHz / M=8 → 2 MHz VCO input, ×N=168 → 336 MHz, /P=2 → 168 MHz
    RCC->PLLCFGR = (8U)
                 | (168U << RCC_PLLCFGR_PLLN_Pos)
                 | (0U << RCC_PLLCFGR_PLLP_Pos)
                 | RCC_PLLCFGR_PLLSRC_HSI
                 | (7U << RCC_PLLCFGR_PLLQ_Pos);
  }

  RCC->CR |= RCC_CR_PLLON;
  timeout = 20000U;
  while (!(RCC->CR & RCC_CR_PLLRDY) && --timeout) {}

  if (timeout == 0U)
    panic(-2);

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}
