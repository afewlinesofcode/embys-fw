#include "client.hpp"

#include <embys/stm32/def.hpp>
#include <embys/stm32/modbus/utils.hpp>

#include "diag.hpp"

namespace Embys::Stm32::Modbus::Rtu
{

Client::Client(Uart::BusCore &transport, Stm32::Base::LoopCore &loop)
  : Base(transport), loop(loop),
    response_timeout_event(loop, ::Embys::Stm32::Base::EventMode::Deferred,
                           {Client::response_timeout_callback, this})
{
  set_frame_in_callback({Client::response_callback, this});
  transport.set_tx_callback({Client::request_sent_callback, this});
}

Client::~Client()
{
  transport.clear_tx_callback();
}

void
Client::enable()
{
  Base::enable();
}

int
Client::read_coils(uint8_t device_id, uint16_t starting_address,
                   uint16_t quantity, ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::ReadCoils;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), starting_address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U), quantity);
  buffer_out_len = Modbus::kFrameHeaderSize + 4U;
  response_cb = cb;
  current_quantity = static_cast<uint8_t>(quantity);
  return send_request();
}

int
Client::read_discrete_inputs(uint8_t device_id, uint16_t starting_address,
                             uint16_t quantity, ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::ReadDiscreteInputs;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), starting_address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U), quantity);
  buffer_out_len = Modbus::kFrameHeaderSize + 4U;
  response_cb = cb;
  current_quantity = static_cast<uint8_t>(quantity);
  return send_request();
}

int
Client::read_holding_registers(uint8_t device_id, uint16_t starting_address,
                               uint16_t quantity, ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::ReadHoldingRegisters;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), starting_address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U), quantity);
  buffer_out_len = Modbus::kFrameHeaderSize + 4U;
  response_cb = cb;
  current_quantity = static_cast<uint8_t>(quantity);
  return send_request();
}

int
Client::read_input_registers(uint8_t device_id, uint16_t starting_address,
                             uint16_t quantity, ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::ReadInputRegisters;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), starting_address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U), quantity);
  buffer_out_len = Modbus::kFrameHeaderSize + 4U;
  response_cb = cb;
  current_quantity = static_cast<uint8_t>(quantity);
  return send_request();
}

int
Client::write_single_coil(uint8_t device_id, uint16_t address, bool value,
                          ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::WriteSingleCoil;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U),
                       value ? 0xFF00U : 0x0000U);
  buffer_out_len = Modbus::kFrameHeaderSize + 4U;
  response_cb = cb;
  current_quantity = 2U;
  return send_request();
}

int
Client::write_single_register(uint8_t device_id, uint16_t address,
                              uint16_t value, ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::WriteSingleRegister;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U), value);
  buffer_out_len = Modbus::kFrameHeaderSize + 4U;
  response_cb = cb;
  current_quantity = 2U;
  return send_request();
}

int
Client::write_multiple_coils(uint8_t device_id, uint16_t starting_address,
                             uint16_t quantity, const uint8_t *coil_data,
                             ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::WriteMultipleCoils;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), starting_address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U), quantity);
  buffer_out[6] = Modbus::calculate_coil_bytes(quantity);
  for (uint8_t i = 0; i < buffer_out[6]; i++)
  {
    buffer_out[7U + i] = coil_data[i];
  }
  buffer_out_len =
      static_cast<uint16_t>(Modbus::kFrameHeaderSize + 5U + buffer_out[6]);
  response_cb = cb;
  current_quantity = 2U;
  return send_request();
}

