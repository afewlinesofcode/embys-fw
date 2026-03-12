#include <vector>

#include "stm32f1xx_sim.hpp"
#include "test.hpp"

namespace Sim = Embys::Stm32::Sim;

struct SimGpioFixture
{
  static int exti_interrupt_called_count;

  static void
  EXTI0_IRQHandler()
  {
    ++exti_interrupt_called_count;
  }

  SimGpioFixture()
  {
    exti_interrupt_called_count = 0;
    Sim::reset();
    Embys::Stm32::Sim::EXTI0_IRQHandler_ptr = EXTI0_IRQHandler;
  }
};

int SimGpioFixture::exti_interrupt_called_count = 0;

TEST_CASE_FIXTURE(SimGpioFixture, "Basic GPIO pin trigger simulation")
{
  // Enable GPIOA clock
  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPAEN);

  // Configure PA0 as input floating
  CLEAR_BIT_V(GPIOA->CRL, GPIO_CRL_MODE0 | GPIO_CRL_CNF0);
  SET_BIT_V(GPIOA->CRL, GPIO_CRL_CNF0_0);

  // Configure EXTI for PA0
  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_AFIOEN);
  CLEAR_BIT_V(AFIO->EXTICR[0], AFIO_EXTICR1_EXTI0); // Map EXTI0 to PA0
  SET_BIT_V(EXTI->IMR, EXTI_IMR_MR0);               // Unmask EXTI0
  SET_BIT_V(EXTI->RTSR, EXTI_RTSR_TR0);             // Trigger on rising edge
  SET_BIT_V(EXTI->FTSR, EXTI_FTSR_TR0);             // Trigger on falling edge

  // Simulate triggering the pin high
  // set
  Sim::Gpio::trigger_pin(GPIOA, 0, 1);
  CHECK(exti_interrupt_called_count == 0);

  for (int i = 0; i < 10; ++i) // should be enough to get IRQ handler called
    __NOP();

  CHECK(exti_interrupt_called_count == 1);

  exti_interrupt_called_count = 0;

  // Simulate triggering the pin low (should trigger again since we enabled both
  // edges)
  Sim::Gpio::trigger_pin(GPIOA, 0, 0);
  CHECK(exti_interrupt_called_count == 0);

  for (int i = 0; i < 10; ++i)
    __NOP();

  CHECK(exti_interrupt_called_count == 1);
}
