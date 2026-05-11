/**
 * @file diagnostics.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus diagnostics counters (FC 0x08 sub-functions)
 *
 * DiagnosticsCounters holds the six counters exposed by the Modbus
 * Diagnostics function code (0x08).  All fields are zero-initialised on
 * construction and can be reset with reset().
 *
 * The RTU Server owns an instance and passes a pointer to Handler so that
 * FC 0x08 requests can read and clear the live values.
 *
 * @version 0.1
 * @date 2026-05-11
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

namespace Embys::Stm32::Modbus
{

struct DiagnosticsCounters
{
  uint16_t bus_message_count = 0;
  uint16_t bus_comm_error_count = 0;
  uint16_t bus_exception_error_count = 0;
  uint16_t slave_message_count = 0;
  uint16_t slave_no_response_count = 0;
  uint16_t slave_busy_count = 0;

  void
  reset()
  {
    *this = DiagnosticsCounters{};
  }
};

}; // namespace Embys::Stm32::Modbus
