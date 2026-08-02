/**
 * @file bus.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief GPIO Bus managing a set of GPIO pins on a port
 *
 * The Bus class acts as a Module registered with the main Loop. It configures
 * GPIO MODE/CNF/EXTI for each registered pin and dispatches pin-level
 * callbacks from EXTI IRQ handlers.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>

#include <embys/stm32/base/loop.hpp>

#include "def.hpp"
#include "pin.hpp"

namespace Embys::Stm32::Gpio
{

/**
 * @class Bus
 * @brief STM32F10x GPIO Controller
 * Central management for GPIO pins and EXTI interrupts with Base system
 * coordination for precise timing.
 * Requires module slot in Base loop for event notifications.
 */
class BusCore
{
public:
  BusCore(const BusCore &) = delete;
  BusCore(BusCore &&) = delete;
  BusCore &
  operator=(const BusCore &) = delete;
  BusCore &
  operator=(BusCore &&) = delete;

  /**
   * @brief Clean up GPIO bus resources
   */
  ~BusCore();

  /**
   * @brief Check if the GPIO bus is enabled
   * @return true if the bus is enabled, false otherwise
   */
  inline bool
  is_enabled() const
  {
    return enabled;
  }

  /**
   * @brief Get the Base loop associated with this GPIO bus
   * @return Base::LoopCore* Pointer to the Base loop
   */
  inline Base::LoopCore *
  get_base() const
  {
    return base;
  }

  /**
   * @brief Enable the GPIO bus and prepare for pin management
   * @return Success or the module-registration failure.
   */
  [[nodiscard]] Status
  enable();

  /**
   * @brief Disable the GPIO bus and clean up resources
   * @return Success.
   */
  [[nodiscard]] Status
  disable();

  /**
   * @brief Handle GPIO interrupt request for a range of pins
   * @param start Starting pin index
   * @param end Ending pin index
   */
  void
  handle_irq(uint8_t start, uint8_t end);

private:
  friend class PinCore;

protected:
  BusCore(Base::LoopCore &base, PinCore **pin_slots, size_t pins_capacity);

private:
  /**
   * @brief Pointer to the Base loop for event scheduling
   */
  Base::LoopCore *base;

  /**
   * @brief Registry of active GPIO pins
   */
  PinCore **pins;

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
   * @param pin GPIO pin to add
   * @return Success or the capacity/enable-state failure.
   */
  [[nodiscard]] Status
  add(PinCore *pin);

  /**
   * @brief Remove GPIO pin and clean up resources
   * @param pin GPIO pin to remove
   * @return Success or the enable-state failure.
   */
  [[nodiscard]] Status
  remove(PinCore *pin);

  /**
   * @brief Mark EXTI line as activated and notify main loop.
   * This should be called from the EXTI IRQ handler when a GPIO interrupt is
   * detected.
   * @param pin_bit Bitmask of the activated EXTI line
   */
  void
  activate_pin(uint32_t pin_bit);

  /**
   * @brief Process all activated EXTI lines and trigger corresponding pin
   * callbacks. This should be called from the main loop context to ensure that
   * pin callbacks are executed outside of the interrupt context.
   * @return Success.
   */
  [[nodiscard]] Status
  trigger_activated_pins();

  /**
   * @brief Check if the GPIO bus is enabled before performing operations
   * @return Success or Error::BusNotEnabled.
   */
  [[nodiscard]] Status
  check_enabled();

  /**
   * @brief Notify main loop of pending GPIO event. This should be called from
   * the EXTI IRQ handler after marking the activated EXTI line to ensure that
   * the main loop executes the GPIO event processing as soon as possible.
   */
  inline void
  set_module_pending()
  {
    if (module)
      base->set_module_pending(module);
  }

  /**
   * @brief Static handler function to be registered as a Base module callback.
   * This function will be called by the Base loop when the GPIO event is
   * scheduled for processing, and it will trigger the processing of activated
   * EXTI lines.
   * @param context Pointer to the Bus instance (passed during module
   * registration)
   */
  static void
  module_callback(void *context) noexcept
  {
    BusCore *gpio = static_cast<BusCore *>(context);
    (void)gpio->trigger_activated_pins();
  }
};

namespace Detail
{

template <size_t PinsCapacity>
struct BusStorage
{
  std::array<PinCore *, PinsCapacity> pins{};
};

} // namespace Detail

template <size_t PinsCapacity>
class Bus final : private Detail::BusStorage<PinsCapacity>, public BusCore
{
  static_assert(PinsCapacity > 0, "A GPIO bus needs at least one pin slot");
  using Storage = Detail::BusStorage<PinsCapacity>;

public:
  explicit Bus(Base::LoopCore &base)
    : Storage(), BusCore(base, Storage::pins.data(), PinsCapacity)
  {
  }
};

}; // namespace Embys::Stm32::Gpio
