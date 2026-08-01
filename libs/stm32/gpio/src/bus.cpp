#include <embys/stm32/def.hpp>

#include "bus.hpp"
#include "def.hpp"
#include "hal.hpp"

namespace Embys::Stm32::Gpio
{

Bus::Bus(Base::Loop *base, Pin **pin_slots, size_t pins_capacity)
  : base(base), pins(pin_slots), pins_capacity(pins_capacity)
{
  // Initialize pin registry
  for (size_t i = 0; i < pins_capacity; ++i)
  {
    pins[i] = nullptr;
  }
}

Bus::~Bus()
{
}

int
Bus::enable()
{
  if (enabled)
  {
    // Already enabled
    return 0;
  }

  // TRY(enable_exti_source_clock());
  module = base->add_module({Bus::module_callback, this});

  enabled = true;

  return 0;
}

int
Bus::disable()
{
  if (!enabled)
  {
    // Already disabled
    return 0;
  }

  // TRY(disable_exti_source_clock());
  base->remove_module(module);
  module = nullptr;

  enabled = false;
  return 0;
}

void
Bus::handle_irq(uint8_t start, uint8_t end)
{
  for (uint8_t pin_index = start; pin_index <= end; ++pin_index)
  {
    if (exti_get_and_clear_pending(pin_index))
      activate_pin(1U << pin_index);
  }

  set_module_pending();
}

int
Bus::add(Pin *pin)
{
  TRY(check_enabled());

  // Find available slot in registry
  for (size_t i = 0; i < pins_capacity; ++i)
  {
    if (!pins[i])
    {
      pins[i] = pin;
      return 0;
    }
  }

  // No available slots
  return BUS_FULL;
}

int
Bus::remove(Pin *pin)
{
  TRY(check_enabled());

  auto port = pin->get_port();
  bool port_in_use = false;

  // Find pin in registry and remove
  for (size_t i = 0; i < pins_capacity; ++i)
  {
    if (pins[i] == pin)
      pins[i] = nullptr;
    else if (pins[i] && pins[i]->get_port() == port)
      port_in_use = true;
  }

  if (!port_in_use)
    TRY(disable_gpio(port));

  return 0;
}

void
Bus::activate_pin(uint32_t pin_bit)
{
  SET_BIT_V(activated_exti_lines, pin_bit);
}

int
Bus::trigger_activated_pins()
{
  // Process all pins with pending interrupts
  for (size_t i = 0; i < pins_capacity; ++i)
  {
    auto pin_ptr = pins[i];

    if (!pin_ptr)
      continue;

    uint32_t pin_bit = (1 << pin_ptr->get_index());

    if ((activated_exti_lines & pin_bit) &&
        has_cfg(pin_ptr->get_cfg(), PinCfg::LISTEN))
    {
      {
        IrqGuard();
        CLEAR_BIT_V(activated_exti_lines, pin_bit);
      }

      // Trigger callback for the pin
      pin_ptr->trigger();
    }
  }

  return 0;
}

int
Bus::check_enabled()
{
  if (!enabled)
    return BUS_NOT_ENABLED; // GPIO bus is not enabled

  return 0;
}

}; // namespace Embys::Stm32::Gpio
