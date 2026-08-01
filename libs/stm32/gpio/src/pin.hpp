/**
 * @file pin.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief GPIO Pin abstraction for individual pin management
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <embys/stm32/types.hpp>

#include "def.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::Gpio
{

class BusCore;
class Api;

class Pin
{
public:
  /**
   * @brief Construct a new Pin object with specified GPIO port, index, and
   * configuration
   * @param bus Pointer to the GPIO bus managing this pin
   * @param port GPIO port (GPIOA, GPIOB, …)
   * @param index Pin index within the port (0..15)
   * @param cfg Pin configuration flags (PinCfg bitmask)
   */
  Pin(BusCore &bus, GPIO_TypeDef *port, uint8_t index, PinCfg cfg);

  Pin(BusCore *bus, GPIO_TypeDef *port, uint8_t index, PinCfg cfg)
    : Pin(*bus, port, index, cfg)
  {
  }

  /**
   * @brief Destroy the Pin object and release resources
   */
  ~Pin();

  /**
   * @brief Set the initial output value for this pin. This value will be
   * applied when the pin is enabled if it is configured as an output.
   * @param value Initial output value (0 for low, non-zero for high)
   */
  void
  set_init_value(uint8_t value);

  /**
   * @brief Bind a PWM timer to this pin for PWM output
   * @param timer Pointer to the Base::Timer object for PWM generation
   * @param channel Timer channel number for PWM output (1..4)
   */
  void
  bind_pwm(Base::Timer *timer, uint8_t channel);

  /**
   * @brief Set the callback to be invoked on pin interrupt events
   * @param cb Callback function to execute on interrupt events, receiving the
   * pin state (0 or 1) as an argument
   */
  void
  set_callback(Callback<uint8_t> cb);

  /**
   * @brief Clear the interrupt callback, disabling interrupt notifications
   */
  void
  clear_callback();

  /**
   * @brief Get the port object
   * @return GPIO_TypeDef*
   */
  inline GPIO_TypeDef *
  get_port() const
  {
    return port;
  }

  /**
   * @brief Get the pin index within the port
   * @return uint8_t Pin index (0..15)
   */
  inline uint8_t
  get_index() const
  {
    return index;
  }

  /**
   * @brief Get the pin configuration flags
   * @return PinCfg Configuration bitmask for this pin
   */
  inline PinCfg
  get_cfg() const
  {
    return cfg;
  }

  /**
   * @brief Check if the pin is currently enabled
   * @return true if the pin is enabled, false otherwise
   */
  inline bool
  is_enabled() const
  {
    return enabled;
  }

  /**
   * @brief Initializes the pin hardware configuration
   */
  int
  enable();

  /**
   * @brief Deinitializes the pin and releases resources
   */
  int
  disable();

  /**
   * @brief Reads the digital state of the pin
   * @param value Pointer to store the read value (0 or 1)
   * @return 0 on success, error code on failure
   */
  int
  read(uint8_t *value);

  /**
   * @brief Writes the digital state of the pin
   * @param value Digital value to write (0 for low, non-zero for high)
   * @return 0 on success, error code on failure
   */
  int
  write(uint8_t value);

private:
  friend class BusCore;

  bool enabled;

  BusCore *bus;         ///< GPIO controller managing this pin
  GPIO_TypeDef *port;   ///< GPIO port (GPIOA, GPIOB, …)
  uint8_t index;        ///< Pin index within the port (0..15)
  PinCfg cfg;           ///< Pin configuration flags (PinCfg bitmask)
  PwmBinding pwm;       ///< PWM timer binding for output pins
  Callback<uint8_t> cb; ///< Interrupt event callback

  bool has_init_value = false; ///< Whether an initial output value has been set
  uint8_t init_value = 0;      ///< Initial output value

  /**
   * @brief Executes the interrupt callback with specified value
   * @param value Pin state that triggered the interrupt (0 or 1)
   * @note Called automatically by GPIO controller during interrupt processing
   */
  void
  trigger();
};

}; // namespace Embys::Stm32::Gpio
