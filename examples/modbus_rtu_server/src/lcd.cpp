#include "lcd.hpp"

#include "util.hpp"

namespace ModbusRtuServer
{

void
Lcd::init()
{
  schedule();
}

void
Lcd::show_operation(char op, const char *type, uint16_t addr)
{
  write_operation(op_line, op, type, addr);
  pending.op_line = true;
  schedule();
}

void
Lcd::schedule()
{
  if (!busy)
    dispatch(this, 0);
}

void
Lcd::dispatch_pending()
{
  using Cb = Embys::Stm32::I2c::Dev::Hd44780::Cb;
  Cb callback = {dispatch, this};

  if (pending.op_line)
  {
    pending.op_line = false;
    busy = true;
    lcd.print_line(1, op_line, callback);
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
    SIM_LOG("LCD error: " << result);
    self->state = Error;
    return;
  }

  using Cb = Embys::Stm32::I2c::Dev::Hd44780::Cb;
  Cb callback = {dispatch, ctx};

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
      self->lcd.print_line(0, "Modbus RTU App", callback);
      break;

    case Ready:
      self->dispatch_pending();
      break;

    case Error:
      break;
  }
}

}; // namespace ModbusRtuServer
