#include "base.hpp"

#include <embys/stm32/def.hpp>
#include <embys/stm32/modbus/utils.hpp>

#include "diag.hpp"

namespace Embys::Stm32::Modbus::Rtu
{

Base::Base(Uart::BusCore &transport)
  : transport(transport),
    frame_timeout_event(*transport.get_base(),
                        ::Embys::Stm32::Base::EventMode::Deferred,
                        {Base::frame_timeout_callback, this})
{
  transport.set_rx_callback({Base::recv_callback, this});
  transport.set_tx_callback({Base::sent_callback, this});
}

Base::~Base()
{
  transport.clear_rx_callback();
}

void
Base::enable()
{
  calculate_timing();
}

int
Base::send_frame()
{
  append_crc(buffer_out, &buffer_out_len);
  TRY(transport.write(std::span{buffer_out}.first(buffer_out_len)));
  return 0;
}

uint16_t
Base::calculate_crc(const uint8_t *data, uint16_t len) const
{
  uint16_t crc = 0xFFFFU;

  for (uint16_t i = 0; i < len; i++)
  {
    crc ^= data[i];

    for (uint8_t j = 0U; j < 8U; j++)
    {
      if ((crc & 0x0001U) != 0U)
      {
        crc = static_cast<uint16_t>((crc >> 1U) ^ 0xA001U);
      }
      else
      {
        crc >>= 1U;
      }
    }
  }

  return crc;
}

void
Base::append_crc(uint8_t *buffer, uint16_t *len)
{
  uint16_t crc = calculate_crc(buffer, *len);
  // Modbus RTU CRC is little-endian: low byte first, then high byte
  buffer[*len] = static_cast<uint8_t>(crc & 0xFFU);
  buffer[*len + 1U] = static_cast<uint8_t>((crc >> 8U) & 0xFFU);
  *len += 2U;
}

bool
Base::validate_crc(const uint8_t *buffer, uint16_t len) const
{
  // Modbus RTU CRC is little-endian: low byte first, then high byte
  uint16_t received_crc = static_cast<uint16_t>(buffer[len - 2U]) |
                          (static_cast<uint16_t>(buffer[len - 1U]) << 8U);
  uint16_t calculated_crc =
      calculate_crc(buffer, static_cast<uint16_t>(len - 2U));
  return received_crc == calculated_crc;
}

void
Base::calculate_timing()
{
  uint32_t baud_rate = transport.get_baud_rate();

  if (baud_rate == 0U)
  {
    // Transport not yet enabled; timing will be wrong — caller should enable()
    // after enabling the bus
    baud_rate = 9600U;
  }

  if (baud_rate > 19200U)
  {
    // Per Modbus spec: for baud rates > 19200 use fixed 1750 µs
    frame_delay_us = 1750U;
  }
  else
  {
    uint32_t frame_bits = transport.get_frame_bits();
    uint32_t char_time_us = (frame_bits * 1000000U) / baud_rate;
    frame_delay_us = (char_time_us * 7U) >> 1U; // 3.5 character times
  }
}

void
Base::recv_callback(void *context, uint8_t byte)
{
  auto *rtu = static_cast<Base *>(context);

  if (!rtu->frame_in_receiving)
  {
    rtu->buffer_in_len = 0;
    rtu->frame_in_receiving = true;
  }

  if (rtu->buffer_in_len < sizeof(rtu->buffer_in))
  {
    rtu->buffer_in[rtu->buffer_in_len] = byte;
    ++rtu->buffer_in_len;
  }
  else
  {
    rtu->buffer_in_overflow = true;
  }

  rtu->frame_timeout_event.enable(
      std::chrono::microseconds{rtu->frame_delay_us});
}

void
Base::frame_timeout_callback(void *context)
{
  auto *rtu = static_cast<Base *>(context);

  rtu->frame_in_receiving = false;

  if (rtu->buffer_in_overflow)
  {
    rtu->buffer_in_overflow = false;
    return;
  }

  if (rtu->buffer_in_len <
      Modbus::kFrameHeaderSize + static_cast<uint16_t>(sizeof(uint16_t)))
  {
    return;
  }

  rtu->frame_in_cb();
}

void
Base::sent_callback(void *context, int status)
{
  auto *rtu = static_cast<Base *>(context);
  rtu->frame_out_cb(status);
}

}; // namespace Embys::Stm32::Modbus::Rtu
