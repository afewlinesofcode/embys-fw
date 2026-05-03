#include "server.hpp"

#include <embys/stm32/def.hpp>

namespace Embys::Stm32::Modbus::Rtu
{

Server::Server(uint8_t device_id, Modbus::Handler *handler,
               Uart::Bus *transport)
  : Base(transport), device_id(device_id), handler(handler)
{
}

Server::~Server()
{
}

void
Server::enable()
{
  set_frame_in_callback({Server::request_callback, this});
  set_frame_out_callback({Server::response_sent_callback, this});
  Base::enable();
}

int
Server::process_request()
{
  if (buffer_in_len < Modbus::kFrameHeaderSize)
  {
    return 0;
  }

  if (buffer_in[0] != device_id && buffer_in[0] != 0U)
  {
    return 0;
  }

  if (!validate_crc(buffer_in, buffer_in_len))
  {
    statistics.inc_crc_errors();
    return 0;
  }

  buffer_in_len -= 2U; // strip CRC

  handling_request = true;
  uint8_t exception =
      handler->handle(buffer_in, buffer_in_len, buffer_out, &buffer_out_len);

  if (buffer_in[0] == 0U)
  {
    // Broadcast — no response
    handling_request = false;
    return 0;
  }

  if (exception != 0U)
  {
    return send_exception(exception);
  }

  return send_response();
}

int
Server::send_response()
{
  TRY(send_frame());
  statistics.inc_responses();
  return 0;
}

int
Server::send_exception(uint8_t exception_code)
{
  buffer_out[0] = buffer_in[0];
  buffer_out[1] = buffer_in[1] | 0x80U;
  buffer_out[2] = exception_code;
  buffer_out_len = Modbus::kFrameHeaderSize + 1U;
  statistics.inc_exceptions();
  return send_response();
}

void
Server::request_callback(void *context)
{
  auto *srv = static_cast<Server *>(context);
  srv->process_request();
}

void
Server::response_sent_callback(void *context, int status)
{
  auto *srv = static_cast<Server *>(context);
  srv->handling_request = false;

  (void)status;
}

}; // namespace Embys::Stm32::Modbus::Rtu
