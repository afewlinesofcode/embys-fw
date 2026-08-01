#include "bus.hpp"

#include <embys/stm32/def.hpp>

#include "hal.hpp"

namespace Embys::Stm32::I2c
{

Bus::Bus(I2C_TypeDef *i2c_, Base::LoopCore *base_)
  : i2c(i2c_), base(base_), sm(this)
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
  TRY(reset_i2c(i2c));


  module = base->add_module({Bus::module_callback, this});
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

  sm.reset();
  base->remove_module(module);
  module = nullptr;

  TRY(disable_i2c(i2c));

  enabled = false;

  return 0;
}

int
Bus::read(uint8_t addr7, uint8_t *buf, uint16_t len, Callback<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  this->cb = cb;
  TRY(sm.start_read(addr7, buf, len));

  return 0;
}

int
Bus::read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len,
          Callback<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  this->cb = cb;
  TRY(sm.start_read(addr7, reg, buf, len));

  return 0;
}

int
Bus::write(uint8_t addr7, const uint8_t *buf, uint16_t len, Callback<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  this->cb = cb;
  TRY(sm.start_write(addr7, buf, len));

  return 0;
}

void
Bus::handle_ev_irq()
{
  sm.handle_irq();

  if (sm.is_complete())
    set_module_pending();
}

void
Bus::handle_er_irq()
{
  sm.handle_error();

  if (sm.is_complete())
    set_module_pending();

  clear_error_flags(i2c); // Clear error flags to avoid repeated interrupts
}

void
Bus::module_callback(void *context)
{
  auto *self = static_cast<Bus *>(context);

  if (self->sm.is_complete())
  {
    int result = self->sm.get_result();
    self->sm.reset();
    self->cb(result);
  }
}

}; // namespace Embys::Stm32::I2c
