#pragma once

#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c-hd44780/device.hpp>
#include <embys/stm32/i2c/bus.hpp>

#include "sim.hpp"

namespace ModbusRtuServer
{

class Lcd
{
public:
  Lcd() = delete;
  Lcd(const Lcd &) = delete;
  Lcd(Lcd &&) = delete;
  Lcd &
  operator=(const Lcd &) = delete;
  Lcd &
  operator=(Lcd &&) = delete;

  Lcd(Embys::Stm32::Base::LoopCore &loop, Embys::Stm32::I2c::BusCore &i2c_bus)
    : lcd(loop, i2c_bus)
  {
  }

  void
  init();

  void
  show_operation(char op, const char *type, uint16_t addr);

private:
  enum State
  {
    Idle,
    Initializing,
    Ready,
    Error
  } state = Idle;

  bool busy = false;
  char op_line[21] = {};

  struct PendingUpdate
  {
    bool op_line = false;
  } pending;

  Embys::Stm32::I2c::Dev::Hd44780::Device lcd;

  void
  schedule();

  void
  dispatch_pending();

  static void
  dispatch(void *ctx, int result) noexcept;
};

}; // namespace ModbusRtuServer
