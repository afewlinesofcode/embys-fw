#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

namespace Sim = Embys::Stm32::Sim;

struct SimTimerFixture
{
  SimTimerFixture()
  {
    Sim::reset();
  }
};

TEST_SUITE("sim_timer")
{

  TEST_CASE_FIXTURE(SimTimerFixture, "Basic simulation with TIM2")
  {
    CHECK(DWT->CYCCNT == 0);

    // Configure TIM2 for a simple test
    // Enable timer, one-pulse mode
    // Set prescaler to match 1 us per tick (same as the Timer class)
    // Set auto-reload value to 5 microseconds
    TIM2->DIER = TIM_DIER_UIE;
    TIM2->PSC = Sim::cyc_per_us - 1;
    TIM2->CR1 = TIM_CR1_CEN | TIM_CR1_OPM;
    TIM2->ARR = 5;

    // Simulate waiting for interrupt
    __WFI();

    // Check that 5 microseconds have passed
    // Since core_clock is 72 MHz, 5 microseconds corresponds to 360 cycles
    CHECK(DWT->CYCCNT == 5 * Sim::core_clock / 1000000);

    // Check that update interrupt flag is set
    CHECK((TIM2->SR & TIM_SR_UIF) != 0);

    // Check that counter reset to 0 after overflow
    CHECK(TIM2->CNT == 0);

    // Check that timer is disabled in one-pulse mode
    CHECK((TIM2->CR1 & TIM_CR1_CEN) == 0);
  }

} // TEST_SUITE("sim_timer")
