#include "lcd.hpp"

namespace Embys::Stm32::I2c::Dev::I2cAht20
{

void
Lcd::init()
{
  schedule();
}

void
Lcd::set_ready_cb(Embys::Callback<int> cb)
{
  ready_cb = cb;
}

void
Lcd::set_unavailable()
{
  pending.unavailable = true;
  pending.temperature = false;
  pending.humidity = false;
  schedule();
}

void
Lcd::set_values(Aht20::Temperature temp, Aht20::RelativeHumidity hum)
{
  temperature = temp;
  humidity = hum;
  pending.unavailable = false;
  pending.temperature = true;
  pending.humidity = true;
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

  if (pending.unavailable)
  {
    pending.unavailable = false;
    busy = true;
    lcd.print_line(1, "Unavailable         ", callback);
  }
  else if (pending.temperature)
  {
    pending.temperature = false;
    busy = true;
    write_temperature(line_buf, temperature);
    lcd.print_line(1, line_buf, callback);
  }
  else if (pending.humidity)
  {
    pending.humidity = false;
    busy = true;
    write_humidity(line_buf, humidity);
    lcd.print_line(2, line_buf, callback);
  }
  else if (!ready_cb.empty())
  {
    Embys::Callback<int> cb = ready_cb;
    ready_cb.clear();
    cb(0);
  }
}

void
Lcd::dispatch(void *ctx, int result) noexcept
{
  auto *self = static_cast<Lcd *>(ctx);
  self->busy = false;

  if (self->state == Error)
    return;

  if (result < 0)
  {
    SIM_LOG("LCD error: " << std::dec << result << " on state " << self->state);
    self->state = Lcd::Error;
    self->error = result;
    return;
  }

  Hd44780::Cb callback = {dispatch, ctx};

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
      self->lcd.print_line(0, "AHT20 App", callback);
      break;
    case Ready:
      self->dispatch_pending();
      break;
    case Error:
      break;
  }
}

}; // namespace Embys::Stm32::I2c::Dev::I2cAht20
