#include "timer.hpp"

#include <embys/stm32/def.hpp>

namespace Embys::Stm32::Base
{

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
  timer->ARR = us > arr_max ? arr_max : us;
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

void
Timer::init_peripheral()
{
  if (timer == TIM2)
  {
    // Enable TIM2 clock
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);
    // Reset TIM2 peripheral to ensure it's in a known state
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_TIM2RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_TIM2RST);
    // Set TIM2 interrupt priority and enable it
    __NVIC_SetPriority(TIM2_IRQn, 0x00);
    __NVIC_EnableIRQ(TIM2_IRQn);
  }
  else if (timer == TIM3)
  {
    // Enable TIM3 clock
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);
    // Reset TIM3 peripheral to ensure it's in a known state
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_TIM3RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_TIM3RST);
    // Set TIM3 interrupt priority and enable it
    __NVIC_SetPriority(TIM3_IRQn, 0x00);
    __NVIC_EnableIRQ(TIM3_IRQn);
  }
  else if (timer == TIM4)
  {
    // Enable TIM4 clock
    SET_BIT_V(RCC->APB1ENR, RCC_APB1ENR_TIM4EN);
    // Reset TIM4 peripheral to ensure it's in a known state
    SET_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_TIM4RST);
    CLEAR_BIT_V(RCC->APB1RSTR, RCC_APB1RSTR_TIM4RST);
    // Set TIM4 interrupt priority and enable it
    __NVIC_SetPriority(TIM4_IRQn, 0x00);
    __NVIC_EnableIRQ(TIM4_IRQn);
  }
}

void
Timer::deinit_peripheral()
{
  if (timer == TIM2)
  {
    // Disable TIM2 interrupt
    __NVIC_DisableIRQ(TIM2_IRQn);
    // Disable TIM2 clock
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_TIM2EN);
  }
  else if (timer == TIM3)
  {
    // Disable TIM3 interrupt
    __NVIC_DisableIRQ(TIM3_IRQn);
    // Disable TIM3 clock
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_TIM3EN);
  }
  else if (timer == TIM4)
  {
    // Disable TIM4 interrupt
    __NVIC_DisableIRQ(TIM4_IRQn);
    // Disable TIM4 clock
    CLEAR_BIT_V(RCC->APB1ENR, RCC_APB1ENR_TIM4EN);
  }
}

}; // namespace Embys::Stm32::Base
