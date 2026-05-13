#include "panic.h"

#include <stm32h7xx.h>

static void
blink_pb0(void)
{
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN;
  GPIOB->MODER = (GPIOB->MODER & ~(0x3U << (0 * 2))) | (0x1U << (0 * 2));
  while (1)
  {
    GPIOB->BSRR = GPIO_BSRR_BS0;
    for (volatile int i = 0; i < 800000; i++) {}
    GPIOB->BSRR = GPIO_BSRR_BR0;
    for (volatile int i = 0; i < 800000; i++) {}
  }
}

void
panic(int code)
{
  (void)code;
  __disable_irq();
  blink_pb0();
}
