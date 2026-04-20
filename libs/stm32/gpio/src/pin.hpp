#pragma once

#include <embys/stm32/types.hpp>

#include "stm32f1xx.hpp"

namespace Embys::Stm32::Gpio
{

class Bus;
class Api;

class Pin
{
public:
  Pin(Bus *bus, GPIO_TypeDef *port, uint8_t index, uint32_t gpio_mode,
      uint32_t gpio_cnf, uint8_t pin_cfg);

  inline GPIO_TypeDef *
  get_port() const
  {
    return port;
  }

  inline uint8_t
  get_index() const
  {
    return index;
  }

  inline uint32_t
  get_gpio_cfg() const
  {
    return gpio_cfg;
  }

  inline uint8_t
  get_pin_cfg() const
  {
    return pin_cfg;
  }

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

  /**
   * @brief Registers a callback for interrupt events
   * @param cb Callback function to execute on interrupt events
   * @return 0 on success, error code on failure
   */
  int
  set_callback(Callable<uint8_t> cb);

  /**
   * @brief Removes the interrupt callback
   * @return 0 on success, error code if no callback was registered
   */
  int
  clear_callback();

private:
  friend class Bus;

  bool enabled;
  Bus *bus; ///< GPIO controller managing this pin
  Api *api; ///< GPIO bus API
  GPIO_TypeDef *port;
  uint8_t index;
  uint32_t gpio_cfg;
  uint8_t pin_cfg;
  Callable<uint8_t> cb; ///< Interrupt event callback

  /**
   * @brief Executes the interrupt callback with specified value
   * @param value Pin state that triggered the interrupt (0 or 1)
   * @note Called automatically by GPIO controller during interrupt processing
   */
  void
  trigger();

  /** @brief Validates pin configuration for hardware compatibility */
  int
  validate_config();
};

}; // namespace Embys::Stm32::Gpio
