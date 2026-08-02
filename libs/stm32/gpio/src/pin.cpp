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
    (void)disable();
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

Status
PinCore::enable()
{
  if (enabled)
  {
    // Already enabled
    return Status::success();
  }

  const Status validation = validate_pin_config(port, index, cfg, &pwm);
  if (!validation)
    return validation;

  const Status configuration = configure_pin(port, index, cfg, &pwm);
  if (!configuration)
    return configuration;

  if (has_init_value)
  {
    const Status write_result = write(init_value);
    if (!write_result)
    {
      (void)reset_pin(port, index, cfg, &pwm);
      return write_result;
    }
  }

  const Status add_result = bus->add(this);
  if (!add_result)
  {
    (void)reset_pin(port, index, cfg, &pwm);
    return add_result;
  }

  enabled = true;

  return Status::success();
}

Status
PinCore::disable()
{
  if (!enabled)
  {
    // Already disabled
    return Status::success();
  }

  const Status remove_result = bus->remove(this);
  if (!remove_result)
    return remove_result;

  enabled = false;

  // Clear interrupt callback
  clear_callback();

  // Reset pin to safe state (input floating)
  const Status reset_result = reset_pin(port, index, cfg, &pwm);
  if (!reset_result)
    return reset_result;

  return Status::success();
}

ReadResult
PinCore::read()
{
  return read_pin(port, index);
}

Status
PinCore::write(uint8_t value)
{
  return write_pin(port, index, value);
}

void
PinCore::trigger()
{
  const ReadResult value = read();
  if (!value)
    return; // Failed to read pin state, can't trigger callback

  cb(value.value() ? 1U : 0U);
}

}; // namespace Embys::Stm32::Gpio
