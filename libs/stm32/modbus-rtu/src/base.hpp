/**
 * @file base.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Modbus RTU base: frame detection, CRC, and character-time timing
 *
 * Base provides the common RTU framing layer shared by both Client and Server.
 * It owns the RX/TX byte buffers, drives the 3.5-character-time frame-timeout
 * event, and exposes protected callback hooks for subclasses.
 *
 * The caller wires the UART Bus externally and calls enable() after the bus
 * has been enabled (so that get_baud_rate() returns a valid value).
 *
 * Base events required: 1 (frame_timeout_event)
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/base/event.hpp>
#include <embys/stm32/modbus/def.hpp>
#include <embys/stm32/types.hpp>
#include <embys/stm32/uart/bus.hpp>

namespace Embys::Stm32::Modbus::Rtu
{

class Base
{
public:
  Base() = delete;
  Base(const Base &) = delete;
  Base(Base &&) = delete;
  Base &
  operator=(const Base &) = delete;
  Base &
  operator=(Base &&) = delete;

  explicit Base(Uart::Bus *transport);
  ~Base();

  void
  enable();

  inline void
  override_frame_delay_us(uint32_t delay_us)
  {
    frame_delay_us = delay_us;
  }

protected:
  Uart::Bus *transport;
  Stm32::Base::Event frame_timeout_event;

  uint32_t frame_delay_us = 0;

  uint8_t buffer_in[Modbus::kFrameSize];
  uint16_t buffer_in_len = 0;

  uint8_t buffer_out[Modbus::kFrameSize];
  uint16_t buffer_out_len = 0;

  Embys::Callback<> frame_in_cb;
  Embys::Callback<int> frame_out_cb;

  bool buffer_in_overflow = false;
  bool frame_in_receiving = false;

  inline void
  set_frame_in_callback(Embys::Callback<> cb)
  {
    frame_in_cb = cb;
  }

  inline void
  set_frame_out_callback(Embys::Callback<int> cb)
  {
    frame_out_cb = cb;
  }

  int
  send_frame();

  uint16_t
  calculate_crc(const uint8_t *data, uint16_t len) const;

  void
  append_crc(uint8_t *buffer, uint16_t *len);

  bool
  validate_crc(const uint8_t *buffer, uint16_t len) const;

  void
  calculate_timing();

private:
  static void
  recv_callback(void *context, uint8_t byte);

  static void
  frame_timeout_callback(void *context);

  static void
  sent_callback(void *context, int status);
};

}; // namespace Embys::Stm32::Modbus::Rtu
