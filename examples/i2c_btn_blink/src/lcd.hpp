#pragma once

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c-hd44780/device.hpp>
#include <embys/stm32/i2c/bus.hpp>

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
  init()
  {
    schedule();
  }

  void
  set_blink_status(bool on)
  {
    pending.status = true;
    blink_on = on;

    if (!blink_on)
    {
      // If blinking is turned off, we should also clear the counter line
      pending.counter = true;
    }

    schedule();
  }

  void
  set_blink_count(int count)
  {
    pending.counter = true;
    blink_count = count;
    schedule();
  }

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
  schedule()
  {
    if (!busy)
    {
      dispatch(this, 0);
    }
  }

  void
  dispatch_pending()
  {
    Hd44780::Cb callback = {dispatch, this};

    if (pending.status)
    {
      pending.status = false;
      busy = true;
      const char *str =
          blink_on ? "Blink status: on    " : "Blink status: off   ";
      lcd.print_line(1, str, callback);
    }
    else if (pending.counter)
    {
      pending.counter = false;
      busy = true;

      if (blink_on)
      {
        write_counter(counter_buf, blink_count);
        lcd.print_line(2, counter_buf, callback);
      }
      else
      {
        lcd.clear_line(2, callback);
      }
    }
  }

  static void
  dispatch(void *ctx, int result)
  {
    auto *self = static_cast<Lcd *>(ctx);
    self->busy = false; // When a callback is invoked, the previous operation is
                        // complete, so we're no longer busy

    if (self->state == Error)
      return; // Don't attempt further operations if already in error state

    // If result is negative, transition to error state and store the error code
    if (result < 0)
    {
      self->state = Lcd::Error;
      self->error = result;
      return;
    }

    Hd44780::Cb callback = {dispatch, ctx};

    // Handle state transitions first
    switch (self->state)
    {
      case Idle:
        self->state = Initializing;
        self->busy = true;
        self->lcd.enable(callback);
        break;
      case Initializing:
        self->state = Ready;
        self->busy = true;
        self->lcd.print_line(0, "Blink App", callback);
        // Schedule an update to show initial status
        self->pending.status = true;
        break;
      case Ready:
        self->dispatch_pending();
        break;
      case Error:
        // Shouldn't reach here due to the early return
        break;
    }
  }
};

}; // namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
