#include "bus.hpp"

#include <embys/stm32/def.hpp>

namespace Embys::Stm32::I2c
{

Bus::Bus(I2C_TypeDef *i2c_, Base::Loop *base_)
  : i2c(i2c_), base(base_), sm(i2c_),
    timeout_event(base_, Base::EV_RT, {Bus::timeout_handler, this})
{
}

Bus::~Bus()
{
  if (enabled)
    (void)disable();
}

int
Bus::enable(uint32_t scl_hz_)
{
  if (enabled)
    return 0;

  TRY(enable_i2c(i2c, scl_hz_));

  if (is_busy())
    TRY(reset_i2c(i2c));

  module = base->add_module({Bus::module_handler, this});
  if (!module)
  {
    (void)disable_i2c(i2c);
    return BUS_NOT_ENABLED;
  }

  scl_hz = scl_hz_;
  enabled = true;

  return 0;
}

int
Bus::disable()
{
  if (!enabled)
    return 0;

  TRY(timeout_event.disable());
  base->remove_module(module);
  module = nullptr;

  TRY(disable_i2c(i2c));

  enabled = false;

  return 0;
}

int
Bus::read(uint8_t addr7, uint8_t *buf, uint16_t len, Callable<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  TRY(sm.start_read(cb, addr7, buf, len));
  TRY(timeout_event.enable(calc_timeout_us(len)));

  return 0;
}

int
Bus::read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len,
          Callable<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  TRY(sm.start_read(cb, addr7, reg, buf, len));
  TRY(timeout_event.enable(calc_timeout_us(len + 1)));

  return 0;
}

int
Bus::write(uint8_t addr7, const uint8_t *buf, uint16_t len, Callable<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  TRY(sm.start_write(cb, addr7, buf, len));
  TRY(timeout_event.enable(calc_timeout_us(len)));

  return 0;
}

void
Bus::handle_ev_irq()
{
  sm.handle_irq();
  if (sm.is_result_ready())
    module_notify();
}

void
Bus::handle_er_irq()
{
  sm.handle_error();
  if (sm.is_result_ready())
    module_notify();
  i2c->SR1 = 0; // Clear error flags to avoid repeated interrupts
}

// ── private
// ───────────────────────────────────────────────────────────────────

uint32_t
Bus::calc_timeout_us(uint16_t len) const
{
  // 25 ms base + ~250 µs per byte (generous for 100 kHz; fine for 400 kHz)
  return 25000u + static_cast<uint32_t>(len) * 250u;
}

void
Bus::module_notify()
{
  base->interrupted(module);
}

void
Bus::module_handler(void *context)
{
  auto *self = static_cast<Bus *>(context);

  if (self->sm.is_result_ready())
  {
    (void)self->timeout_event.disable();
    self->sm.complete();
  }
}

void
Bus::timeout_handler(void *context)
{
  auto *self = static_cast<Bus *>(context);
  self->sm.force_timeout();
  self->module_notify();
}

}; // namespace Embys::Stm32::I2c
