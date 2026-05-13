#include "clock_init.h"

// Configure PLL1 to achieve 480 MHz (H743 max) from 8 MHz HSE.
// HSE / M(4) * N(240) / P(2) = 240 MHz HCLK (SysClk 480 / D1CPRE=1).
// USB: PLL3 or PLL1Q — left for application to set up.
//
// NOTE: H7 domain clocks:
//   SYS_D1CPRE = 480 MHz (CPU)
//   HCLK = SYS / HPRE(2) = 240 MHz
//   APB1 = HCLK / D2PPRE1(2) = 120 MHz
//   APB2 = HCLK / D2PPRE2(2) = 120 MHz
//   APB4 = HCLK / D3PPRE(2)  = 120 MHz
void
clock_init_480mhz(void)
{
  // Switch to HSI to safely configure PLL
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY)) {}
  RCC->CFGR = 0; // HSI as system clock
  while ((RCC->CFGR & RCC_CFGR_SWS) != 0) {} // wait SWS = HSI

  // Enable HSE
  RCC->CR |= RCC_CR_HSEON;
  while (!(RCC->CR & RCC_CR_HSERDY)) {}

  // Enable PWR and set voltage scale VOS0 (boost) for max frequency
  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
  RCC->APB1LENR |= RCC_APB1LENR_PWREN;
  PWR->CR3 |= PWR_CR3_SCUEN;
  PWR->D3CR = (PWR->D3CR & ~PWR_D3CR_VOS) | (0x3U << PWR_D3CR_VOS_Pos); // VOS1
  while (!(PWR->D3CR & PWR_D3CR_VOSRDY)) {}

  // Flash latency: 4 wait states for 240 MHz HCLK (VOS1), enable caches
  FLASH->ACR = (4U << FLASH_ACR_LATENCY_Pos) | FLASH_ACR_WRHIGHFREQ_1
             | FLASH_ACR_PRFTEN;

  // Domain clock dividers
  RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV1  // CPU: SYS/1 = 480 MHz
              | RCC_D1CFGR_D1PPRE_DIV2   // APB3: HCLK/2
              | RCC_D1CFGR_HPRE_DIV2;    // HCLK: SYS/2 = 240 MHz
  RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_DIV2 // APB1: HCLK/2 = 120 MHz
              | RCC_D2CFGR_D2PPRE2_DIV2; // APB2: HCLK/2 = 120 MHz
  RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2; // APB4: HCLK/2 = 120 MHz

  // PLL1: M=4, N=240, P=2 → 480 MHz VCO output / P(2) = 240 MHz → but
  // D1CPRE=1, so SYS = PLL1 P output = 480 MHz requires P=1.
  // Use P=1 (DIVP1EN): SYS = HSE/M * N / P = 8/4 * 480 / 2 = 480 MHz.
  RCC->PLL1DIVR = ((240U - 1U) << RCC_PLL1DIVR_N1_Pos) // N=240
                | ((2U - 1U) << RCC_PLL1DIVR_P1_Pos)   // P=2 → 240 MHz SYSCLK
                | ((2U - 1U) << RCC_PLL1DIVR_Q1_Pos)   // Q=2
                | ((2U - 1U) << RCC_PLL1DIVR_R1_Pos);  // R=2
  RCC->PLLCKSELR = (4U << RCC_PLLCKSELR_DIVM1_Pos)     // M=4
                 | RCC_PLLCKSELR_PLLSRC_HSE;
  RCC->PLLCFGR = RCC_PLLCFGR_PLL1RGE_2   // VCO input 4-8 MHz range
               | RCC_PLLCFGR_DIVP1EN;

  // Enable PLL1 and wait
  RCC->CR |= RCC_CR_PLL1ON;
  while (!(RCC->CR & RCC_CR_PLL1RDY)) {}

  // Switch to PLL1
  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) {}

  SystemCoreClockUpdate();
}
