#include "panic.h"

#include <stm32f4xx.h>

uint32_t cycles_per_ms = 8000; // some default value for now

static void
init()
{
  SystemCoreClockUpdate();

  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
  (void)RCC->AHB1ENR; // read-back ensures write commits before GPIO access
  GPIOC->MODER = (GPIOC->MODER & ~(0x3U << (13 << 1))) | (0x1U << (13 << 1));

  // Default OFF (inactive = high because LED is active-low)
  GPIOC->BSRR = GPIO_BSRR_BS13;

  // Calculate cycles_per_ms
  cycles_per_ms = SystemCoreClock / 1000 / 20;
}

static inline void
on()
{
  GPIOC->BSRR = GPIO_BSRR_BR13;
}

static inline void
off()
{
  GPIOC->BSRR = GPIO_BSRR_BS13;
}

static void
wait_ms(uint32_t ms)
{
  for (uint32_t i = 0; i < ms; ++i)
  {
    for (uint32_t j = 0; j < cycles_per_ms; ++j)
    {
      __asm volatile("nop");
    }
  }
}

static void
blink_digit(int digit)
{
  if (digit == 0)
  {
    on();
    wait_ms(120);
    off();
    wait_ms(120);
    on();
    wait_ms(120);
    off();
    wait_ms(120);
  }

  for (int i = 0; i < digit; ++i)
  {
    on();
    wait_ms(500);
    off();
    wait_ms(600);
  }
}

static void
do_panic(int times)
{
  for (int i = 0; i < times; ++i)
  {
    on();
    wait_ms(120);
    off();
    wait_ms(120);
  }
}

void
panic(int code)
{
  init();

  __disable_irq();

  // panic mode
  if (code == 0)
  {
    // Fast blink forever
    while (1)
    {
      on();
      wait_ms(120);
      off();
      wait_ms(120);
    }
  }

  uint8_t digits[16];
  int digit_count = 0;

  int abs_code = code < 0 ? -code : code;

  while (abs_code != 0)
  {
    digits[digit_count++] = abs_code % 10;
    abs_code /= 10;
  }

  // error code mode
  if (code < 0)
  {
    while (1)
    {
      // Indicate error with 2 sets of 3 blinks, then blink digits of the code
      do_panic(3);
      wait_ms(500);
      do_panic(3);
      wait_ms(2000);

      for (int i = digit_count - 1; i >= 0; --i)
      {
        blink_digit(digits[i]);
        wait_ms(2000);
      }

      wait_ms(2000);
    }
  }

  // info code mode
  while (1)
  {
    // Indicate info with 1 set of 6 blinks, then blink digits of the code
    do_panic(6);
    wait_ms(2000);

    for (int i = digit_count - 1; i >= 0; --i)
    {
      blink_digit(digits[i]);
      wait_ms(2000);
    }

    wait_ms(2000);
  }
}
