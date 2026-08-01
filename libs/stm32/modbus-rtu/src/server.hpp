/**
 * @file server.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus RTU server (slave)
 *
 * Server listens on the bus for frames addressed to device_id, passes them
 * to the Handler for processing, and transmits the response.  Broadcast
 * frames (device ID 0) are executed but produce no response.
 *
 * Usage:
 * ```
 * Modbus::Server server(1, &handler, &uart_bus);
 * server.enable();
 * ```
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/modbus/diagnostics.hpp>
#include <embys/stm32/modbus/handler.hpp>
#include <embys/stm32/types.hpp>

#include "base.hpp"

namespace Embys::Stm32::Modbus::Rtu
{

class Server : public Base
{
public:
  Server() = delete;
  Server(const Server &) = delete;
  Server(Server &&) = delete;
  Server &
  operator=(const Server &) = delete;
  Server &
  operator=(Server &&) = delete;

  Server(uint8_t device_id, Modbus::Handler &handler, Uart::BusCore &transport);
  ~Server();

  inline const Modbus::DiagnosticsCounters &
  get_diagnostics_counters() const
  {
    return diag_counters;
  }

  /**
   * @brief Register a callback invoked for every valid (CRC-OK, addressed)
   * request, with the raw PDU bytes and length (CRC already stripped).
   * Byte layout: [0]=device_id, [1]=FC, [2:3]=starting address.
   */
  inline void
  set_on_request_callback(Embys::Callback<const uint8_t *, uint16_t> cb)
  {
    on_request_cb = cb;
  }

  void
  enable();

private:
  uint8_t device_id;
  Modbus::Handler &handler;
  bool handling_request = false;

  Embys::Callback<const uint8_t *, uint16_t> on_request_cb;

  Modbus::DiagnosticsCounters diag_counters;

  int
  process_request();

  int
  send_response();

  int
  send_exception(uint8_t exception_code);

  static void
  request_callback(void *context);

  static void
  response_sent_callback(void *context, int status);
};

}; // namespace Embys::Stm32::Modbus::Rtu
