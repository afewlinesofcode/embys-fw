#include "event.hpp"

#include "loop.hpp"

namespace Embys::Stm32::Base
{

Event::Event(Loop *loop, uint8_t flags, Callable<> cb)
  : loop(loop), flags(flags), cb(cb)
{
}

int
Event::enable(uint32_t us)
{
  interval_us = us;
  next_time_us = interval_us;
  return loop->add(this);
}

int
Event::disable()
{
  return loop->remove(this);
}

bool
Event::advance_us(uint32_t elapsed_us)
{
  if (next_time_us > elapsed_us)
  {
    // Event not ready - subtract elapsed time
    next_time_us -= elapsed_us;
    return false;
  }

  // Event ready to execute
  uint32_t overrun_us = elapsed_us - next_time_us;
  if (overrun_us >= interval_us)
    next_time_us = 0; // Handle extreme overrun by scheduling immediately
  else
    next_time_us = interval_us - overrun_us; // Schedule next occurrence

  return true;
}

}; // namespace Embys::Stm32::Base
