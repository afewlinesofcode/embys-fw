/**
 * @file client.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus RTU client (master)
 *
 * Client serialises Modbus requests over an RTU bus and dispatches the parsed
 * response (or a timeout notification) via a caller-supplied ResponseCallback.
 *
 * Only one request may be in flight at a time; is_available() returns false
 * while a response is pending.  Broadcast requests (device_id == 0) do not
 * wait for a response.
 *
 * Required events: 1 (frame_timeout from Base) + 1 (response_timeout_event)
 *
 * Usage:
 * ```
 * Modbus::Rtu::Client client(&uart_bus, &loop);
 * client.enable();
 * client.read_holding_registers(1, 0, 4, {on_response, &ctx});
 * ```
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/base/event.hpp>
#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/modbus/statistics.hpp>
#include <embys/stm32/types.hpp>

#include "base.hpp"

namespace Embys::Stm32::Modbus::Rtu
{

class Client : public Base
{
public:
  /**
   * @brief Response callback signature.
   *
   * Arguments: device_id, function_code, quantity, data_ptr.
   * On timeout, quantity == 0 and data_ptr == nullptr.
   * On exception response, function_code has bit 7 set.
   */
  using ResponseCallback =
      Embys::Callback<uint8_t, uint8_t, uint8_t, uint8_t *>;

  static inline uint32_t kRequestTimeoutUs = 5U * 1000000U; // 5 seconds

  Client() = delete;
  Client(const Client &) = delete;
  Client(Client &&) = delete;
  Client &
  operator=(const Client &) = delete;
  Client &
  operator=(Client &&) = delete;

  Client(Uart::BusCore &transport, Stm32::Base::LoopCore &loop);
  ~Client();

  void
  enable();

  inline bool
  is_available() const
  {
    return !expecting_response;
  }

  int
  read_coils(uint8_t device_id, uint16_t starting_address, uint16_t quantity,
             ResponseCallback cb);

  int
  read_discrete_inputs(uint8_t device_id, uint16_t starting_address,
                       uint16_t quantity, ResponseCallback cb);

  int
  read_holding_registers(uint8_t device_id, uint16_t starting_address,
                         uint16_t quantity, ResponseCallback cb);

  int
  read_input_registers(uint8_t device_id, uint16_t starting_address,
                       uint16_t quantity, ResponseCallback cb);

  int
  write_single_coil(uint8_t device_id, uint16_t address, bool value,
                    ResponseCallback cb);

  int
  write_single_register(uint8_t device_id, uint16_t address, uint16_t value,
                        ResponseCallback cb);

  int
  write_multiple_coils(uint8_t device_id, uint16_t starting_address,
                       uint16_t quantity, const uint8_t *coil_data,
                       ResponseCallback cb);

  int
  write_multiple_registers(uint8_t device_id, uint16_t starting_address,
                           uint16_t quantity, const uint16_t *register_data,
                           ResponseCallback cb);

private:
  Stm32::Base::LoopCore &loop;
  Stm32::Base::Event response_timeout_event;
  ResponseCallback response_cb;
  bool expecting_response = false;
  uint8_t current_device_id = 0;
  uint8_t current_function_code = 0;
  uint8_t current_quantity = 0;
  bool sending_request = false;

  struct Stats
  {
    uint32_t responses = 0;
    uint32_t exceptions = 0;
    uint32_t crc_errors = 0;
    uint32_t timeouts = 0;
  };

  Modbus::Statistics<Stats> statistics;

  int
  send_request();

  int
  process_response();

  static void
  response_callback(void *context) noexcept;

  static void
  response_timeout_callback(void *context) noexcept;

  static void
  request_sent_callback(void *context, Uart::Status status) noexcept;
};

}; // namespace Embys::Stm32::Modbus::Rtu
