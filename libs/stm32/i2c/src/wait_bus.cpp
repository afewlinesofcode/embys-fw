#include "wait_bus.hpp"

#include "bus.hpp"
#include "hal.hpp"

namespace Embys::Stm32::I2c
{

WaitBus::WaitBus(BusCore *bus, Callback<int> cb_)
  : i2c(bus->get_i2c()), checks_count(bus->get_scl_hz() > 100000u ? 20u : 50u),
    cb(cb_), event(*bus->get_base(), 0, {WaitBus::event_callback, this})
{
}

int
WaitBus::start()
{
  count = 0;

  if (!is_busy(i2c))
  {
    cb(0);
    return 0;
  }

  return event.enable(WaitBus::check_us);
}

void
WaitBus::event_callback(void *context)
{
  auto *self = static_cast<WaitBus *>(context);

  if (!is_busy(self->i2c))
  {
    self->cb(0);
    return;
  }

  if (++self->count >= self->checks_count)
  {
    self->cb(BUS_BUSY);
    return;
  }

  (void)self->event.enable(WaitBus::check_us);
}

}; // namespace Embys::Stm32::I2c
