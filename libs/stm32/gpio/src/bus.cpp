#include "bus.hpp"

#include <embys/stm32/def.hpp>

#include "def.hpp"
#include "hal.hpp"

namespace Embys::Stm32::Gpio
{

BusCore::BusCore(Base::LoopCore &base, PinCore **pin_slots,
                 size_t pins_capacity)
  : base(&base), pins(pin_slots), pins_capacity(pins_capacity)
{
  // Initialize pin registry
  for (size_t i = 0; i < pins_capacity; ++i)
  {
    pins[i] = nullptr;
  }
}

BusCore::~BusCore()
{
}

Status
BusCore::enable()
{
  if (enabled)
  {
    // Already enabled
    return Status::success();
  }

  module = base->add_module({BusCore::module_callback, this});
  if (module == nullptr)
    return Status::failure(Error::ModuleCapacity);

  enabled = true;

  return Status::success();
}

Status
BusCore::disable()
{
  if (!enabled)
  {
    // Already disabled
    return Status::success();
  }

  base->remove_module(module);
  module = nullptr;

  enabled = false;
  return Status::success();
}

void
BusCore::handle_irq(uint8_t start, uint8_t end)
{
  for (uint8_t pin_index = start; pin_index <= end; ++pin_index)
  {
    if (exti_get_and_clear_pending(pin_index))
      activate_pin(1U << pin_index);
  }

  set_module_pending();
}

Status
BusCore::add(PinCore *pin)
{
  const Status enabled_result = check_enabled();
  if (!enabled_result)
    return enabled_result;

  // Find available slot in registry
  for (size_t i = 0; i < pins_capacity; ++i)
  {
    if (!pins[i])
    {
      pins[i] = pin;
      return Status::success();
    }
  }

  // No available slots
  return Status::failure(Error::BusFull);
}

Status
BusCore::remove(PinCore *pin)
{
  const Status enabled_result = check_enabled();
  if (!enabled_result)
    return enabled_result;

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
  {
    const Status disable_result = disable_gpio(port);
    if (!disable_result)
      return disable_result;
  }

  return Status::success();
}

void
BusCore::activate_pin(uint32_t pin_bit)
{
  SET_BIT_V(activated_exti_lines, pin_bit);
}

Status
BusCore::trigger_activated_pins()
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
        IrqGuard guard;
        CLEAR_BIT_V(activated_exti_lines, pin_bit);
      }

      // Trigger callback for the pin
      pin_ptr->trigger();
    }
  }

  return Status::success();
}

Status
BusCore::check_enabled()
{
  if (!enabled)
    return Status::failure(Error::BusNotEnabled);

  return Status::success();
}

}; // namespace Embys::Stm32::Gpio
