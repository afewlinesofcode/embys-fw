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
#include "resource.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::Gpio
{

class BusCore;
class Api;

class PinCore
{
public:
  PinCore(const PinCore &) = delete;
  PinCore(PinCore &&) = delete;
  PinCore &
  operator=(const PinCore &) = delete;
  PinCore &
  operator=(PinCore &&) = delete;

  /**
   * @brief Destroy the Pin object and release resources
   */
  ~PinCore();

  /**
   * @brief Set the initial output value for this pin. This value will be
   * applied when the pin is enabled if it is configured as an output.
   * @param value Initial output value (0 for low, non-zero for high)
   */
  void
  set_init_value(uint8_t value);

  /**
   * @brief Bind a PWM timer to this pin for PWM output
   * @param timer Timer used for PWM generation.
   */
  template <uint8_t Channel>
  void
  bind_pwm(Base::Timer &timer)
  {
    static_assert(Channel >= 1 && Channel <= 4,
                  "PWM channel must be in the range 1..4");
    bind_pwm_impl(timer, Channel);
  }

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
  [[nodiscard]] Status
  enable();

  /**
   * @brief Deinitializes the pin and releases resources
   */
  [[nodiscard]] Status
  disable();

  /**
   * @brief Reads the digital state of the pin
   * @return Current pin state or a hardware error.
   */
  [[nodiscard]] ReadResult
  read();

  /**
   * @brief Writes the digital state of the pin
   * @param value Digital value to write (0 for low, non-zero for high)
   * @return 0 on success, error code on failure
   */
  [[nodiscard]] Status
  write(uint8_t value);

private:
  friend class BusCore;

protected:
  /**
   * @brief Construct the capacity-independent core for a typed pin.
   * @param bus GPIO bus managing this pin.
   * @param port GPIO peripheral selected by the Pin template.
   * @param index Compile-time pin index in the range 0..15.
   * @param cfg Compile-time pin configuration.
   */
  PinCore(BusCore &bus, GPIO_TypeDef *port, uint8_t index, PinCfg cfg);

private:
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

  void
  bind_pwm_impl(Base::Timer &timer, uint8_t channel);
};

template <Port SelectedPort, uint8_t Index, PinCfg Config>
class Pin final : public PinCore
{
  static_assert(Index < 16, "GPIO pin index must be in the range 0..15");
  static_assert(port_available<Stm32::TargetDevice, SelectedPort>,
                "Selected GPIO port is not available on this target");
  static_assert(config_valid<Config>, "Invalid GPIO pin configuration");

public:
  static constexpr Port port = SelectedPort;
  static constexpr uint8_t index = Index;
  static constexpr PinCfg config = Config;

  explicit Pin(BusCore &bus)
    : PinCore(bus, port_address<SelectedPort>(), Index, Config)
  {
  }
};

template <Port SelectedPort, uint8_t Index, PinCfg Options = PinCfg::NONE>
using InputPin = Pin<SelectedPort, Index, PinCfg::IN | Options>;

template <Port SelectedPort, uint8_t Index, PinCfg Options = PinCfg::NONE>
using OutputPin = Pin<SelectedPort, Index, PinCfg::OUT | Options>;

template <Port SelectedPort, uint8_t Index, PinCfg Options = PinCfg::NONE>
using UartPin = Pin<SelectedPort, Index, PinCfg::UART | Options>;

template <Port SelectedPort, uint8_t Index, PinCfg Options = PinCfg::NONE>
using I2cPin = Pin<SelectedPort, Index, PinCfg::I2C | Options>;

}; // namespace Embys::Stm32::Gpio
