#include "panic.h"

#include <stm32f4xx.h>

static void
blink_pa5(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  GPIOA->MODER = (GPIOA->MODER & ~(0x3U << (5 * 2))) | (0x1U << (5 * 2));
  while (1)
  {
    GPIOA->BSRR = GPIO_BSRR_BS5;
    for (volatile int i = 0; i < 400000; i++) {}
    GPIOA->BSRR = GPIO_BSRR_BR5;
    for (volatile int i = 0; i < 400000; i++) {}
  }
}

void
panic(int code)
{
  (void)code;
  __disable_irq();
  blink_pa5();
}
