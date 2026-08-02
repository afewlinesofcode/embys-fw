#include "loop.hpp"

namespace Embys::Stm32::Base
{

LoopCore::LoopCore(Timer &timer, Event **event_slots,
                   Event **active_event_slots, size_t events_capacity,
                   Module *module_slots, size_t modules_capacity)
  : timer(&timer), events(event_slots), active_events(active_event_slots),
    events_capacity(events_capacity), modules(module_slots),
    modules_capacity(modules_capacity),
    stop_event(*this, EventMode::Realtime, {loopbreak_callback, this}),
    active(false)
{
  // Initialize event arrays
  for (size_t i = 0; i < events_capacity; ++i)
  {
    events[i] = nullptr;
    active_events[i] = nullptr;
  }

  // Initialize module array
  for (size_t i = 0; i < modules_capacity; ++i)
  {
    modules[i].interrupted = false;
    modules[i].cb.clear();
  }

  this->timer->set_callback({timer_callback, this});
}

LoopCore::~LoopCore()
{
  timer->set_callback({}); // Clear timer callback

  if (stop_scheduled)
  {
    // Remove previously scheduled stop event
    remove(&stop_event);
  }
}

void
LoopCore::run()
{
  active = true;

  do
  {
    /*
     * Main loop execution flow:
     * 1. Process all active events (deferred from IRQ context).
     * On the first iteration, this will also process any events that were
     * scheduled as startup events with us=0.
     * 2. Execute registered module callbacks.
     * Peripherals or other entities may register themselves in this
     * instance as modules and notify about processing in the
     * application context is required from their interrupt handlers.
     * 3. Wait for next interrupt (if no more modules have been interrupted).
     */

    run_active_events();
    run_modules();

    {
      IrqGuard guard;

      if (interrupted_modules_count == 0)
      {
        __DSB();
        __WFI();
      }
    }
  } while (active);

  // Process any remaining active events and modules before exiting
  run_active_events();
  run_modules();
}

int
LoopCore::stop(std::chrono::microseconds delay)
{
  if (stop_scheduled)
  {
    stop_event.disable();
  }

  // Schedule loop termination callback
  const int result = stop_event.enable(delay);
  stop_scheduled = (result == 0);
  return result;
}

void
LoopCore::terminate(int code, void *error_context)
{
  this->exit_code = code;
  this->error_context = error_context;
  active = false;
}

int
LoopCore::add(Event *event)
{
  /*
   * Event may suddenly become not pending after the check below,
   * if it is not persistent, and become removed from the list, thus critical
   * section
   */

  {
    IrqGuard guard;

    if (event->pending)
    {
      // Event already in the loop, reschedule it
      schedule_event(event, event->interval_us);
      return 0;
    }
  }

  for (size_t i = 0; i < events_capacity; ++i)
  {
    if (events[i] == nullptr)
    {
      events[i] = event;
      event->pending = true;
      schedule_event(event, event->interval_us);
      return 0;
    }
  }

  return -1;
}

int
LoopCore::remove(Event *event)
{
  for (size_t i = 0; i < events_capacity; ++i)
  {
    if (events[i] == event)
    {
      events[i] = nullptr;
      event->pending = false;
      return 0;
    }
  }

  // Event not found, don't care
  return 0;
}

Module *
LoopCore::add_module(Callback<> module_cb)
{
  for (size_t i = 0; i < modules_capacity; ++i)
  {
    if (modules[i].cb.empty())
    {
      modules[i].cb = module_cb;
      return &modules[i];
    }
  }

  return nullptr;
}

void
LoopCore::remove_module(Module *module)
{
  module->cb.clear();
}

void
LoopCore::tick()
{
  // Reset active events processing
  if (active_event_idx == active_events_count)
  {
    active_event_idx = 0;
    active_events_count = 0;
  }

  uint32_t soonest_us = UINT32_MAX;
  uint32_t elapsed_us = timer->get_scheduled_us();

  // Process all pending events and determine the soonest next event time
  for (size_t i = 0; i < events_capacity; ++i)
  {
    Event *event = events[i];

    if (event == nullptr)
    {
      continue;
    }

    if (event->advance_us(elapsed_us))
    {
      // Event is due - execute callback

      // Handle persistence
      if (!has_mode(event->mode, EventMode::Persistent))
      {
        events[i] = nullptr; // Single-shot
        event->pending = false;
      }

      if (has_mode(event->mode, EventMode::Realtime))
      {
        event->cb();
      }
      else
      {
        // Deferred event - queue for main loop execution
        active_events[active_events_count] = event;
        active_events_count = active_events_count + 1;
      }
    }

    // Update soonest event time if the event is still pending
    if (events[i] && event->next_time_us < soonest_us)
    {
      soonest_us = event->next_time_us;
    }
  }

  timer->schedule_us(soonest_us, 0, true);
}

void
LoopCore::schedule_event(Event *event, uint32_t us)
{
  if (!timer->is_enabled())
  {
    // Timer is not running - start with this event
    event->next_time_us = us;
    timer->schedule_us(event->next_time_us, 0, true);
  }
  else
  {
    // Adjust new event time relative to current timer
    event->next_time_us = timer->get_elapsed_us() + us;

    if (event->next_time_us < timer->get_scheduled_us())
    {
      // New event is sooner than current schedule - update timer
      timer->set_scheduled_us(event->next_time_us);
    }
  }
}

void
LoopCore::run_active_events()
{
  // Process deferred events in main loop context
  while (has_active_events())
  {
    auto *event = active_events[active_event_idx];
    INC_V(active_event_idx);
    event->cb();
  }
}

void
LoopCore::run_modules()
{
  // Execute all active module callbacks
  for (size_t i = 0; i < modules_capacity; ++i)
  {
    if (modules[i].interrupted)
    {
      {
        IrqGuard guard;

        modules[i].interrupted = false;
        DEC_V(interrupted_modules_count);
      }

      modules[i].cb();
    }
  }
}

void
LoopCore::timer_callback(void *context)
{
  auto loop = static_cast<LoopCore *>(context);
  loop->tick();
}

void
LoopCore::loopbreak_callback(void *context)
{
  auto loop = static_cast<LoopCore *>(context);
  loop->stop_scheduled = false;
  loop->active = false;
}

}; // namespace Embys::Stm32::Base
