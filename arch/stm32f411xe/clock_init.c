#include "clock_init.h"

#include <stm32f4xx.h>
#include "panic.h"

// Configure PLL to achieve 100 MHz (F411 max).
// Tries HSE (8 MHz) first: M=4, N=100, P=2 → VCO 200 MHz → 100 MHz.
// Falls back to HSI (16 MHz): M=8, N=100, P=2 → VCO 200 MHz → 100 MHz
// if HSE does not become ready within ~20 000 cycles.
void
clock_init_100mhz(void)
{
  // ── Voltage scale 1 (required for 100 MHz; reset default is scale 2) ─────
  RCC->APB1ENR |= RCC_APB1ENR_PWREN;
  (void)RCC->APB1ENR; // read-back ensures write commits before PWR access

  PWR->CR |= PWR_CR_VOS; // VOS[1:0] = 11 = scale 1

  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY)) {}

  // ── Try HSE ──────────────────────────────────────────────────────────────
  RCC->CR |= RCC_CR_HSEON;
  uint32_t timeout = 20000U;
  while (!(RCC->CR & RCC_CR_HSERDY) && --timeout) {}
  const char hse_ok = (timeout != 0U);

  if (!hse_ok)
    RCC->CR &= ~RCC_CR_HSEON; // give up on HSE

  // ── Flash latency for 100 MHz (set before switching clock) ───────────────
  FLASH->ACR = FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN
             | FLASH_ACR_LATENCY_3WS;

  while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_3WS) {}

  // ── AHB/APB prescalers: AHB=/1, APB1=/2 (50 MHz), APB2=/1 (100 MHz) ────
  RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;

  // ── PLL ──────────────────────────────────────────────────────────────────
  RCC->CR &= ~RCC_CR_PLLON;
  while (RCC->CR & RCC_CR_PLLRDY) {}

  if (hse_ok)
  {
    // HSE 25 MHz / M=25 → 1 MHz VCO input, ×N=200 → 200 MHz, /P=2 → 100 MHz
    RCC->PLLCFGR = (25U)
                 | (200U << RCC_PLLCFGR_PLLN_Pos)
                 | (0U   << RCC_PLLCFGR_PLLP_Pos)
                 | RCC_PLLCFGR_PLLSRC_HSE
                 | (4U   << RCC_PLLCFGR_PLLQ_Pos);
  }
  else
  {
    // HSI 16 MHz / M=8 → 2 MHz VCO input, ×N=100 → 200 MHz, /P=2 → 100 MHz
    RCC->PLLCFGR = (8U)
                 | (100U << RCC_PLLCFGR_PLLN_Pos)
                 | (0U   << RCC_PLLCFGR_PLLP_Pos)
                 | RCC_PLLCFGR_PLLSRC_HSI
                 | (4U   << RCC_PLLCFGR_PLLQ_Pos);
  }

  RCC->CR |= RCC_CR_PLLON;

  timeout = 20000U;
  while (!(RCC->CR & RCC_CR_PLLRDY) && --timeout)
  {
  }

  if (timeout == 0U)
    panic(-2);

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
  {
  }
}
