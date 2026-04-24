#include "pin.hpp"

#include <embys/stm32/def.hpp>

#include "api.hpp"
#include "bus.hpp"
#include "diag.hpp"

namespace Embys::Stm32::Gpio
{

Pin::Pin(Bus *bus, GPIO_TypeDef *port, uint8_t index, uint32_t gpio_mode,
         uint32_t gpio_cnf, uint8_t pin_cfg)
  : enabled(false), bus(bus), port(port), index(index),
    gpio_cfg(make_cfg(gpio_mode, gpio_cnf)), pin_cfg(pin_cfg)
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

  // Configure GPIO pin mode and CNF
  TRY(configure_pin(port, index, gpio_cfg));

  // Configure pull resistors via ODR
  if (pin_cfg & PinCfg::PULL_UP)
  {
    TRY(configure_pin_pull_up(port, index));
  }
  else if (pin_cfg & PinCfg::PULL_DOWN)
  {
    TRY(configure_pin_pull_down(port, index));
  }

  // Initialize interrupt if requested
  if (pin_cfg & PinCfg::IRQ)
  {
    TRY(enable_pin_irq(port, index));
  }

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

  // Clean up interrupt configuration
  if (pin_cfg & PinCfg::IRQ)
  {
    disable_pin_irq(port, index);
  }

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
  // Extract MODE and CNF fields
  uint32_t mode = gpio_cfg & 0b11;
  uint32_t cnf = (gpio_cfg & 0b1100) >> 2;

  // Validate CNF based on MODE
  if (mode == Mode::IN)
  {
    if (cnf > Cnf::IN_PU)
      return PIN_CNF_CONFIG_FAILED; // Invalid input CNF

    // Check pull configuration compatibility
    // Pull-up/pull-down only valid with GPIO_CNF_IN_PU
    if (cnf != Cnf::IN_PU &&
        (pin_cfg & (PinCfg::PULL_UP | PinCfg::PULL_DOWN)) != 0)
      return PIN_CONFIG_CONFLICT; // Conflicting input pull configuration
  }

  // Check for conflicting pull settings
  if ((pin_cfg & PinCfg::PULL_UP) && (pin_cfg & PinCfg::PULL_DOWN))
    return PIN_CONFIG_CONFLICT; // Conflicting pull configuration

  return 0;
}

}; // namespace Embys::Stm32::Gpio
