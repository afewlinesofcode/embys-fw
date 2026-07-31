#include "hal.hpp"

#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>

#include "def.hpp"

// All family-specific GPIO HAL functions are implemented in
// src/hal/<family>/hal.cpp, selected by the build system.
// This file contains only the portable pin I/O functions that share
// the same register layout (IDR, BSRR) across all supported families.

namespace Embys::Stm32::Gpio
{

extern bool
is_valid_pwm_binding(const PwmBinding *binding);

int
read_pin(GPIO_TypeDef *port, uint8_t index, uint8_t *value)
{
  *value = (port->IDR & (1U << index)) ? 1 : 0;
  return 0;
}

int
write_pin(GPIO_TypeDef *port, uint8_t index, uint8_t value)
{
  if (value)
    port->BSRR = (1U << index);
  else
    port->BSRR = (1U << (index + 16));

  return 0;
}

int
validate_pin_config([[maybe_unused]] GPIO_TypeDef *port,
                    [[maybe_unused]] uint8_t index, PinCfg cfg,
                    const PwmBinding *pwm)
{
  if (has_many_roles(cfg))
    return PIN_CONFIG_CONFLICT;

  if (has_many_speeds(cfg))
    return PIN_CONFIG_CONFLICT;

  if (has_cfg(cfg, PinCfg::PU) && has_cfg(cfg, PinCfg::PD))
    return PIN_CONFIG_CONFLICT;

  if (!has_any_role(cfg) && !has_cfg(cfg, PinCfg::IN) &&
      !has_cfg(cfg, PinCfg::OUT) && !has_cfg(cfg, PinCfg::AF))
    return PIN_CONFIG_CONFLICT;

  if (has_cfg(cfg, PinCfg::PWM) && !is_valid_pwm_binding(pwm))
    return PIN_CONFIG_CONFLICT;


  return 0;
}

PinCfg
get_effective_pin_cfg(const PinCfg &cfg)
{
  PinCfg effective_cfg = cfg;

  if (has_cfg(cfg, PinCfg::I2C))
    effective_cfg = PinCfg::OUT | PinCfg::AF | PinCfg::OD;
  else if (has_cfg(cfg, PinCfg::UART))
    effective_cfg = PinCfg::OUT | PinCfg::AF;
  else if (has_cfg(cfg, PinCfg::SPI))
    effective_cfg = PinCfg::OUT | PinCfg::AF;
  else if (has_cfg(cfg, PinCfg::PWM))
    effective_cfg = PinCfg::OUT | PinCfg::AF;
  else if (has_cfg(cfg, PinCfg::ANALOG))
    effective_cfg = PinCfg::ANALOG;

  if (has_cfg(effective_cfg, PinCfg::OUT))
  {
    if (!has_cfg(cfg, PinCfg::LOW | PinCfg::MEDIUM))
      effective_cfg |= PinCfg::HIGH;
  }

  return effective_cfg;
}

}; // namespace Embys::Stm32::Gpio
