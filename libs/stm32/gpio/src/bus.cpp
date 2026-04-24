#include "bus.hpp"

#include <embys/stm32/def.hpp>

#include "diag.hpp"
#include "stm32f1xx.hpp"

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

  TRY(enable_afio());
  module = base->add_module({Bus::module_handler, this});

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

  TRY(disable_afio());
  base->remove_module(module);
  module = nullptr;

  enabled = false;
  return 0;
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
        (pin_ptr->get_pin_cfg() & PinCfg::IRQ))
    {
      cs_begin();
      CLEAR_BIT_V(activated_exti_lines, pin_bit);
      cs_end();

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
