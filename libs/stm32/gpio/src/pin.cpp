#include "pin.hpp"

#include <embys/stm32/def.hpp>

#include "bus.hpp"
#include "def.hpp"
#include "hal.hpp"

namespace Embys::Stm32::Gpio
{

PinCore::PinCore(BusCore &bus, GPIO_TypeDef *port, uint8_t index, PinCfg cfg)
  : enabled(false), bus(&bus), port(port), index(index), cfg(cfg)
{
}

PinCore::~PinCore()
{
  if (enabled)
  {
    disable();
  }
}

void
PinCore::set_init_value(uint8_t value)
{
  init_value = value ? 1 : 0;
  has_init_value = true;
}

void
PinCore::bind_pwm_impl(Base::Timer &timer, uint8_t channel)
{
  pwm.timer = &timer;
  pwm.channel = channel;
}

void
PinCore::set_callback(Callback<uint8_t> cb)
{
  this->cb = cb;
}

void
PinCore::clear_callback()
{
  cb.clear();
}

int
PinCore::enable()
{
  if (enabled)
  {
    // Already enabled
    return 0;
  }

  TRY(validate_pin_config(port, index, cfg, &pwm));

  TRY(configure_pin(port, index, cfg, &pwm));

  if (has_init_value)
    TRY(write(init_value));

  TRY(bus->add(this));

  enabled = true;

  return 0;
}

int
PinCore::disable()
{
  if (!enabled)
  {
    // Already disabled
    return 0;
  }

  TRY(bus->remove(this));

  enabled = false;

  // Clear interrupt callback
  clear_callback();

  // Reset pin to safe state (input floating)
  reset_pin(port, index, cfg, &pwm);

  return 0;
}

int
PinCore::read(uint8_t *value)
{
  return read_pin(port, index, value);
}

int
PinCore::write(uint8_t value)
{
  return write_pin(port, index, value);
}

void
PinCore::trigger()
{
  uint8_t value;

  if (read(&value) < 0)
    return; // Failed to read pin state, can't trigger callback

  cb(value);
}

}; // namespace Embys::Stm32::Gpio
