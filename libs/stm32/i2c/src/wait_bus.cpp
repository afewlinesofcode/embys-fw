#include "wait_bus.hpp"

#include "bus.hpp"
#include "hal.hpp"

namespace Embys::Stm32::I2c
{

WaitBus::WaitBus(BusCore *bus, Callback<Status> cb_)
  : i2c(bus->get_i2c()), checks_count(bus->get_scl_hz() > 100000u ? 20u : 50u),
    cb(cb_), event(*bus->get_base(), Base::EventMode::Deferred,
                   {WaitBus::event_callback, this})
{
}

Status
WaitBus::start()
{
  count = 0;

  if (!is_busy(i2c))
  {
    cb(Status::success());
    return Status::success();
  }

  if (!event.enable(std::chrono::microseconds{WaitBus::check_us}))
    return Status::failure(Error::Schedule);

  return Status::success();
}

void
WaitBus::event_callback(void *context) noexcept
{
  auto *self = static_cast<WaitBus *>(context);

  if (!is_busy(self->i2c))
  {
    self->cb(Status::success());
    return;
  }

  if (++self->count >= self->checks_count)
  {
    self->cb(Status::failure(Error::BusBusy));
    return;
  }

  if (!self->event.enable(std::chrono::microseconds{WaitBus::check_us}))
    self->cb(Status::failure(Error::Schedule));
}

}; // namespace Embys::Stm32::I2c
