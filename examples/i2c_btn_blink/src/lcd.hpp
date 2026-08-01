#pragma once

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c-hd44780/device.hpp>
#include <embys/stm32/i2c/bus.hpp>

#include "sim.hpp"
#include "util.hpp"

namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
{

class Lcd
{
public:
  enum State
  {
    Idle,
    Initializing,
    Ready,
    Error
  };

  Lcd() = delete;
  Lcd(const Lcd &) = delete;
  Lcd(Lcd &&) = delete;
  Lcd &
  operator=(const Lcd &) = delete;
  Lcd &
  operator=(Lcd &&) = delete;

  Lcd(Embys::Stm32::Base::LoopCore *loop, Embys::Stm32::I2c::Bus *i2c_bus)
    : lcd(loop, i2c_bus) {};

  inline State
  get_state() const
  {
    return state;
  }

  inline int
  get_error() const
  {
    return error;
  }

  void
  init();

  void
  set_blink_status(bool on);

  void
  set_blink_count(int count);

private:
  State state = Idle;
  int error = 0;
  bool blink_on = false;
  int blink_count = 0;

  struct PendingUpdate
  {
    bool status = false;
    bool counter = false;
  } pending;
  bool busy = false;

  Embys::Stm32::I2c::Dev::Hd44780::Device lcd;
  char counter_buf[21];

  void
  schedule();

  void
  dispatch_pending();

  static void
  dispatch(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
