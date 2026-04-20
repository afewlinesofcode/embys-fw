#pragma once

#include <embys/stm32/base/loop.hpp>

#include "api.hpp"
#include "pin.hpp"


/**
 * @brief EXTI IRQ handler template for pins start to end (inclusive)
 *
 */
#define EMBYS_EXTI_IRQ_HANDLER(bus_ptr, start, end)


namespace Embys::Stm32::Gpio
{

class Pin;

/**
 * @class Gpio
 * @brief STM32F10x GPIO Controller
 *
 * Central management for GPIO pins and EXTI interrupts with Base system
 * coordination for precise timing.
 * Requires module slot in Base loop for event notifications.
 */
class Bus
{
public:
  /** @brief Initialize GPIO controller and hardware */
  Bus(Base::Loop *base, Pin **pin_slots, size_t pins_capacity);

  /** @brief Clean up GPIO controller resources */
  ~Bus();

  inline bool
  is_enabled() const
  {
    return enabled;
  }

  inline Base::Loop *
  get_base() const
  {
    return base;
  }

  int
  enable();

  int
  disable();

  inline void
  handle_irq(uint8_t start, uint8_t end)
  {
    for (uint8_t pin_index = start; pin_index <= end; ++pin_index)
    {
      uint32_t pin_bit = (1 << pin_index);

      if (EXTI->PR & pin_bit)
      {
        EXTI->PR = pin_bit;
        activate_pin(pin_bit);
      }
    }

    base->interrupted(module);
  }

private:
  friend class Pin;

  /**
   * @brief Pointer to the Base loop for event scheduling
   */
  Base::Loop *base;

  /**
   * @brief Registry of active GPIO pins
   */
  Pin **pins;

  /**
   * @brief Capacity of the pins registry
   */
  size_t pins_capacity;

  /**
   * @brief Indicates whether the GPIO controller is enabled
   */
  bool enabled = false;

  /**
   * @brief Bitmask of EXTI lines with pending interrupts
   */
  volatile uint32_t activated_exti_lines = 0;

  /**
   * @brief Registered Base module for GPIO event notifications
   */
  Base::Module *module = nullptr;

  /**
   * @brief Add and initialize GPIO pin
   *
   * @param pin GPIO pin to add
   * @return int Status code
   */
  int
  add(Pin *pin);

  /**
   * @brief Remove GPIO pin and clean up resources
   *
   * @param pin GPIO pin to remove
   * @return int Status code
   */
  int
  remove(Pin *pin);

  /**
   * @brief Mark EXTI line as activated and notify main loop.
   * This should be called from the EXTI IRQ handler when a GPIO interrupt is
   * detected.
   *
   * @param pin_bit Bitmask of the activated EXTI line
   */
  void
  activate_pin(uint32_t pin_bit);

  /**
   * @brief Process all activated EXTI lines and trigger corresponding pin
   * callbacks. This should be called from the main loop context to ensure that
   * pin callbacks are executed outside of the interrupt context.
   *
   * @return int Status code
   */
  int
  trigger_activated_pins();

  /**
   * @brief Notify main loop of pending GPIO event. This should be called from
   * the EXTI IRQ handler after marking the activated EXTI line to ensure that
   * the main loop executes the GPIO event processing as soon as possible.
   */
  inline void
  module_notify()
  {
    if (module)
      base->interrupted(module);
  }

  /**
   * @brief Static handler function to be registered as a Base module callback.
   * This function will be called by the Base loop when the GPIO event is
   * scheduled for processing, and it will trigger the processing of activated
   * EXTI lines.
   *
   * @param context Pointer to the Bus instance (passed during module
   * registration)
   */
  static void
  module_handler(void *context)
  {
    Bus *gpio = static_cast<Bus *>(context);
    gpio->trigger_activated_pins();
  }
};

}; // namespace Embys::Stm32::Gpio
