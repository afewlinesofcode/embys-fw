#include "lcd.hpp"

namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
{

void
Lcd::init()
{
  schedule();
}

void
Lcd::set_blink_status(bool on)
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
Lcd::set_blink_count(int count)
{
  pending.counter = true;
  blink_count = count;
  schedule();
}

void
Lcd::schedule()
{
  if (!busy)
  {
    dispatch(this, 0);
  }
}

void
Lcd::dispatch_pending()
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

void
Lcd::dispatch(void *ctx, int result)
{
  auto *self = static_cast<Lcd *>(ctx);
  self->busy = false; // When a callback is invoked, the previous operation is
                      // complete, so we're no longer busy

  if (self->state == Error)
    return; // Don't attempt further operations if already in error state

  // If result is negative, transition to error state and store the error code
  if (result < 0)
  {
    SIM_LOG("LCD error: " << std::dec << result << " on state " << self->state);
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

}; // namespace Embys::Stm32::I2c::Dev::I2cBtnBlink
