#include "handler.hpp"

#include <string.h>

#include "utils.hpp"

namespace Embys::Stm32::Modbus
{

Handler::Handler(StoreCore &store) : store(store)
{
}

uint8_t
Handler::handle(BufferIn request, uint16_t req_len, BufferOut response,
                uint16_t *res_len)
{
  response[0] = request[0]; // Device ID
  response[1] = request[1]; // Function code

  switch (request[1])
  {
    case FunctionCode::ReadCoils:
      return handle_read_coils(request, req_len, response, res_len);
    case FunctionCode::ReadDiscreteInputs:
      return handle_read_discrete_inputs(request, req_len, response, res_len);
    case FunctionCode::ReadHoldingRegisters:
      return handle_read_holding_registers(request, req_len, response, res_len);
    case FunctionCode::ReadInputRegisters:
      return handle_read_input_registers(request, req_len, response, res_len);
    case FunctionCode::WriteSingleCoil:
      return handle_write_single_coil(request, req_len, response, res_len);
    case FunctionCode::WriteSingleRegister:
      return handle_write_single_register(request, req_len, response, res_len);
    case FunctionCode::Diagnostics:
      return handle_diagnostics(request, req_len, response, res_len);
    case FunctionCode::ReportServerId:
      return handle_report_server_id(request, req_len, response, res_len);
    case FunctionCode::WriteMultipleCoils:
      return handle_write_multiple_coils(request, req_len, response, res_len);
    case FunctionCode::WriteMultipleRegisters:
      return handle_write_multiple_registers(request, req_len, response,
                                             res_len);
    default:
      return ExceptionCode::IllegalFunction;
  }
}

uint8_t
Handler::handle_read_coils(BufferIn request, uint16_t req_len,
                           BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address =
      static_cast<uint16_t>(read_u16_be(&request[2]) - coils_offset);
  quantity = read_u16_be(&request[4]);

  if (quantity == 0U || quantity > 2000U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = calculate_coil_bytes(quantity);

  if (static_cast<uint32_t>(response[2]) + 1U + kFrameHeaderSize > kFrameSize)
  {
    return ExceptionCode::IllegalDataValue;
  }

  memset(response + 3, 0, response[2]);
  if (store.get_coils(starting_address, response + 3, quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  *res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_read_discrete_inputs(BufferIn request, uint16_t req_len,
                                     BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address =
      static_cast<uint16_t>(read_u16_be(&request[2]) - discrete_inputs_offset);
  quantity = read_u16_be(&request[4]);

  if (quantity == 0U || quantity > 2000U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = calculate_coil_bytes(quantity);

  if (static_cast<uint32_t>(response[2]) + 1U + kFrameHeaderSize > kFrameSize)
  {
    return ExceptionCode::IllegalDataValue;
  }

  memset(response + 3, 0, response[2]);
  if (store.get_discrete_inputs(starting_address, response + 3, quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  *res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_read_holding_registers(BufferIn request, uint16_t req_len,
                                       BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(read_u16_be(&request[2]) -
                                           holding_registers_offset);
  quantity = read_u16_be(&request[4]);

  if (quantity == 0U || quantity > 125U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = static_cast<uint8_t>(quantity * 2U);

  if (static_cast<uint32_t>(response[2]) + 1U + kFrameHeaderSize > kFrameSize)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.get_holding_registers_be(starting_address, response + 3, quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  *res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_read_input_registers(BufferIn request, uint16_t req_len,
                                     BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address =
      static_cast<uint16_t>(read_u16_be(&request[2]) - input_registers_offset);
  quantity = read_u16_be(&request[4]);

  if (quantity == 0U || quantity > 125U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = static_cast<uint8_t>(quantity * 2U);

  if (static_cast<uint32_t>(response[2]) + 1U + kFrameHeaderSize > kFrameSize)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.get_input_registers_be(starting_address, response + 3, quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  *res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_write_single_coil(BufferIn request, uint16_t req_len,
                                  BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address =
      static_cast<uint16_t>(read_u16_be(&request[2]) - coils_offset);
  write_value = read_u16_be(&request[4]);

  if (write_value != 0x0000U && write_value != 0xFF00U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  bool coil_state = (write_value == 0xFF00U);

  if (store.set_coil(starting_address, coil_state))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(&response[2],
               static_cast<uint16_t>(starting_address + coils_offset));
  write_u16_be(&response[4], write_value);
  *res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_write_single_register(BufferIn request, uint16_t req_len,
                                      BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(read_u16_be(&request[2]) -
                                           holding_registers_offset);
  write_value = read_u16_be(&request[4]);

  if (store.set_holding_register(starting_address, write_value))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(&response[2], static_cast<uint16_t>(starting_address +
                                                   holding_registers_offset));
  write_u16_be(&response[4], write_value);
  *res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_write_multiple_coils(BufferIn request, uint16_t req_len,
                                     BufferOut response, uint16_t *res_len)
{
  if (req_len < kFrameHeaderSize + 6U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address =
      static_cast<uint16_t>(read_u16_be(&request[2]) - coils_offset);
  quantity = read_u16_be(&request[4]);
  uint8_t byte_count = request[6];

  if (req_len != kFrameHeaderSize + 5U + byte_count)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (quantity == 0U || quantity > 1968U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (byte_count != calculate_coil_bytes(quantity))
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.set_coils(starting_address, &request[7], quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(&response[2],
               static_cast<uint16_t>(starting_address + coils_offset));
  write_u16_be(&response[4], quantity);
  *res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_write_multiple_registers(BufferIn request, uint16_t req_len,
                                         BufferOut response, uint16_t *res_len)
{
  if (req_len < kFrameHeaderSize + 5U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(read_u16_be(&request[2]) -
                                           holding_registers_offset);
  quantity = read_u16_be(&request[4]);
  uint8_t byte_count = request[6];

  if (quantity == 0U || quantity > 123U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (byte_count != static_cast<uint8_t>(quantity * 2U))
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (req_len != kFrameHeaderSize + 5U + byte_count)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.set_holding_registers_be(starting_address, &request[7], quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(&response[2], static_cast<uint16_t>(starting_address +
                                                   holding_registers_offset));
  write_u16_be(&response[4], quantity);
  *res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_diagnostics(BufferIn request, uint16_t req_len,
                            BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  uint16_t sub_fn = read_u16_be(&request[2]);
  uint16_t data_field = read_u16_be(&request[4]);

  write_u16_be(&response[2], sub_fn);

  uint16_t counter_value = 0;

  switch (sub_fn)
  {
    case DiagnosticsSubCode::ReturnQueryData:
      write_u16_be(&response[4], data_field);
      break;
    case DiagnosticsSubCode::ClearCounters:
      if (diag_counters != nullptr)
      {
        diag_counters->reset();
      }
      write_u16_be(&response[4], 0x0000U);
      break;
    case DiagnosticsSubCode::ReturnBusMessageCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->bus_message_count : 0U;
      write_u16_be(&response[4], counter_value);
      break;
    case DiagnosticsSubCode::ReturnBusCommErrorCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->bus_comm_error_count : 0U;
      write_u16_be(&response[4], counter_value);
      break;
    case DiagnosticsSubCode::ReturnBusExceptionErrorCount:
      counter_value = (diag_counters != nullptr)
                          ? diag_counters->bus_exception_error_count
                          : 0U;
      write_u16_be(&response[4], counter_value);
      break;
    case DiagnosticsSubCode::ReturnSlaveMessageCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->slave_message_count : 0U;
      write_u16_be(&response[4], counter_value);
      break;
    case DiagnosticsSubCode::ReturnSlaveNoResponseCount:
      counter_value = (diag_counters != nullptr)
                          ? diag_counters->slave_no_response_count
                          : 0U;
      write_u16_be(&response[4], counter_value);
      break;
    case DiagnosticsSubCode::ReturnSlaveBusyCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->slave_busy_count : 0U;
      write_u16_be(&response[4], counter_value);
      break;
    default:
      return ExceptionCode::IllegalFunction;
  }

  *res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_report_server_id(BufferIn request, uint16_t req_len,
                                 BufferOut response, uint16_t *res_len)
{
  if (req_len != kFrameHeaderSize)
  {
    return ExceptionCode::IllegalDataValue;
  }

  static constexpr uint8_t kStatusIndicator = 0xFFU; // running

  response[2] = 2 + server_id_len; // byte count
  response[3] = request[0];        // device_id from request frame
  response[4] = kStatusIndicator;  // device status

  if (server_id_buf != nullptr)
  {
    for (uint8_t i = 0; i < server_id_len; ++i)
      response[5 + i] = server_id_buf[i];
  }

  *res_len = static_cast<uint16_t>(kFrameHeaderSize + 1U + response[2]);
  return 0;
}

}; // namespace Embys::Stm32::Modbus
