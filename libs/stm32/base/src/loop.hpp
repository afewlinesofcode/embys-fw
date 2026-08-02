/**
 * @file loop.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Main loop management for the Base system
 * Provides the Loop class which manages event scheduling, execution, and module
 * callbacks. The Loop class integrates with the Timer for hardware timing and
 * ensures precise event execution.
 *
 * Example:
 * ```
 * Timer timer(TIM2);
 * Loop<10, 5> loop(timer);
 * Event startup_event(loop, EventMode::Deferred, {check_sensors, nullptr});
 * loop.run();
 * ```
 *
 * @version 0.1
 * @date 2026-03-13
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <chrono>
#include <cstddef>

#include <embys/stm32/def.hpp>
#include <embys/stm32/types.hpp>

#include "cs.hpp"
#include "event.hpp"
#include "stm32xx.hpp"
#include "timer.hpp"

namespace Embys::Stm32::Base
{

struct Module
{
  Callback<> cb;
  volatile bool interrupted = false;
};

class LoopCore
{
public:
  LoopCore() = delete;
  LoopCore(const LoopCore &) = delete;
  LoopCore(LoopCore &&) = delete;
  LoopCore &
  operator=(const LoopCore &) = delete;
  LoopCore &
  operator=(LoopCore &&) = delete;

  /**
   * @brief Initialize the type-erased loop core over owned template storage.
   *
   * @param timer Pointer to the Timer object for hardware timing
   * @param event_slots Array to hold pointers to scheduled events
   * @param active_event_slots Array to hold pointers to active events in the
   * current loop iteration
   * @param events_capacity Capacity of the event slots array
   * @param module_slots Array to hold registered modules
   * @param modules_capacity Capacity of the module slots array
   */
  LoopCore(Timer &timer, Event **event_slots, Event **active_event_slots,
           size_t events_capacity, Module *module_slots,
           size_t modules_capacity);

  /**
   * @brief Clean up Base system resources.
   * This includes clearing the timer callback and removing any scheduled stop
   * event.
   */
  ~LoopCore();

  /**
   * @brief Start main loop execution.
   */
  void
  run();

  /**
   *@brief Stop main loop execution after the specified delay.
   *
   * @param delay Time to wait before stopping the loop. A zero duration stops
   * after the current iteration.
   * @return int
   */
  int
  stop(std::chrono::microseconds delay = std::chrono::microseconds::zero());

  /**
   * @brief Terminate the loop with a specified exit code and optional error
   * context.
   *
   * @param code Exit code to indicate the reason for termination (e.g., 0 for
   * normal exit, non-zero for errors).
   * @param error_context Optional pointer to additional context about the error
   * (can be nullptr if not applicable).
   */
  void
  terminate(int code, void *error_context = nullptr);

  /**
   * @brief Add event to scheduler and initialize it.
   *
   * @param event Pointer to the event to be added.
   * @return int Status code indicating success or failure.
   */
  int
  add(Event *event);

  /**
   * @brief Remove event from scheduler.
   *
   * @param event Pointer to the event to be removed.
   * @return int Status code indicating success or failure.
   */
  int
  remove(Event *event);

  /**
   * @brief Check if the main loop is currently active.
   * @return true if the loop is active, false otherwise
   */
  inline bool
  is_active() const
  {
    return active;
  }

  /**
   * @brief Get the exit code indicating the reason for loop termination.
   * @return int Exit code (e.g., 0 for normal exit, non-zero for errors)
   */
  inline int
  get_exit_code() const
  {
    return exit_code;
  }

  /**
   * @brief Get the error context associated with the loop termination.
   * @return void* Pointer to the error context (can be nullptr if not
   * applicable)
   */
  inline void *
  get_error_context() const
  {
    return error_context;
  }

  /**
   * @brief Register a module callback for IRQ-triggered processing.
   *
   * @param module_cb The callback function to be registered as a module.
   * @return Module* Pointer to the registered module. Returns nullptr if
   * registration fails (e.g., no available slots).
   */
  Module *
  add_module(Callback<> module_cb);

  /**
   * @brief Unregister a module callback.
   *
   * @param module Pointer to the module to be removed.
   */
  void
  remove_module(Module *module);

  /**
   * @brief Notify that a module has been interrupted and requires processing in
   * the application context.
   * This should be called from the module's interrupt handler to ensure that
   * the main loop executes the module's callback as soon as possible.
   * @param module Pointer to the module that has been interrupted.
   */
  inline void
  set_module_pending(Module *module)
  {
    IrqGuard guard;

    if (!module->interrupted)
    {
      module->interrupted = true;
      INC_V(interrupted_modules_count);
    }
  }

