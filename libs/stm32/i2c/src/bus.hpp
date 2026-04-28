#pragma once

#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/types.hpp>

#include "api.hpp"
#include "def.hpp"
#include "sm.hpp"
#include "stm32f1xx.hpp"

namespace Embys::Stm32::I2c
{

void
ev_irq_handler(uint8_t index);

void
er_irq_handler(uint8_t index);

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
 * - Wiring I2Cx_EV_IRQHandler → ev_irq_handler(n)
 * - Wiring I2Cx_ER_IRQHandler → er_irq_handler(n)
 * - Provisioning one extra event slot in the Loop for the timeout event
 *
 * Example:
 * ```
 * I2c::Bus bus(I2C1, &loop);
 * bus.enable(400000);
 *
 * void I2C1_EV_IRQHandler() { I2c::ev_irq_handler(0); }
 * void I2C1_ER_IRQHandler() { I2c::er_irq_handler(0); }
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
  Bus(I2C_TypeDef *i2c, Base::Loop *base);
  ~Bus();

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
  read(uint8_t addr7, uint8_t *buf, uint16_t len, Callable<int> cb);

  /**
   * @brief Asynchronous register-addressed read.
   * Writes reg in the first frame, issues a repeated START, then reads.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len,
       Callable<int> cb);

  /**
   * @brief Asynchronous write of len bytes to I2C address addr7.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  write(uint8_t addr7, const uint8_t *buf, uint16_t len, Callable<int> cb);

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

private:
  friend void
  ev_irq_handler(uint8_t);

  friend void
  er_irq_handler(uint8_t);

  I2C_TypeDef *i2c;
  Base::Loop *base;
  Sm sm;
  Base::Event timeout_event;
  Base::Module *module = nullptr;
  uint32_t scl_hz = 0u;
  bool enabled = false;

  void
  module_notify();

  uint32_t
  calc_timeout_us(uint16_t len) const;

  static void
  module_handler(void *context);

  static void
  timeout_handler(void *context);
};

extern Bus *instances[2];

}; // namespace Embys::Stm32::I2c
