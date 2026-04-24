#include "panic.h"

#include <stm32f1xx.h>

static void
init_pc13()
{
  if ((GPIOC->CRH & GPIO_CRH_MODE13) != 0)
  {
    // Already initialized, no need to reconfigure
    return;
  }

  // GPIOC clock
  RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

  // PC13
  // Clear MODE and CNF bits
  GPIOC->CRH &= ~(GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
  // MODE = 0b10 (Output mode, max speed 2 MHz), CNF = 0b00 (Push-Pull)
  GPIOC->CRH |= GPIO_CRH_MODE13_1;

  // Default OFF (inactive = high because LED is active-low)
  GPIOC->BSRR = GPIO_BSRR_BS13;
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
    for (uint32_t j = 0; j < 8000; ++j)
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
  init_pc13();

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
