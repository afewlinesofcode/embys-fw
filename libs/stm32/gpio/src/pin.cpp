#include "pin.hpp"

#include <embys/stm32/def.hpp>

#include "bus.hpp"
#include "def.hpp"
#include "hal.hpp"

namespace Embys::Stm32::Gpio
{

Pin::Pin(Bus *bus, GPIO_TypeDef *port, uint8_t index, PinCfg cfg)
  : enabled(false), bus(bus), port(port), index(index), cfg(cfg)
{
}

Pin::~Pin()
{
  if (enabled)
  {
    disable();
  }
}

void
Pin::set_init_value(uint8_t value)
{
  init_value = value ? 1 : 0;
  has_init_value = true;
}

void
Pin::bind_pwm(Base::Timer *timer, uint8_t channel)
{
  pwm.timer = timer;
  pwm.channel = channel;
}

void
Pin::set_callback(Callback<uint8_t> cb)
{
  this->cb = cb;
}

void
Pin::clear_callback()
{
  cb.clear();
}

int
Pin::enable()
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
Pin::disable()
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
Pin::read(uint8_t *value)
{
  return read_pin(port, index, value);
}

int
Pin::write(uint8_t value)
{
  return write_pin(port, index, value);
}

void
Pin::trigger()
{
  uint8_t value;

  if (read(&value) < 0)
    return; // Failed to read pin state, can't trigger callback

  cb(value);
}

}; // namespace Embys::Stm32::Gpio
