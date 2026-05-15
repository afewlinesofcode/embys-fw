#include "pin.hpp"

#include <embys/stm32/def.hpp>

#include "bus.hpp"
#include "diag.hpp"
#include "hal.hpp"

namespace Embys::Stm32::Gpio
{

Pin::Pin(Bus *bus, GPIO_TypeDef *port, uint8_t index, PinCfg cfg)
  : enabled(false), bus(bus), port(port), index(index), cfg(cfg)
{
}

int
Pin::enable()
{
  if (enabled)
  {
    // Already enabled
    return 0;
  }

  TRY(validate_config());

  // Enable GPIO port clock
  TRY(enable_gpio(port));

  TRY(configure_pin(port, index, cfg));

  if (has_cfg(cfg, PinCfg::OUT) && !has_any_role(cfg))
    TRY(write_pin(port, index, init_value));

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
  TRY(clear_callback());

  // Reset pin to safe state (input floating)
  reset_pin(port, index);

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

int
Pin::set_callback(Callable<uint8_t> cb)
{
  this->cb = cb;
  return 0;
}

int
Pin::clear_callback()
{
  cb.clear();
  return 0;
}

void
Pin::trigger()
{
  uint8_t value;

  if (read(&value) < 0)
    return; // Failed to read pin state, can't trigger callback

  cb(value);
}

int
Pin::validate_config()
{
  uint32_t role_bits = static_cast<uint32_t>(cfg) & all_roles_mask;
  if (role_bits != 0 && (role_bits & (role_bits - 1U)) != 0)
    return PIN_CONFIG_CONFLICT;

  uint32_t speed_bits = static_cast<uint32_t>(cfg) & all_speed_mask;
  if (speed_bits != 0 && (speed_bits & (speed_bits - 1U)) != 0)
    return PIN_CONFIG_CONFLICT;

  if (has_cfg(cfg, PinCfg::PU) && has_cfg(cfg, PinCfg::PD))
    return PIN_CONFIG_CONFLICT;

  if (!has_any_role(cfg) && !has_cfg(cfg, PinCfg::IN) &&
      !has_cfg(cfg, PinCfg::OUT) && !has_cfg(cfg, PinCfg::AF))
    return PIN_CONFIG_CONFLICT;

  return 0;
}

}; // namespace Embys::Stm32::Gpio
