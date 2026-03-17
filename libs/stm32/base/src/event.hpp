/**
 * @file event.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief A structure representing an individual event in the Base system.
 * Events are the units of work that the main loop schedules and executes based
 * on timing and flags. An event must be enabled to be scheduled.
 *
 * Example:
 * ```
 * Event blink_event(&loop, EV_PERSIST, []() { toggle_led(); });
 * blink_event.enable(500000); // Schedule to run every half second
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

namespace Embys::Stm32::Base
{

class Loop;

// Event flags

// Event should persist after execution (rescheduled automatically)
constexpr uint8_t EV_PERSIST = 1 << 0;

// Event is real-time (executed in IRQ context, should be short and
// non-blocking)
constexpr uint8_t EV_RT = 1 << 1;

struct Event
{
  /**
   * @brief Pointer to the Base system loop managing this event
   */
  Loop *loop;

  /**
   * @brief Event flags (EV_PERSIST, EV_RT, etc.)
   */
  uint8_t flags;

  /**
   * @brief Event callback function to be executed when the event is triggered
   */
  Callable<> cb;

  /**
   * @brief Event timing interval in microseconds
   *
   */
  uint32_t interval_us = 0;

  /**
   * @brief Next execution time in microseconds
   */
  uint32_t next_time_us = 0;

  /**
   * @brief Whether the event is currently pending in the scheduler
   */
  bool pending = false;

  // Deleted constructors and assignment operators to enforce unique ownership
  Event() = delete;
  Event(const Event &) = delete;
  Event(Event &&) = delete;
  Event &
  operator=(const Event &) = delete;
  Event &
  operator=(Event &&) = delete;

  /**
   * @brief Initialize a new Event object
   * @param loop Pointer to the Base system loop managing this event
   * @param flags Event flags (EV_PERSIST, EV_RT, etc.)
   * @param cb Event callback function
   */
  Event(Loop *loop, uint8_t flags, Callable<> cb);

  /**
   * @brief Enable the event with a specified interval in microseconds
   * @param us Interval in microseconds
   * @return 0 on success, negative error code on failure
   */
  int
  enable(uint32_t us);

  /**
   * @brief Disable the event, removing it from the scheduler
   * @return 0 on success, negative error code on failure
   */
  int
  disable();

  /**
   * @brief Advance event timer by elapsed microseconds
   * @param elapsed_us Elapsed time in microseconds
   * @return true if event is ready to execute
   */
  bool
  advance_us(uint32_t elapsed_us);
};

}; // namespace Embys::Stm32::Base
