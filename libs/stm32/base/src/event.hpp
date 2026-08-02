/**
 * @file event.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief A structure representing an individual event in the Base system.
 * Events are the units of work that the main loop schedules and executes based
 * on timing and flags. An event must be enabled to be scheduled.
 *
 * Example:
 * ```
 * Event blink_event(loop, EventMode::Persistent, {toggle_led, nullptr});
 * blink_event.enable(std::chrono::milliseconds{500});
 * ```
 *
 * @version 0.1
 * @date 2026-03-13
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <chrono>
#include <cstdint>

#include <embys/stm32/types.hpp>

namespace Embys::Stm32::Base
{

class LoopCore;

enum class EventMode : uint8_t
{
  Deferred = 0,
  Persistent = 1U << 0,
  Realtime = 1U << 1,
};

[[nodiscard]] constexpr EventMode
operator|(EventMode lhs, EventMode rhs) noexcept
{
  return static_cast<EventMode>(static_cast<uint8_t>(lhs) |
                                static_cast<uint8_t>(rhs));
}

[[nodiscard]] constexpr bool
has_mode(EventMode value, EventMode flag) noexcept
{
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0U;
}

struct Event
{
  /**
   * @brief Pointer to the Base system loop managing this event
   */
  LoopCore *loop;

  /**
   * @brief Scheduling and callback execution mode.
   */
  EventMode mode;

  /**
   * @brief Event callback function to be executed when the event is triggered
   */
  Callback<> cb;

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
   * @param mode Scheduling and callback execution mode.
   * @param cb Event callback function
   */
  Event(LoopCore &loop, EventMode mode, Callback<> cb);

  /**
   * @brief Enable the event with a specified interval.
   * @param interval Interval represented at microsecond precision.
   * @return 0 on success, negative error code on failure
   */
  int
  enable(std::chrono::microseconds interval);

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
