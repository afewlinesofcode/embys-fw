#include "clock_init.h"

#include <stm32h7xx.h>
#include "panic.h"

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
  uint32_t timeout;

  // Enable HSI and switch SYSCLK to HSI before touching PLL1
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_HSI;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}

  // Disable PLL1 before reconfiguration
  RCC->CR &= ~RCC_CR_PLL1ON;
  while ((RCC->CR & RCC_CR_PLL1RDY) != 0) {}

  // Enable HSE
  RCC->CR |= RCC_CR_HSEON;

  timeout = 1000000U;
  while ((RCC->CR & RCC_CR_HSERDY) == 0 && --timeout) {}

  if (timeout == 0U)
    panic(-2);

  // Enable PWR clock
  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;
  (void)RCC->APB4ENR;

  /*
    Supply config depends on your board.

    For many STM32H743 boards using internal LDO:
      PWR->CR3 = (PWR->CR3 & ~PWR_CR3_SCUEN) | PWR_CR3_LDOEN;

    Do NOT blindly set SCUEN unless your board really uses SMPS/external supply.
  */
  PWR->CR3 = (PWR->CR3 & ~PWR_CR3_SCUEN) | PWR_CR3_LDOEN;

  // First set VOS1
  PWR->D3CR = (PWR->D3CR & ~PWR_D3CR_VOS)
            | (0x3U << PWR_D3CR_VOS_Pos);

  while ((PWR->D3CR & PWR_D3CR_VOSRDY) == 0) {}

  /*
    Enable VOS0 / overdrive boost for 480 MHz.

    On STM32H743 this is done through SYSCFG_PWRCR ODEN.
    Some CMSIS headers define SYSCFG_PWRCR_ODEN, some do not.
  */
#ifdef SYSCFG_PWRCR_ODEN
  SYSCFG->PWRCR |= SYSCFG_PWRCR_ODEN;
#else
  SYSCFG->PWRCR |= 0x00000001U;
#endif

  while ((PWR->D3CR & PWR_D3CR_VOSRDY) == 0) {}

  /*
    Flash latency.

    For HCLK = 240 MHz at high performance voltage scale,
    use conservative latency. Many ST examples use LATENCY = 4
    with WRHIGHFREQ = 2 for this class of config.

    If your header provides FLASH_ACR_WRHIGHFREQ_1 only, check the bit values.
  */
  FLASH->ACR = (4U << FLASH_ACR_LATENCY_Pos)
             | (2U << FLASH_ACR_WRHIGHFREQ_Pos);

  while ((FLASH->ACR & FLASH_ACR_LATENCY)
         != (4U << FLASH_ACR_LATENCY_Pos)) {}

  /*
    Domain dividers:
      SYSCLK = 480 MHz
      CPU clock = 480 MHz
      HCLK = 240 MHz
      APB1 = 120 MHz
      APB2 = 120 MHz
      APB3 = 120 MHz
      APB4 = 120 MHz
  */
  RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV1
              | RCC_D1CFGR_HPRE_DIV2
              | RCC_D1CFGR_D1PPRE_DIV2;

  RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_DIV2
              | RCC_D2CFGR_D2PPRE2_DIV2;

  RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2;

  /*
    PLL1:
      HSE = 8 MHz
      M = 4  => PLL input = 2 MHz
      N = 480 => VCO = 960 MHz
      P = 2 => PLL1_P = 480 MHz SYSCLK
      Q = 4 => 240 MHz
      R = 4 => 240 MHz
  */

  RCC->PLLCKSELR = (4U << RCC_PLLCKSELR_DIVM1_Pos)
                 | RCC_PLLCKSELR_PLLSRC_HSE;

  RCC->PLL1DIVR = ((480U - 1U) << RCC_PLL1DIVR_N1_Pos)
                | ((2U   - 1U) << RCC_PLL1DIVR_P1_Pos)
                | ((4U   - 1U) << RCC_PLL1DIVR_Q1_Pos)
                | ((4U   - 1U) << RCC_PLL1DIVR_R1_Pos);

  RCC->PLLCFGR = RCC_PLLCFGR_PLL1RGE_1     // PLL input 2-4 MHz
               | RCC_PLLCFGR_PLL1VCOSEL    // wide VCO range
               | RCC_PLLCFGR_DIVP1EN;

  RCC->CR |= RCC_CR_PLL1ON;

  timeout = 1000000U;
  while ((RCC->CR & RCC_CR_PLL1RDY) == 0 && --timeout) {}

  if (timeout == 0U)
    panic(-3);

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;

  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) {}
}
