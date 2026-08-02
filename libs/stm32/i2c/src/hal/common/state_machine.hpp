/**
 * @file hal/common/state_machine.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Shared interrupt-driven I2C state machine (STM32F1 / STM32F4 / sim).
 *
 * Implements the I2C protocol state machine driven by EV and ER interrupt
 * handlers for the classic SR1/SR2/DR layout. Supports single-byte,
 * two-byte, and N-byte read sequences with register address and repeated-start,
 * plus write operations.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include "../../stm32xx.hpp"

#ifdef EMBYS_I2C_CLASSIC_REGISTERS

#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/types.hpp>

#include "../../wait_bus.hpp"

namespace Embys::Stm32::I2c
{

class BusCore;

/**
 * @brief Interrupt-driven state machine for the shared classic I2C layout.
 *
 * Manages a single I2C transaction (read, register-addressed read, or write)
 * driven entirely by EV and ER interrupts. No blocking waits.
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

  Sm(BusCore *bus);

  /**
   * @brief Start an asynchronous read of len bytes from addr7.
   * Issues a START condition; completion delivered via cb.
   */
  [[nodiscard]] Status
  start_read(uint8_t addr7, uint8_t *buf, uint16_t len);

  /**
   * @brief Start an asynchronous register-addressed read.
   * Writes reg in a first frame, issues a repeated START, then reads len bytes.
   */
  [[nodiscard]] Status
  start_read(uint8_t addr7, uint8_t reg, uint8_t *buf, uint16_t len);

  /**
   * @brief Start an asynchronous write of len bytes to addr7.
   */
  [[nodiscard]] Status
  start_write(uint8_t addr7, const uint8_t *buf, uint16_t len);

  /** @brief Process an I2C event interrupt. */
  void
  handle_irq();

  /** @brief Process an I2C error interrupt. */
  void
  handle_error();

  /** @brief True if the transaction has completed (success or error). */
  inline bool
  is_complete() const
  {
    return state == State::Stop || state == State::Error;
  }

  inline Status
  get_result() const
  {
    return succeeded ? Status::success() : Status::failure(error_code);
  }

  /**
   * @brief Deliver the result to the callback. Must be called from main-loop
   * context after is_complete() returns true.
   */
  void
  reset();

private:
  enum class Direction : uint8_t
  {
    Write,
    Read
  };

  enum class State : uint8_t
  {
    Idle,
    WaitBus,
    Start,
    Address,
    WriteReg,
    WriteData,
    ReadData,
    Stop,
    Error
  };

  volatile State state = State::Idle;
  BusCore *bus;
  I2C_TypeDef *i2c;
  WaitBus wait_bus;
  Base::Event timeout_event;

  volatile uint8_t addr7 = 0;
  volatile Direction dir = Direction::Write;
  volatile bool reg_and_restart = false;
  volatile uint8_t reg = 0;

  // Two separate buffer pointers; only one is active per direction.
  volatile uint8_t *rx_buf = nullptr;
  volatile const uint8_t *tx_buf = nullptr;
  volatile uint16_t buf_len = 0;
  volatile uint16_t buf_pos = 0;

  volatile bool succeeded = false;
  volatile Error error_code = Error::InvalidState;

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
  stop();

  void
  done();

  void
  error(Error error);

  void
  start();

  static void
  timeout_handler(void *context) noexcept;

  static void
  wait_bus_callback(void *context, Status result) noexcept;
};

}; // namespace Embys::Stm32::I2c

#endif // EMBYS_I2C_CLASSIC_REGISTERS
