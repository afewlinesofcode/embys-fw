#pragma once

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c-hd44780/device.hpp>
#include <embys/stm32/i2c/bus.hpp>
#include <embys/stm32/types.hpp>

#include "sim.hpp"
#include "util.hpp"

namespace Embys::Stm32::I2c::Dev::I2cAht20
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

  Lcd(Embys::Stm32::Base::Loop *loop, Embys::Stm32::I2c::Bus *i2c_bus)
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
  set_ready_cb(Embys::Callback<int> cb);

  void
  set_unavailable();

  void
  set_values(float temperature, float humidity);

private:
  State state = Idle;
  int error = 0;

  struct PendingUpdate
  {
    bool unavailable = false;
    bool temperature = false;
    bool humidity = false;
  } pending;

  bool busy = false;

  Embys::Callback<int> ready_cb;
  float temperature = 0.0f;
  float humidity = 0.0f;

  Embys::Stm32::I2c::Dev::Hd44780::Device lcd;
  char line_buf[21];

  void
  schedule();

  void
  dispatch_pending();

  static void
  dispatch(void *ctx, int result);
};

}; // namespace Embys::Stm32::I2c::Dev::I2cAht20
