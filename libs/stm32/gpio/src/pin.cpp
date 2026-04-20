#include "pin.hpp"

#include "api.hpp"
#include "bus.hpp"

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

  if (validate_config() < 0)
    return -1; // Invalid configuration

  // Configure GPIO pin mode and CNF
  if (configure_pin(port, index, gpio_cfg) < 0)
    return -1; // Pin configuration failed

  // Configure pull resistors via ODR
  if (pin_cfg & PinCfg::PULL_UP)
  {
    if (configure_pin_pull_up(port, index) < 0)
      return -1; // Failed to configure pull-up
  }
  else if (pin_cfg & PinCfg::PULL_DOWN)
  {
    if (configure_pin_pull_down(port, index) < 0)
      return -1; // Failed to configure pull-down
  }

  // Initialize interrupt if requested
  if (pin_cfg & PinCfg::IRQ)
  {
    if (enable_pin_irq(port, index) < 0)
      return -1; // Failed to configure pin IRQ
  }

  if (bus->add(this) < 0)
    return -1; // Failed to register pin with bus

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

  if (bus->remove(this) < 0)
    return -1; // Failed to unregister pin from bus

  enabled = false;

  // Clear interrupt callback
  if (clear_callback() < 0)
    return -1; // Failed to clear callback

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
      return -1; // Invalid input CNF

    // Check pull configuration compatibility
    // Pull-up/pull-down only valid with GPIO_CNF_IN_PU
    if (cnf != Cnf::IN_PU &&
        (pin_cfg & (PinCfg::PULL_UP | PinCfg::PULL_DOWN)) != 0)
      return -1; // Conflicting input pull configuration
  }

  // Check for conflicting pull settings
  if ((pin_cfg & PinCfg::PULL_UP) && (pin_cfg & PinCfg::PULL_DOWN))
    return -1; // Conflicting pull configuration

  return 0;
}

}; // namespace Embys::Stm32::Gpio
