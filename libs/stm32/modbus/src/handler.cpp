#include "handler.hpp"

#include "utils.hpp"

namespace Embys::Stm32::Modbus
{

Handler::Handler(StoreCore &store) : store(store)
{
}

uint8_t
Handler::handle(BufferIn request, BufferOut response, uint16_t &res_len)
{
  if (request.size() < kFrameHeaderSize ||
      response.size() < kFrameHeaderSize + 1U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[0] = request[0]; // Device ID
  response[1] = request[1]; // Function code

  switch (request[1])
  {
    case FunctionCode::ReadCoils:
      return handle_read_coils(request, response, res_len);
    case FunctionCode::ReadDiscreteInputs:
      return handle_read_discrete_inputs(request, response, res_len);
    case FunctionCode::ReadHoldingRegisters:
      return handle_read_holding_registers(request, response, res_len);
    case FunctionCode::ReadInputRegisters:
      return handle_read_input_registers(request, response, res_len);
    case FunctionCode::WriteSingleCoil:
      return handle_write_single_coil(request, response, res_len);
    case FunctionCode::WriteSingleRegister:
      return handle_write_single_register(request, response, res_len);
    case FunctionCode::Diagnostics:
      return handle_diagnostics(request, response, res_len);
    case FunctionCode::ReportServerId:
      return handle_report_server_id(request, response, res_len);
    case FunctionCode::WriteMultipleCoils:
      return handle_write_multiple_coils(request, response, res_len);
    case FunctionCode::WriteMultipleRegisters:
      return handle_write_multiple_registers(request, response, res_len);
    default:
      return ExceptionCode::IllegalFunction;
  }
}

uint8_t
Handler::handle_read_coils(BufferIn request, BufferOut response,
                           uint16_t &res_len)
{
  if (request.size() != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - coils_offset);
  quantity = read_u16_be(request.subspan(4U, 2U));

  if (quantity == 0U || quantity > 2000U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = calculate_coil_bytes(quantity);

  if (static_cast<std::size_t>(response[2]) + 1U + kFrameHeaderSize >
      response.size())
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.get_coils(starting_address, response.subspan(3U, response[2]),
                      quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_read_discrete_inputs(BufferIn request, BufferOut response,
                                     uint16_t &res_len)
{
  if (request.size() != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - discrete_inputs_offset);
  quantity = read_u16_be(request.subspan(4U, 2U));

  if (quantity == 0U || quantity > 2000U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = calculate_coil_bytes(quantity);

  if (static_cast<std::size_t>(response[2]) + 1U + kFrameHeaderSize >
      response.size())
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.get_discrete_inputs(starting_address,
                                response.subspan(3U, response[2]), quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_read_holding_registers(BufferIn request, BufferOut response,
                                       uint16_t &res_len)
{
  if (request.size() != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - holding_registers_offset);
  quantity = read_u16_be(request.subspan(4U, 2U));

  if (quantity == 0U || quantity > 125U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = static_cast<uint8_t>(quantity * 2U);

  if (static_cast<std::size_t>(response[2]) + 1U + kFrameHeaderSize >
      response.size())
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.get_holding_registers_be(starting_address,
                                     response.subspan(3U, response[2])))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_read_input_registers(BufferIn request, BufferOut response,
                                     uint16_t &res_len)
{
  if (request.size() != kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - input_registers_offset);
  quantity = read_u16_be(request.subspan(4U, 2U));

  if (quantity == 0U || quantity > 125U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  response[2] = static_cast<uint8_t>(quantity * 2U);

  if (static_cast<std::size_t>(response[2]) + 1U + kFrameHeaderSize >
      response.size())
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.get_input_registers_be(starting_address,
                                   response.subspan(3U, response[2])))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  res_len = static_cast<uint16_t>(kFrameHeaderSize + response[2] + 1U);
  return 0;
}

uint8_t
Handler::handle_write_single_coil(BufferIn request, BufferOut response,
                                  uint16_t &res_len)
{
  if (request.size() != kFrameHeaderSize + 4U ||
      response.size() < kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - coils_offset);
  write_value = read_u16_be(request.subspan(4U, 2U));

  if (write_value != 0x0000U && write_value != 0xFF00U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  bool coil_state = (write_value == 0xFF00U);

  if (store.set_coil(starting_address, coil_state))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(response.subspan(2U, 2U),
               static_cast<uint16_t>(starting_address + coils_offset));
  write_u16_be(response.subspan(4U, 2U), write_value);
  res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_write_single_register(BufferIn request, BufferOut response,
                                      uint16_t &res_len)
{
  if (request.size() != kFrameHeaderSize + 4U ||
      response.size() < kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - holding_registers_offset);
  write_value = read_u16_be(request.subspan(4U, 2U));

  if (store.set_holding_register(starting_address, write_value))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(
      response.subspan(2U, 2U),
      static_cast<uint16_t>(starting_address + holding_registers_offset));
  write_u16_be(response.subspan(4U, 2U), write_value);
  res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_write_multiple_coils(BufferIn request, BufferOut response,
                                     uint16_t &res_len)
{
  if (request.size() < kFrameHeaderSize + 6U ||
      response.size() < kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - coils_offset);
  quantity = read_u16_be(request.subspan(4U, 2U));
  uint8_t byte_count = request[6];

  if (request.size() != kFrameHeaderSize + 5U + byte_count)
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

  if (store.set_coils(starting_address, request.subspan(7U, byte_count),
                      quantity))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(response.subspan(2U, 2U),
               static_cast<uint16_t>(starting_address + coils_offset));
  write_u16_be(response.subspan(4U, 2U), quantity);
  res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_write_multiple_registers(BufferIn request, BufferOut response,
                                         uint16_t &res_len)
{
  if (request.size() < kFrameHeaderSize + 5U ||
      response.size() < kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  starting_address = static_cast<uint16_t>(
      read_u16_be(request.subspan(2U, 2U)) - holding_registers_offset);
  quantity = read_u16_be(request.subspan(4U, 2U));
  uint8_t byte_count = request[6];

  if (quantity == 0U || quantity > 123U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (byte_count != static_cast<uint8_t>(quantity * 2U))
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (request.size() != kFrameHeaderSize + 5U + byte_count)
  {
    return ExceptionCode::IllegalDataValue;
  }

  if (store.set_holding_registers_be(starting_address,
                                     request.subspan(7U, byte_count)))
  {
    return ExceptionCode::IllegalDataAddress;
  }

  write_u16_be(
      response.subspan(2U, 2U),
      static_cast<uint16_t>(starting_address + holding_registers_offset));
  write_u16_be(response.subspan(4U, 2U), quantity);
  res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_diagnostics(BufferIn request, BufferOut response,
                            uint16_t &res_len)
{
  if (request.size() != kFrameHeaderSize + 4U ||
      response.size() < kFrameHeaderSize + 4U)
  {
    return ExceptionCode::IllegalDataValue;
  }

  uint16_t sub_fn = read_u16_be(request.subspan(2U, 2U));
  uint16_t data_field = read_u16_be(request.subspan(4U, 2U));

  write_u16_be(response.subspan(2U, 2U), sub_fn);

  uint16_t counter_value = 0;

  switch (sub_fn)
  {
    case DiagnosticsSubCode::ReturnQueryData:
      write_u16_be(response.subspan(4U, 2U), data_field);
      break;
    case DiagnosticsSubCode::ClearCounters:
      if (diag_counters != nullptr)
      {
        diag_counters->reset();
      }
      write_u16_be(response.subspan(4U, 2U), 0x0000U);
      break;
    case DiagnosticsSubCode::ReturnBusMessageCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->bus_message_count : 0U;
      write_u16_be(response.subspan(4U, 2U), counter_value);
      break;
    case DiagnosticsSubCode::ReturnBusCommErrorCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->bus_comm_error_count : 0U;
      write_u16_be(response.subspan(4U, 2U), counter_value);
      break;
    case DiagnosticsSubCode::ReturnBusExceptionErrorCount:
      counter_value = (diag_counters != nullptr)
                          ? diag_counters->bus_exception_error_count
                          : 0U;
      write_u16_be(response.subspan(4U, 2U), counter_value);
      break;
    case DiagnosticsSubCode::ReturnSlaveMessageCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->slave_message_count : 0U;
      write_u16_be(response.subspan(4U, 2U), counter_value);
      break;
    case DiagnosticsSubCode::ReturnSlaveNoResponseCount:
      counter_value = (diag_counters != nullptr)
                          ? diag_counters->slave_no_response_count
                          : 0U;
      write_u16_be(response.subspan(4U, 2U), counter_value);
      break;
    case DiagnosticsSubCode::ReturnSlaveBusyCount:
      counter_value =
          (diag_counters != nullptr) ? diag_counters->slave_busy_count : 0U;
      write_u16_be(response.subspan(4U, 2U), counter_value);
      break;
    default:
      return ExceptionCode::IllegalFunction;
  }

  res_len = kFrameHeaderSize + 4U;
  return 0;
}

uint8_t
Handler::handle_report_server_id(BufferIn request, BufferOut response,
                                 uint16_t &res_len)
{
  const std::size_t response_size = kFrameHeaderSize + 3U + server_id.size();
  if (request.size() != kFrameHeaderSize || response_size > response.size() ||
      response_size > kFrameSize)
  {
    return ExceptionCode::IllegalDataValue;
  }

  static constexpr uint8_t kStatusIndicator = 0xFFU; // running

  response[2] = static_cast<uint8_t>(2U + server_id.size()); // byte count
  response[3] = request[0];                                  // device ID
  response[4] = kStatusIndicator;                            // device status

  for (std::size_t i = 0; i < server_id.size(); ++i)
  {
    response[5U + i] = server_id[i];
  }

  res_len = static_cast<uint16_t>(kFrameHeaderSize + 1U + response[2]);
  return 0;
}

}; // namespace Embys::Stm32::Modbus
