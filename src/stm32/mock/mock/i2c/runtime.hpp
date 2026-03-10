/**
 * @file runtime.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Runtime state and hooks for the I2C simulation in the STM32 mock
 * environment.
 *
 * Requires:
 * - "i2c_read_dr" test hook to be called when DR register is read
 * - "i2c_write_dr" test hook to be called when DR register is written to
 * - "i2c_read_sr1" test hook to be called when SR1 register is read
 * - "i2c_read_sr2" test hook to be called when SR2 register is read
 *
 * Example:
 * If you have defined in your code:
 * ```cpp
 * #ifdef MOCK_STM32
 * #define TEST_HOOK(key) Embys::Stm32::Mock::Base::trigger_test_hook(key)
 * #else
 * #define TEST_HOOK(key)
 * #endif
 * ```
 * And then you signal DR register read with:
 * ```cpp
 * uint8_t read_dr() {
 *   auto dr = I2C1->DR;
 *   TEST_HOOK("i2c_read_dr");
 *   return dr;
 * }
 * ```
 *
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <functional>
#include <optional>
#include <vector>

#include "../core.hpp"

namespace Embys::Stm32::Mock::I2C
{

/**
 * @brief Runtime state for the I2C simulation, including the current condition
 * of the I2C peripheral, shift register state, read/write buffers, and flags
 * for managing the simulation behavior.
 */
struct Runtime
{
  /**
   * @brief Condition of the I2C peripheral, used to track whether it is idle,
   * starting a transaction, reading data, or writing data.
   */
  enum Condition
  {
    Idle,
    Starting,
    Reading,
    Writing
  } condition;

  /**
   * @brief Condition that the I2C peripheral is expecting after START.
   */
  Condition expecting_condition;

  /**
   * @brief Flag indicating whether the I2C peripheral is waiting for a clear
   * condition (SR1 has been read).
   */
  bool wait_clear;

  /**
   * @brief Address of the current I2C peripheral.
   */
  std::optional<uint8_t> addr;

  /**
   * @brief Shift register for simulating data reception and transmission.
   */
  uint8_t sr;

  /**
   * @brief Flag indicating shift register is full.
   */
  bool sr_full;

  /**
   * @brief Flag indicating shift register is currently receiving data.
   */
  bool sr_receiving;

  /**
   * @brief Flag indicating shift register is currently sending data.
   */
  bool sr_sending;

  /**
   * @brief Countdown for simulating NACK when ACK is disabled.
   */
  uint8_t nack_countdown;

  /**
   * @brief Buffers for storing data to be read by the I2C peripheral.
   */
  std::vector<std::vector<uint8_t>> read_buffers;

  /**
   * @brief Position within the current read buffer.
   */
  uint8_t read_buffer_pos;

  /**
   * @brief Buffers for storing data written by the I2C peripheral.
   */
  std::vector<std::vector<uint8_t>> write_buffers;

  /**
   * @brief Whether to block SB flag from being set to simulate a stuck bus or
   * unresponsive slave.
   */
  bool block_sb;

  /**
   * @brief Whether to block ADDR flag from being set to simulate a slave that
   * does not acknowledge its address.
   */
  bool block_addr;

  /**
   * @brief Reset the I2C runtime state to its initial conditions. This should
   * be called at the beginning of each test to ensure a clean testing
   * environment.
   */
  void
  reset();
};

extern Runtime runtime;
}; // namespace Embys::Stm32::Mock::I2C
