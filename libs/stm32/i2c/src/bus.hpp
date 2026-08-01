/**
 * @file bus.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C Bus managing I2C peripheral communication
 *
 * The Bus class provides the main interface for I2C communication,
 * integrating the interrupt-driven state machine with the main Loop.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/types.hpp>

#include "def.hpp"
#include "sm.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::I2c
{

/**
 * @class Bus
 * @brief Interrupt-driven I2C master for STM32F1.
 *
 * Integrates with Base::Loop via a Module (deferred completion callbacks) and
 * an EV_RT Event (transaction timeout).
 *
 * GPIO configuration for SCL and SDA pins must be done by the caller before
 * calling enable() (open-drain AF output, appropriate speed).
 *
 * The caller is responsible for:
 * - Configuring SCL/SDA GPIO pins
 * - Enabling NVIC for I2Cx_EV_IRQn and I2Cx_ER_IRQn
 * - Wiring I2Cx_EV_IRQHandler → bus.handle_ev_irq()
 * - Wiring I2Cx_ER_IRQHandler → bus.handle_er_irq()
 * - Provisioning one extra event slot in the Loop for the timeout event
 *
 * Example:
 * ```
 * I2c::Bus bus(I2C1, &loop);
 * bus.enable(400000);
 *
 * void I2C1_EV_IRQHandler() { bus.handle_ev_irq(); }
 * void I2C1_ER_IRQHandler() { bus.handle_er_irq(); }
 * ```
 */
class Bus
{
public:
  Bus() = delete;
  Bus(const Bus &) = delete;
  Bus(Bus &&) = delete;
  Bus &
  operator=(const Bus &) = delete;
  Bus &
  operator=(Bus &&) = delete;

  /**
   * @brief Construct an I2C Bus.
   * @param i2c  Peripheral instance (I2C1 or I2C2).
   * @param base Main loop for module and event registration.
   */
  Bus(I2C_TypeDef *i2c, Base::LoopCore *base);
  ~Bus();

  inline I2C_TypeDef *
  get_i2c() const
  {
    return i2c;
  }

  inline Base::LoopCore *
  get_base() const
  {
    return base;
  }

  inline uint32_t
  get_scl_hz() const
  {
    return scl_hz;
  }

  inline bool
  is_enabled() const
  {
    return enabled;
  }

  /**
   * @brief Enable the I2C peripheral and register with the loop.
   * If the bus is found busy on entry, a recovery attempt is made.
   * @param scl_hz SCL frequency in Hz (default 100 kHz).
   * @return 0 on success, negative error code on failure.
   */
  int
  enable(uint32_t scl_hz = 100000u);

  /**
   * @brief Disable the I2C peripheral and unregister from the loop.
   * @return 0 on success, negative error code on failure.
   */
  int
  disable();

  /**
   * @brief Asynchronous read of len bytes from I2C address addr7.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  read(uint8_t addr7, uint8_t *buf, uint16_t len, Callback<int> cb);

  /**
   * @brief Asynchronous register-addressed read.
   * Writes reg in the first frame, issues a repeated START, then reads.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len,
       Callback<int> cb);

  /**
   * @brief Asynchronous write of len bytes to I2C address addr7.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  write(uint8_t addr7, const uint8_t *buf, uint16_t len, Callback<int> cb);

  /**
   * @brief I2C event IRQ handler — call from I2Cx_EV_IRQHandler.
   */
  void
  handle_ev_irq();

  /**
   * @brief I2C error IRQ handler — call from I2Cx_ER_IRQHandler.
   */
  void
  handle_er_irq();

  void
  set_module_pending()
  {
    base->set_module_pending(module);
  }

private:
  I2C_TypeDef *i2c;
  Base::LoopCore *base;
  Sm sm;
  Base::Module *module = nullptr;
  uint32_t scl_hz = 0u;
  bool enabled = false;
  Callback<int> cb;

  static void
  module_callback(void *context);
};

}; // namespace Embys::Stm32::I2c
