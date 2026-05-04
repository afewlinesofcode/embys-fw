/**
 * @file statistics.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus RTU server statistics counters
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/modbus/statistics.hpp>

namespace Embys::Stm32::Modbus::Rtu
{

struct ServerStats
{
  uint16_t responses = 0;
  uint16_t exceptions = 0;
  uint16_t crc_errors = 0;
};

class ServerStatistics : public Modbus::Statistics<ServerStats>
{
public:
  ServerStatistics() = default;

  inline uint16_t
  get_responses() const
  {
    return get(&ServerStats::responses);
  }

  inline uint16_t
  get_exceptions() const
  {
    return get(&ServerStats::exceptions);
  }

  inline uint16_t
  get_crc_errors() const
  {
    return get(&ServerStats::crc_errors);
  }

  inline void
  inc_responses()
  {
    increment(&ServerStats::responses);
  }

  inline void
  inc_exceptions()
  {
    increment(&ServerStats::exceptions);
  }

  inline void
  inc_crc_errors()
  {
    increment(&ServerStats::crc_errors);
  }
};

}; // namespace Embys::Stm32::Modbus::Rtu
