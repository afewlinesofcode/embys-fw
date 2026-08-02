/**
 * @file timer.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Timer abstraction for STM32F1 series
 *
 * Provides a hardware timer abstraction for general purpose timers. The Timer
 * class manages timer configuration, scheduling, and callback invocation with
 * microsecond precision.
 * The timer peripheral will be automatically enabled and reset by the Timer
 * class.
 * But the interrupt handler must be implemented by the user, and interrupt must
 * be enabled in NVIC, and priority set.
 *
 * Example:
 * ```
 * Timer timer(TIM2);
 * timer.set_callback({on_timer, context});
 *
 * __NVIC_SetPriority(TIM2_IRQn, 0x00);
 * __NVIC_EnableIRQ(TIM2_IRQn);
 *
 * // Interrupt handler
 * TIM2_IRQHandler() {
 *   CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF); // Clear interrupt flag
 *   timer.handle_irq(); // Will call on_timer(context) in IRQ context
 * }
 * ```
 *
 * @version 0.1
 * @date 2026-03-13
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/def.hpp>
#include <embys/stm32/mcu_traits.hpp>
#include <embys/stm32/types.hpp>

#include "stm32xx.hpp"

namespace Embys::Stm32::Base
{

class Timer
{
public:
  enum Mode : uint8_t
  {
    MODE_ONESHOT = 0,
    MODE_PWM = 1,
  };

  // Deleted constructors and assignment operators to enforce unique ownership
  Timer() = delete;
  Timer(const Timer &) = delete;
  Timer(Timer &&) = delete;
  Timer &
  operator=(const Timer &) = delete;
  Timer &
  operator=(Timer &&) = delete;

  /**
   * @brief Initialize a new Timer object with specified timer instance
   * @param timer Pointer to the timer peripheral instance
   */
  Timer(TIM_TypeDef *timer);

  /**
   * @brief Initialize a Timer in PWM mode for a specific channel.
   * The timer is configured but remains stopped until pwm_start() is called.
   * @param timer Pointer to the timer peripheral instance
   * @param channel PWM channel number in range [1..4]
   */
  Timer(TIM_TypeDef *timer, uint8_t channel);

  /**
   * @brief Destroy the Timer object and deinitialize the timer peripheral
   */
  ~Timer();

  inline TIM_TypeDef *
  get_peripheral() const
  {
    return timer;
  }

  inline Mode
  get_mode() const
  {
    return mode;
  }

  inline uint8_t
  get_pwm_channel() const
  {
    return pwm_channel;
  }

  /**
   * @brief Invoke the timer callback
   */
  inline void
  handle_irq()
  {
    CLEAR_BIT_V(timer->SR, TIM_SR_UIF); // Clear interrupt flag
    cb();
  }

  /**
   * @brief Get the number of clock cycles per microsecond
   * @return uint32_t Clock cycles per microsecond
   */
  inline uint32_t
  get_cyc_per_us() const
  {
    return cyc_per_us;
  }

  /**
   * @brief Set the maximum auto-reload value in microseconds
   * Timer cannot schedule events beyond this value due to hardware limitations.
   * @param max_us Maximum auto-reload value
   */
  inline void
  set_arr_max(uint32_t max_us)
  {
    arr_max = max_us;
  }

  /**
   * @brief Set the callback to be invoked on timer events
   * @param cb Callback object representing the callback function and context
   */
  void
  set_callback(Callback<> cb);

  /**
   * @brief Schedule a timer event in microseconds,
   * or stop the timer if us is UINT32_MAX
   * @param us Time in microseconds
   * @param jitter_us Jitter in microseconds to be added to the counted time
   * @param start Whether to start the timer immediately
   */
  void
  schedule_us(uint32_t us, uint32_t jitter_us, bool start);

  /**
   * @brief Start or restart the timer with current ARR
   * Will enable timer if it's disabled
   */
  void
  start();

  /**
   * @brief Stop the timer
   */
  void
  stop();

  /**
   * @brief Reset the timer counter
   * If the timer is disabled at the moment it won't be enabled
   */
  void
  reset();

  /**
   * @brief Set PWM period in microseconds.
   */
  void
  set_pwm_us(uint32_t period_us);

  /**
   * @brief Set PWM duty cycle in permille (0..1000).
   */
  void
  set_pwm_duty(uint16_t duty_permille);

  /**
   * @brief Check if the timer is enabled
   * @return true if the timer is enabled, false otherwise
   */
  inline bool
  is_enabled() const
  {
    return (timer->CR1 & TIM_CR1_CEN);
  }

  /**
   * @brief Get the remaining time in microseconds
   * @return uint32_t Remaining time in microseconds
   */
  inline uint32_t
  get_remaining_us() const
  {
    return (timer->CR1 & TIM_CR1_CEN) ? (timer->ARR - timer->CNT) : 0;
  }

  /**
   * @brief Set the scheduled time in microseconds
   * @param us Time in microseconds
   */
  inline void
  set_scheduled_us(uint32_t us)
  {
    timer->ARR = us > arr_max ? arr_max : us;
  }

  /**
   * @brief Get the scheduled time in microseconds
   * @return uint32_t Scheduled time in microseconds
   */
  inline uint32_t
  get_scheduled_us() const
  {
    return timer->ARR;
  }

  /**
   * @brief Get the elapsed time in microseconds
   * @return uint32_t Elapsed time in microseconds
   */
  inline uint32_t
  get_elapsed_us() const
  {
    return timer->CNT;
  }

  /**
   * @brief Check if the timer has been triggered
   * @return true if the timer has been triggered, false otherwise
   */
  inline bool
  is_triggered() const
  {
    return (timer->SR & TIM_SR_UIF);
  }

private:
  /**
   * @brief Pointer to the timer peripheral instance
   */
  TIM_TypeDef *timer;

  Mode mode = MODE_ONESHOT;
  uint8_t pwm_channel = 0;

  /**
   * @brief Callback object representing the callback function and context
   */
  Callback<> cb;

  /**
   * @brief Maximum auto-reload value in microseconds.
   * Defaults to the exact target's TIM2 register width.
   */
  uint32_t arr_max = McuTraits::timer2_arr_max;

  /**
   * @brief Number of clock cycles per microsecond
   */
  uint32_t cyc_per_us = 0;

  void
  init_peripheral();

  void
  init_common();

  void
  init_pwm_channel();

  void
  deinit_peripheral();

  volatile uint32_t *
  pwm_ccr();
};


}; // namespace Embys::Stm32::Base
