#pragma once

#include <stdint.h>

#include <embys/stm32/types.hpp>

#include "api.hpp"
#include "def.hpp"
#include "stm32f1xx.hpp"

namespace Embys::Stm32::I2c
{

/**
 * @brief Interrupt-driven I2C state machine.
 *
 * Manages a single I2C transaction (read, register-addressed read, or write)
 * driven entirely by EV and ER interrupts. No blocking waits.
 *
 * The owning Bus calls:
 *   handle_irq()   — from I2Cx_EV_IRQHandler
 *   handle_error() — from I2Cx_ER_IRQHandler
 *   complete()     — from main-loop context after is_result_ready() is true
 *   force_timeout()— from Bus timeout handler (EV_RT event)
 */
class Sm
{
public:
  Sm() = delete;
  Sm(const Sm &) = delete;
  Sm(Sm &&) = delete;
  Sm &
  operator=(const Sm &) = delete;
  Sm &
  operator=(Sm &&) = delete;

  explicit Sm(I2C_TypeDef *i2c);

  /**
   * @brief Start an asynchronous read of len bytes from addr7.
   * Issues a START condition; completion delivered via cb.
   */
  int
  start_read(Callable<int> cb, uint8_t addr7, uint8_t *buf, uint16_t len);

  /**
   * @brief Start an asynchronous register-addressed read.
   * Writes reg in a first frame, issues a repeated START, then reads len bytes.
   */
  int
  start_read(Callable<int> cb, uint8_t addr7, uint8_t reg, uint8_t *buf,
             uint16_t len);

  /**
   * @brief Start an asynchronous write of len bytes to addr7.
   */
  int
  start_write(Callable<int> cb, uint8_t addr7, const uint8_t *buf,
              uint16_t len);

  /** @brief Process an I2C event interrupt. */
  void
  handle_irq();

  /** @brief Process an I2C error interrupt. */
  void
  handle_error();

  /** @brief True if the transaction has completed (success or error). */
  inline bool
  is_result_ready() const
  {
    return result_ready;
  }

  /**
   * @brief Deliver the result to the callback. Must be called from main-loop
   * context after is_result_ready() returns true.
   */
  void
  complete();

  /**
   * @brief Force a timeout error. Called from the Bus timeout handler when the
   * EV_RT event fires before the transaction finishes naturally.
   */
  void
  force_timeout();

private:
  enum class Direction : uint8_t
  {
    Write,
    Read
  };

  enum class State : uint8_t
  {
    Idle,
    Start,
    Address,
    WriteReg,
    WriteData,
    ReadData
  };

  volatile State state = State::Idle;
  I2C_TypeDef *i2c;
  Callable<int> cb;

  volatile uint8_t addr7 = 0;
  volatile Direction dir = Direction::Write;
  volatile bool reg_and_restart = false;
  volatile uint8_t reg = 0;

  // Two separate buffer pointers; only one is active per direction.
  volatile uint8_t *rx_buf = nullptr;
  volatile const uint8_t *tx_buf = nullptr;
  volatile uint16_t buf_len = 0;
  volatile uint16_t buf_pos = 0;

  volatile bool result_ready = false;
  volatile int result = 0;

  void
  handle_start();

  void
  handle_address();

  void
  handle_write_reg();

  void
  handle_write_data();

  void
  handle_read_data();

  void
  handle_read_data_1();

  void
  handle_read_data_2();

  void
  handle_read_data_n();

  void
  done();

  void
  error(int result_code);
};

}; // namespace Embys::Stm32::I2c
