/**
 * @file statistics.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief In-memory statistics counter template for Modbus layers
 *
 * Statistics<T> tracks counters defined in the user-supplied struct T.
 * Counters are incremented with @c increment() and read with @c get().
 * No dynamic allocation; all state lives in the struct.
 *
 * Example:
 * ```
 * struct MyStats { uint32_t responses; uint32_t errors; };
 * Statistics<MyStats> stats;
 * stats.increment(&MyStats::responses);
 * uint32_t r = stats.get(&MyStats::responses);
 * ```
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

namespace Embys::Stm32::Modbus
{

template <typename T>
class Statistics
{
public:
  Statistics() = default;
  Statistics(const Statistics &) = delete;
  Statistics(Statistics &&) = delete;
  Statistics &
  operator=(const Statistics &) = delete;
  Statistics &
  operator=(Statistics &&) = delete;

  template <typename Member>
  void
  increment(Member T::*member)
  {
    ++(stats.*member);
  }

  template <typename Member>
  auto
  get(Member T::*member) const
  {
    return stats.*member;
  }

  void
  reset()
  {
    stats = T{};
  }

private:
  T stats{};
};

}; // namespace Embys::Stm32::Modbus
