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
 * But the interrupt handler must be implemented by the user, and should
 * clear the interrupt flag and call the Timer's callback.
 *
 * Example:
 * ```
 * Timer timer(TIM2);
 * timer.set_callback({on_timer, context});
 *
 * // Interrupt handler
 * TIM2_IRQHandler() {
 *   CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF); // Clear interrupt flag
 *   timer(); // Will call on_timer(context) in IRQ context
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

#include <embys/stm32/types.hpp>

#include "stm32f1xx.hpp"

namespace Embys::Stm32::Base
{

class Timer
{
public:
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
   * @brief Destroy the Timer object and deinitialize the timer peripheral
   */
  ~Timer();

  /**
   * @brief Invoke the timer callback
   */
  inline void
  operator()() const
  {
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
   * @param cb Callable object representing the callback function and context
   */
  void
  set_callback(Callable<> cb);

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
   * @brief Restart the timer with current ARR
   * Will enable timer if it's disabled
   */
  void
  restart();

  /**
   * @brief Reset the timer counter
   * If the timer is disabled at the moment it won't be enabled
   */
  void
  reset();

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
  get_remaining_us()
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
    timer->ARR = us;
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

  /**
   * @brief Callable object representing the callback function and context
   */
  Callable<> cb;

  /**
   * @brief Maximum auto-reload value in microseconds
   */
  uint32_t arr_max = UINT16_MAX;

  /**
   * @brief Number of clock cycles per microsecond
   */
  uint32_t cyc_per_us = 0;

  void
  init_peripheral();

  void
  deinit_peripheral();
};


}; // namespace Embys::Stm32::Base
