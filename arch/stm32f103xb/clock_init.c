#include "clock_init.h"

void
clock_init_72mhz(void)
{
  // Enable HSE (assuming 8 MHz crystal)
  RCC->CR |= RCC_CR_HSEON;
  while ((RCC->CR & RCC_CR_HSERDY) == 0) {}

  // Configure Flash for 72 MHz
  FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

  // Configure ADC prescaler
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6; // or DIV8 depending

  // Configure PLL: source = HSE, mul = 9 → 8 MHz * 9 = 72 MHz
  RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
  RCC->CFGR |=  (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL9);

  // Configure prescalers
  // AHB = /1 (72 MHz), APB1 = /2 (36 MHz), APB2 = /1 (72 MHz)
  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
  RCC->CFGR |=  RCC_CFGR_PPRE1_DIV2;

  // Enable PLL
  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0) {}

  // Switch SYSCLK to PLL
  RCC->CFGR &= ~RCC_CFGR_SW;
  RCC->CFGR |=  RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}

void
clock_init_64mhz_hsi(void)
{
  // Use internal HSI oscillator (8 MHz). PLL input is HSI/2 (4 MHz).
  // Max SYSCLK without HSE on STM32F1 is 64 MHz (4 MHz * 16).

  // Ensure HSI is enabled
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}

  // Configure Flash for 64 MHz
  FLASH->ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2;

  // Configure ADC prescaler
  RCC->CFGR &= ~RCC_CFGR_ADCPRE;
  RCC->CFGR |= RCC_CFGR_ADCPRE_DIV6;

  // Configure prescalers
  // AHB = /1 (64 MHz), APB1 = /2 (32 MHz), APB2 = /1 (64 MHz)
  RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
  RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;

  // Configure PLL: source = HSI/2, mul = 16 → 4 MHz * 16 = 64 MHz
  RCC->CFGR &= ~(RCC_CFGR_PLLSRC | RCC_CFGR_PLLXTPRE | RCC_CFGR_PLLMULL);
  RCC->CFGR |= RCC_CFGR_PLLMULL16;

  // Enable PLL
  RCC->CR |= RCC_CR_PLLON;
  while ((RCC->CR & RCC_CR_PLLRDY) == 0) {}

  // Switch SYSCLK to PLL
  RCC->CFGR &= ~RCC_CFGR_SW;
  RCC->CFGR |= RCC_CFGR_SW_PLL;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
}