private:
  /**
   * @brief Pointer to Timer for hardware timing
   */
  Timer *timer;

  /**
   * @brief Array of pointers to scheduled events
   */
  Event **events;

  /**
   * @brief Array of pointers to active events in the current loop iteration
   */
  Event **active_events;

  /**
   * @brief Capacity of the event slots array
   */
  size_t events_capacity;

  /**
   * @brief Array of registered modules
   */
  Module *modules;

  /**
   * @brief Capacity of the modules array
   */
  size_t modules_capacity;

  /**
   * @brief Number of active events in the current loop iteration
   */
  volatile size_t active_events_count = 0;

  /**
   * @brief Index of the next active event to be processed
   */
  volatile size_t active_event_idx = 0;

  /**
   * @brief Internal stop event for graceful loop termination
   */
  Event stop_event;

  /**
   * @brief Indicates if a stop event is scheduled
   */
  volatile bool stop_scheduled = false;

  /**
   * @brief Count of modules that have been interrupted and require processing
   * in the main loop
   */
  volatile size_t interrupted_modules_count = 0;

  /**
   * @brief Indicates if the main loop is currently active
   */
  volatile bool active;

  /**
   * @brief Exit code to indicate the reason for loop termination (e.g., 0 for
   * normal exit, non-zero for errors).
   */
  int exit_code = 0;

  /**
   * @brief Pointer to additional context about the error
   */
  void *error_context = nullptr;

  /**
   * @brief Activate expired events, run real-time events, update schedule
   */
  void
  tick();

  /**
   * @brief Check if there are active events pending execution
   * @return true if there are active events, false otherwise
   */
  inline bool
  has_active_events() const
  {
    return active_event_idx < active_events_count;
  }

  /**
   * @brief Schedule event execution after specified microseconds
   * @param event Pointer to the event to be scheduled
   * @param us Time in microseconds after which the event should be executed
   */
  void
  schedule_event(Event *event, uint32_t us);

  /**
   * @brief Execute all active deferred events
   */
  void
  run_active_events();

  /**
   * @brief Execute callbacks of all interrupted modules
   */
  void
  run_modules();

  /**
   * @brief Timer callback to be called on timer events, responsible for ticking
   * the loop
   * @param context Pointer to the loop instance
   */
  static void
  timer_callback(void *context);

  /**
   * @brief Static callback for loop termination
   * @param context Pointer to the loop instance
   */
  static void
  loopbreak_callback(void *context);
};

namespace Detail
{

template <size_t EventsCapacity, size_t ModulesCapacity>
struct LoopStorage
{
  static_assert(EventsCapacity > 0, "A loop needs at least one event slot");
  static_assert(ModulesCapacity > 0, "A loop needs at least one module slot");

  std::array<Event *, EventsCapacity> events{};
  std::array<Event *, EventsCapacity> active_events{};
  std::array<Module, ModulesCapacity> modules{};
};

} // namespace Detail

template <size_t EventsCapacity, size_t ModulesCapacity>
class Loop final
  : private Detail::LoopStorage<EventsCapacity, ModulesCapacity>,
    public LoopCore
{
  using Storage = Detail::LoopStorage<EventsCapacity, ModulesCapacity>;

public:
  explicit Loop(Timer &timer)
    : Storage(),
      LoopCore(timer, Storage::events.data(), Storage::active_events.data(),
               EventsCapacity, Storage::modules.data(), ModulesCapacity)
  {
  }
};

}; // namespace Embys::Stm32::Base
