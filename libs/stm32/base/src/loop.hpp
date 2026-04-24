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
 * Event *event_slots[10];
 * Event *active_event_slots[10];
 * Module module_slots[5];
 * Loop loop(&timer, event_slots, active_event_slots, 10, module_slots, 5);
 * Event startup_event(&loop, 0, []() { check_sensors(); });
 * loop.run();
 * ```
 *
 * @version 0.1
 * @date 2026-03-13
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stddef.h>

#include <embys/stm32/def.hpp>
#include <embys/stm32/types.hpp>

#include "cs.hpp"
#include "event.hpp"
#include "stm32f1xx.hpp"
#include "timer.hpp"

namespace Embys::Stm32::Base
{

struct Module
{
  Callable<> cb;
  volatile bool interrupted = false;
};

class Loop
{
public:
  Loop() = delete;
  Loop(const Loop &) = delete;
  Loop(Loop &&) = delete;
  Loop &
  operator=(const Loop &) = delete;
  Loop &
  operator=(Loop &&) = delete;

  /**
   * @brief Initialize a new Loop object with specified timer, event slots, and
   * module slots.
   *
   * @param timer Pointer to the Timer object for hardware timing
   * @param event_slots Array to hold pointers to scheduled events
   * @param active_event_slots Array to hold pointers to active events in the
   * current loop iteration
   * @param events_capacity Capacity of the event slots array
   * @param module_slots Array to hold registered modules
   * @param modules_capacity Capacity of the module slots array
   */
  Loop(Timer *timer, Event **event_slots, Event **active_event_slots,
       size_t events_capacity, Module *module_slots, size_t modules_capacity);

  /**
   * @brief Clean up Base system resources.
   * This includes clearing the timer callback and removing any scheduled stop
   * event.
   */
  ~Loop();

  /**
   * @brief Start main loop execution.
   */
  void
  run();

  /**
   *@brief Stop main loop execution after specified microseconds.
   *
   * @param us Time in microseconds to wait before stopping the loop. If 0, the
   * loop will stop immediately after the current iteration.
   * @return int
   */
  int
  stop(uint32_t us = 0);

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
   * @brief Get the number of currently active events.
   *
   * @return size_t Number of active events
   */
  size_t
  count_events();

  /**
   * @brief Register a module callback for IRQ-triggered processing.
   *
   * @param module_cb The callback function to be registered as a module.
   * @return Module* Pointer to the registered module. Returns nullptr if
   * registration fails (e.g., no available slots).
   */
  Module *
  add_module(Callable<> module_cb);

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
  interrupted(Module *module)
  {
    cs_begin();

    if (!module->interrupted)
    {
      module->interrupted = true;
      INC_V(interrupted_modules_count);
    }

    cs_end();
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
  bool stop_scheduled = false;

  /**
   * @brief Count of modules that have been interrupted and require processing
   * in the main loop
   */
  size_t interrupted_modules_count = 0;

  /**
   * @brief Indicates if the main loop is currently active
   */
  volatile bool active;

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

}; // namespace Embys::Stm32::Base
