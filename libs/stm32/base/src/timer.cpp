#include "timer.hpp"

#include <embys/stm32/def.hpp>
#include <embys/stm32/mcu_traits.hpp>

namespace Embys::Stm32::Base
{

// Helper: pointer to the APB1 enable/reset registers for the timer clock.
// H7 splits APB1 into APB1LENR (low) and APB1HENR (high); TIM2-7 are in LENR.
// All other supported families use APB1ENR.
static volatile uint32_t *
apb1enr()
{
#if defined(STM32H7xx)
  return &RCC->APB1LENR;
#else
  return &RCC->APB1ENR;
#endif
}

static volatile uint32_t *
apb1rstr()
{
#if defined(STM32H7xx)
  return &RCC->APB1LRSTR;
#else
  return &RCC->APB1RSTR;
#endif
}

Timer::Timer(TIM_TypeDef *timer) : timer(timer)
{
  init_peripheral();

  timer->CR1 = 0;  // Disable
  timer->CR2 = 0;  // Default configuration
  timer->SMCR = 0; // No slave mode
  timer->CNT = 0;  // Reset counter
  timer->SR = 0;   // Clear status register

  // Configure prescaler: 72 MHz / 72 = 1 MHz => 1 tick = 1 μs
  cyc_per_us = SystemCoreClock / 1000000;
  timer->PSC = static_cast<uint16_t>(cyc_per_us) - 1;
  timer->EGR = TIM_EGR_UG; // load PSC immediately
  timer->SR = 0;           // clear UIF set by UG

  SET_BIT_V(timer->DIER, TIM_DIER_UIE); // Enable update interrupt
}

Timer::~Timer()
{
  deinit_peripheral();
}

void
Timer::set_callback(Callable<> cb)
{
  this->cb = cb;
}

void
Timer::schedule_us(uint32_t us, uint32_t jitter_us, bool start)
{
  timer->CR1 = 0;

  if (us == UINT32_MAX)
    return; // No events pending - stop timer for power saving

  // Start timer with range-limited timeout value
  // Set auto-reload value with range limit
  set_scheduled_us(us);
  timer->CNT = jitter_us;

  if (start)
    timer->CR1 = TIM_CR1_OPM | TIM_CR1_CEN; // One-pulse mode + enable
}

void
Timer::restart()
{
  timer->CNT = 0;
  timer->CR1 = TIM_CR1_OPM | TIM_CR1_CEN;
}

void
Timer::reset()
{
  timer->CNT = 0;
}

// H7 uses APB1LENR/APB1LRSTR for TIM2-7; all other families use
// APB1ENR/APB1RSTR. The bit positions are the same, so we alias the constants.
#if defined(STM32H7xx)
#define TIM_APB1ENR_TIM2EN RCC_APB1LENR_TIM2EN
#define TIM_APB1ENR_TIM3EN RCC_APB1LENR_TIM3EN
#define TIM_APB1ENR_TIM4EN RCC_APB1LENR_TIM4EN
#define TIM_APB1RSTR_TIM2RST RCC_APB1LRSTR_TIM2RST
#define TIM_APB1RSTR_TIM3RST RCC_APB1LRSTR_TIM3RST
#define TIM_APB1RSTR_TIM4RST RCC_APB1LRSTR_TIM4RST
#else
#define TIM_APB1ENR_TIM2EN RCC_APB1ENR_TIM2EN
#define TIM_APB1ENR_TIM3EN RCC_APB1ENR_TIM3EN
#define TIM_APB1ENR_TIM4EN RCC_APB1ENR_TIM4EN
#define TIM_APB1RSTR_TIM2RST RCC_APB1RSTR_TIM2RST
#define TIM_APB1RSTR_TIM3RST RCC_APB1RSTR_TIM3RST
#define TIM_APB1RSTR_TIM4RST RCC_APB1RSTR_TIM4RST
#endif

void
Timer::init_peripheral()
{
  volatile uint32_t *enr = apb1enr();
  volatile uint32_t *rstr = apb1rstr();

  uint32_t en_mask = 0;
  uint32_t rst_mask = 0;

  if (timer == TIM2)
  {
    en_mask = TIM_APB1ENR_TIM2EN;
    rst_mask = TIM_APB1RSTR_TIM2RST;
  }
  else if (timer == TIM3)
  {
    en_mask = TIM_APB1ENR_TIM3EN;
    rst_mask = TIM_APB1RSTR_TIM3RST;
  }
  else if (timer == TIM4)
  {
    en_mask = TIM_APB1ENR_TIM4EN;
    rst_mask = TIM_APB1RSTR_TIM4RST;
  }

  if (en_mask)
  {
    SET_BIT_V(*enr, en_mask);
    SET_BIT_V(*rstr, rst_mask);
    CLEAR_BIT_V(*rstr, rst_mask);
  }
}

void
Timer::deinit_peripheral()
{
  volatile uint32_t *enr = apb1enr();

  if (timer == TIM2)
    CLEAR_BIT_V(*enr, TIM_APB1ENR_TIM2EN);
  else if (timer == TIM3)
    CLEAR_BIT_V(*enr, TIM_APB1ENR_TIM3EN);
  else if (timer == TIM4)
    CLEAR_BIT_V(*enr, TIM_APB1ENR_TIM4EN);
}

#undef TIM_APB1ENR_TIM2EN
#undef TIM_APB1ENR_TIM3EN
#undef TIM_APB1ENR_TIM4EN
#undef TIM_APB1RSTR_TIM2RST
#undef TIM_APB1RSTR_TIM3RST
#undef TIM_APB1RSTR_TIM4RST

}; // namespace Embys::Stm32::Base
