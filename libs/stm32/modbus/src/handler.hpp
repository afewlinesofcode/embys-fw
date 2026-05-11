/**
 * @file handler.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus protocol request handler
 *
 * Handler processes a raw Modbus PDU (device-ID + function-code + data,
 * without CRC) and writes the corresponding response PDU into a caller-provided
 * output buffer.  It returns 0 on success or a Modbus exception code (uint8_t)
 * on failure.
 *
 * Address offsets let the caller remap the on-wire addresses to a different
 * base within the Store (useful when multiple Modbus servers share one store).
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include "def.hpp"
#include "diagnostics.hpp"
#include "store.hpp"

namespace Embys::Stm32::Modbus
{

class Handler
{
public:
  using BufferIn = const uint8_t *;
  using BufferOut = uint8_t *;

  Handler() = delete;
  Handler(const Handler &) = delete;
  Handler(Handler &&) = delete;
  Handler &
  operator=(const Handler &) = delete;
  Handler &
  operator=(Handler &&) = delete;

  explicit Handler(Store *store);

  inline Store *
  get_store() const
  {
    return store;
  }

  /**
   * @brief Process a Modbus request and build a response.
   *
   * @param request  Pointer to the request PDU (device ID + FC + data).
   * @param req_len  Length of the request PDU.
   * @param response Pointer to the response buffer.
   * @param res_len  Written with the length of the response PDU on success.
   * @return 0 on success, or a Modbus ExceptionCode on failure.
   */
  uint8_t
  handle(BufferIn request, uint16_t req_len, BufferOut response,
         uint16_t *res_len);

  inline void
  set_coils_offset(uint16_t offset)
  {
    coils_offset = offset;
  }

  inline void
  set_discrete_inputs_offset(uint16_t offset)
  {
    discrete_inputs_offset = offset;
  }

  inline void
  set_holding_registers_offset(uint16_t offset)
  {
    holding_registers_offset = offset;
  }

  inline void
  set_input_registers_offset(uint16_t offset)
  {
    input_registers_offset = offset;
  }

  inline void
  set_diagnostics_counters(DiagnosticsCounters *ptr)
  {
    diag_counters = ptr;
  }

  inline void
  set_server_id(const uint8_t *buf, uint8_t len)
  {
    server_id_buf = buf;
    server_id_len = len;
  }

private:
  Store *store;
  DiagnosticsCounters *diag_counters = nullptr;
  const uint8_t *server_id_buf = nullptr;
  uint8_t server_id_len = 0;
  uint16_t starting_address = 0;
  uint16_t quantity = 0;
  uint16_t write_value = 0;
  uint16_t coils_offset = 0;
  uint16_t discrete_inputs_offset = 0;
  uint16_t holding_registers_offset = 0;
  uint16_t input_registers_offset = 0;

  uint8_t
  handle_read_coils(BufferIn request, uint16_t req_len, BufferOut response,
                    uint16_t *res_len);

  uint8_t
  handle_read_discrete_inputs(BufferIn request, uint16_t req_len,
                              BufferOut response, uint16_t *res_len);

  uint8_t
  handle_read_holding_registers(BufferIn request, uint16_t req_len,
                                BufferOut response, uint16_t *res_len);

  uint8_t
  handle_read_input_registers(BufferIn request, uint16_t req_len,
                              BufferOut response, uint16_t *res_len);

  uint8_t
  handle_write_single_coil(BufferIn request, uint16_t req_len,
                           BufferOut response, uint16_t *res_len);

  uint8_t
  handle_write_single_register(BufferIn request, uint16_t req_len,
                               BufferOut response, uint16_t *res_len);

  uint8_t
  handle_diagnostics(BufferIn request, uint16_t req_len, BufferOut response,
                     uint16_t *res_len);

  uint8_t
  handle_report_server_id(BufferIn request, uint16_t req_len,
                          BufferOut response, uint16_t *res_len);

  uint8_t
  handle_write_multiple_coils(BufferIn request, uint16_t req_len,
                              BufferOut response, uint16_t *res_len);

  uint8_t
  handle_write_multiple_registers(BufferIn request, uint16_t req_len,
                                  BufferOut response, uint16_t *res_len);
};

}; // namespace Embys::Stm32::Modbus
