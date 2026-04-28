#include "bus.hpp"

#include <embys/stm32/def.hpp>

namespace Embys::Stm32::I2c
{

Bus::Bus(I2C_TypeDef *i2c_, Base::Loop *base_)
  : i2c(i2c_), base(base_), sm(i2c_),
    timeout_event(base_, Base::EV_RT, {Bus::timeout_handler, this})
{
  if (i2c == I2C1 && !instances[0])
    instances[0] = this;
  else if (i2c == I2C2 && !instances[1])
    instances[1] = this;
}

Bus::~Bus()
{
  if (enabled)
    (void)disable();

  if (i2c == I2C1 && instances[0] == this)
    instances[0] = nullptr;
  else if (i2c == I2C2 && instances[1] == this)
    instances[1] = nullptr;
}

int
Bus::enable(uint32_t scl_hz_)
{
  if (enabled)
    return 0;

  TRY(enable_i2c(i2c, scl_hz_));

  if (is_busy(i2c))
    (void)reset_i2c(i2c);

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

  (void)timeout_event.disable();
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

  int rc = sm.start_read(cb, addr7, buf, len);
  if (rc == INVALID_STATE)
    return BUSY;
  TRY(rc);
  (void)timeout_event.enable(calc_timeout_us(len));

  return 0;
}

int
Bus::read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len,
          Callable<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  int rc = sm.start_read(cb, addr7, reg, buf, len);
  if (rc == INVALID_STATE)
    return BUSY;
  TRY(rc);
  (void)timeout_event.enable(calc_timeout_us(static_cast<uint16_t>(len + 1u)));

  return 0;
}

int
Bus::write(uint8_t addr7, const uint8_t *buf, uint16_t len, Callable<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  int rc = sm.start_write(cb, addr7, buf, len);
  if (rc == INVALID_STATE)
    return BUSY;
  TRY(rc);
  (void)timeout_event.enable(calc_timeout_us(len));

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

Bus *instances[2] = {nullptr, nullptr};

void
ev_irq_handler(uint8_t index)
{
  auto *bus = instances[index];
  if (bus)
    bus->handle_ev_irq();
}

void
er_irq_handler(uint8_t index)
{
  auto *bus = instances[index];
  if (bus)
    bus->handle_er_irq();
  else if (index == 0u)
    I2C1->SR1 = 0u; // clear flags to prevent IRQ storm
  else
    I2C2->SR1 = 0u;
}

}; // namespace Embys::Stm32::I2c