int
Client::write_multiple_registers(uint8_t device_id, uint16_t starting_address,
                                 uint16_t quantity,
                                 const uint16_t *register_data,
                                 ResponseCallback cb)
{
  buffer_out[0] = device_id;
  buffer_out[1] = Modbus::FunctionCode::WriteMultipleRegisters;
  Modbus::write_u16_be(std::span{buffer_out}.subspan(2U, 2U), starting_address);
  Modbus::write_u16_be(std::span{buffer_out}.subspan(4U, 2U), quantity);
  buffer_out[6] = static_cast<uint8_t>(quantity * 2U);
  for (uint16_t i = 0; i < quantity; i++)
  {
    Modbus::write_u16_be(std::span{buffer_out}.subspan(7U + (i * 2U), 2U),
                         register_data[i]);
  }
  buffer_out_len =
      static_cast<uint16_t>(Modbus::kFrameHeaderSize + 5U + buffer_out[6]);
  response_cb = cb;
  current_quantity = 2U;
  return send_request();
}

int
Client::send_request()
{
  if (expecting_response && current_device_id != 0U)
  {
    return Diag::EXPECTING_RESPONSE;
  }

  if (sending_request)
  {
    return Diag::SEND_IN_PROGRESS;
  }

  TRY(send_frame());
  sending_request = true;
  current_device_id = buffer_out[0];
  current_function_code = buffer_out[1];

  if (current_device_id != 0U)
  {
    expecting_response = true;
    (void)response_timeout_event.enable(
        std::chrono::microseconds{kRequestTimeoutUs});
  }

  return 0;
}

int
Client::process_response()
{
  if (buffer_in_len < Modbus::kFrameHeaderSize)
  {
    return 0;
  }

  if (!validate_crc(buffer_in, buffer_in_len))
  {
    statistics.increment(&Stats::crc_errors);
    return 0;
  }

  if (buffer_in[0] != current_device_id ||
      buffer_in[1] != current_function_code)
  {
    return 0;
  }

  expecting_response = false;
  response_timeout_event.disable();
  statistics.increment(&Stats::responses);

  switch (buffer_in[1])
  {
    case Modbus::FunctionCode::ReadCoils:
    case Modbus::FunctionCode::ReadDiscreteInputs:
      if (Modbus::calculate_coil_bytes(current_quantity) != buffer_in[2])
      {
        return 0;
      }
      response_cb(buffer_in[0], buffer_in[1], current_quantity, &buffer_in[3]);
      break;

    case Modbus::FunctionCode::ReadHoldingRegisters:
    case Modbus::FunctionCode::ReadInputRegisters:
      if (static_cast<uint8_t>(current_quantity << 1U) != buffer_in[2])
      {
        return 0;
      }
      response_cb(buffer_in[0], buffer_in[1], current_quantity, &buffer_in[3]);
      break;

    case Modbus::FunctionCode::WriteSingleCoil:
    case Modbus::FunctionCode::WriteSingleRegister:
    case Modbus::FunctionCode::WriteMultipleCoils:
    case Modbus::FunctionCode::WriteMultipleRegisters:
      response_cb(buffer_in[0], buffer_in[1], current_quantity, &buffer_in[2]);
      break;

    default:
      // Exception response (FC | 0x80)
      if ((buffer_in[1] & 0x80U) != 0U)
      {
        statistics.increment(&Stats::exceptions);
        response_cb(buffer_in[0], buffer_in[1], 0U, &buffer_in[2]);
      }
      break;
  }

  return 0;
}

void
Client::response_callback(void *context) noexcept
{
  auto *client = static_cast<Client *>(context);

  if (!client->expecting_response)
  {
    return;
  }

  client->process_response();
}

void
Client::response_timeout_callback(void *context) noexcept
{
  auto *client = static_cast<Client *>(context);

  if (!client->expecting_response)
  {
    return;
  }

  client->expecting_response = false;
  client->statistics.increment(&Client::Stats::timeouts);
  client->response_cb(client->current_device_id, client->current_function_code,
                      0U, nullptr);
}

void
Client::request_sent_callback(void *context, int status) noexcept
{
  auto *client = static_cast<Client *>(context);
  client->sending_request = false;

  (void)status;
}

}; // namespace Embys::Stm32::Modbus::Rtu
