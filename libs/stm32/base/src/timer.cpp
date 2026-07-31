#include "timer.hpp"

#include <embys/stm32/def.hpp>
#include <embys/stm32/mcu_traits.hpp>

namespace Embys::Stm32::Base
{

// H7 uses APB1LENR/APB1LRSTR for TIM2-7; all other families use
// APB1ENR/APB1RSTR. The bit positions are the same, so we alias the constants.
#if defined(STM32H7xx)
constexpr static uint32_t TIM_APB1ENR_TIM2EN = RCC_APB1LENR_TIM2EN;
constexpr static uint32_t TIM_APB1ENR_TIM3EN = RCC_APB1LENR_TIM3EN;
constexpr static uint32_t TIM_APB1ENR_TIM4EN = RCC_APB1LENR_TIM4EN;
constexpr static uint32_t TIM_APB1RSTR_TIM2RST = RCC_APB1LRSTR_TIM2RST;
constexpr static uint32_t TIM_APB1RSTR_TIM3RST = RCC_APB1LRSTR_TIM3RST;
constexpr static uint32_t TIM_APB1RSTR_TIM4RST = RCC_APB1LRSTR_TIM4RST;
// H7 splits APB1 into APB1LENR (low) and APB1HENR (high); TIM2-7 are in LENR.
// All other supported families use APB1ENR.
#define RCC_APB1ENR (RCC->APB1LENR)
#define RCC_APB1RSTR (RCC->APB1LRSTR)
#else
constexpr static uint32_t TIM_APB1ENR_TIM2EN = RCC_APB1ENR_TIM2EN;
constexpr static uint32_t TIM_APB1ENR_TIM3EN = RCC_APB1ENR_TIM3EN;
constexpr static uint32_t TIM_APB1ENR_TIM4EN = RCC_APB1ENR_TIM4EN;
constexpr static uint32_t TIM_APB1RSTR_TIM2RST = RCC_APB1RSTR_TIM2RST;
constexpr static uint32_t TIM_APB1RSTR_TIM3RST = RCC_APB1RSTR_TIM3RST;
constexpr static uint32_t TIM_APB1RSTR_TIM4RST = RCC_APB1RSTR_TIM4RST;
#define RCC_APB1ENR (RCC->APB1ENR)
#define RCC_APB1RSTR (RCC->APB1RSTR)
#endif

Timer::Timer(TIM_TypeDef *timer) : timer(timer), mode(MODE_ONESHOT)
{
  init_peripheral();
  init_common();

  SET_BIT_V(timer->DIER, TIM_DIER_UIE); // Enable update interrupt
}

Timer::Timer(TIM_TypeDef *timer, uint8_t channel)
  : timer(timer), mode(MODE_PWM), pwm_channel(channel)
{
  init_peripheral();
  init_common();
  init_pwm_channel();
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
  if (mode != MODE_ONESHOT)
    return;

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
Timer::start()
{
  timer->CNT = 0;

  if (mode == MODE_ONESHOT)
    timer->CR1 = TIM_CR1_OPM | TIM_CR1_CEN;
  else
    timer->CR1 = TIM_CR1_CEN;
}

void
Timer::stop()
{
  if (mode == MODE_ONESHOT)
    CLEAR_BIT_V(timer->CR1, TIM_CR1_OPM | TIM_CR1_CEN);
  else
    CLEAR_BIT_V(timer->CR1, TIM_CR1_CEN);
}

void
Timer::reset()
{
  if (mode != MODE_ONESHOT)
    return;

  timer->CNT = 0;
}

void
Timer::set_pwm_us(uint32_t period_us)
{
  if (mode != MODE_PWM)
    return;

  uint32_t bounded = (period_us == 0U) ? 1U : period_us;
  uint32_t arr = bounded - 1U;
  timer->ARR = (arr > arr_max) ? arr_max : arr;
  timer->EGR = TIM_EGR_UG;
}

void
Timer::set_pwm_duty(uint16_t duty_permille)
{
  if (mode != MODE_PWM)
    return;

  uint16_t bounded = duty_permille > 1000U ? 1000U : duty_permille;
  volatile uint32_t *ccr = pwm_ccr();
  if (ccr == nullptr)
    return;

  uint32_t period_ticks = timer->ARR + 1U;
  *ccr = (period_ticks * bounded) / 1000U;
}

void
Timer::init_peripheral()
{
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
    SET_BIT_V(RCC_APB1ENR, en_mask);
    SET_BIT_V(RCC_APB1RSTR, rst_mask);
    CLEAR_BIT_V(RCC_APB1RSTR, rst_mask);
  }
}

void
Timer::init_common()
{
  timer->CR1 = 0;  // Disable
  timer->CR2 = 0;  // Default configuration
  timer->SMCR = 0; // No slave mode
  timer->CNT = 0;  // Reset counter
  timer->SR = 0;   // Clear status register

  // Configure prescaler: f_tim / f_prescaled = 1 MHz => 1 tick = 1 us
  cyc_per_us = SystemCoreClock / 1000000;
  timer->PSC = static_cast<uint16_t>(cyc_per_us) - 1;
  timer->EGR = TIM_EGR_UG; // load PSC immediately
  timer->SR = 0;           // clear UIF set by UG
}

void
Timer::init_pwm_channel()
{
  if (pwm_channel < 1U || pwm_channel > 4U)
    return;

  CLEAR_BIT_V(timer->DIER, TIM_DIER_UIE);
  SET_BIT_V(timer->CR1, TIM_CR1_ARPE);

  if (pwm_channel == 1U)
  {
    CLEAR_BIT_V(timer->CCMR1, TIM_CCMR1_CC1S | TIM_CCMR1_OC1M);
    SET_BIT_V(timer->CCMR1,
              TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);
    SET_BIT_V(timer->CCER, TIM_CCER_CC1E);
  }
  else if (pwm_channel == 2U)
  {
    CLEAR_BIT_V(timer->CCMR1, TIM_CCMR1_CC2S | TIM_CCMR1_OC2M);
    SET_BIT_V(timer->CCMR1,
              TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2);
    SET_BIT_V(timer->CCER, TIM_CCER_CC2E);
  }
  else if (pwm_channel == 3U)
  {
    CLEAR_BIT_V(timer->CCMR2, TIM_CCMR2_CC3S | TIM_CCMR2_OC3M);
    SET_BIT_V(timer->CCMR2,
              TIM_CCMR2_OC3PE | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2);
    SET_BIT_V(timer->CCER, TIM_CCER_CC3E);
  }
  else
  {
    CLEAR_BIT_V(timer->CCMR2, TIM_CCMR2_CC4S | TIM_CCMR2_OC4M);
    SET_BIT_V(timer->CCMR2,
              TIM_CCMR2_OC4PE | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2);
    SET_BIT_V(timer->CCER, TIM_CCER_CC4E);
  }

#ifdef TIM_BDTR_MOE
  if (timer == TIM1
#ifdef TIM8
      || timer == TIM8
#endif
  )
    SET_BIT_V(timer->BDTR, TIM_BDTR_MOE);
#endif

  set_pwm_us(1000U);
  set_pwm_duty(500U);
}

void
Timer::deinit_peripheral()
{
  if (timer == TIM2)
    CLEAR_BIT_V(RCC_APB1ENR, TIM_APB1ENR_TIM2EN);
  else if (timer == TIM3)
    CLEAR_BIT_V(RCC_APB1ENR, TIM_APB1ENR_TIM3EN);
  else if (timer == TIM4)
    CLEAR_BIT_V(RCC_APB1ENR, TIM_APB1ENR_TIM4EN);
}

volatile uint32_t *
Timer::pwm_ccr()
{
  if (pwm_channel == 1U)
    return &timer->CCR1;
  if (pwm_channel == 2U)
    return &timer->CCR2;
  if (pwm_channel == 3U)
    return &timer->CCR3;
  if (pwm_channel == 4U)
    return &timer->CCR4;
  return nullptr;
}

}; // namespace Embys::Stm32::Base
