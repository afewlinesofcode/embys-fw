#include "event.hpp"

#include <limits>

#include "loop.hpp"

namespace Embys::Stm32::Base
{

Event::Event(LoopCore &loop, EventMode mode, Callback<> cb)
  : loop(&loop), mode(mode), cb(cb)
{
}

int
Event::enable(std::chrono::microseconds interval)
{
  const auto count = interval.count();
  if (count < 0 || static_cast<uint64_t>(count) >
                       std::numeric_limits<uint32_t>::max())
  {
    return -1;
  }

  interval_us = static_cast<uint32_t>(count);
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
