/**
 * @file runtime.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief Runtime state for the UART peripheral simulation.
 *
 * Requires:
 * - "uart_read_dr" test hook to be called when DR register is read
 * - "uart_write_dr" test hook to be called when DR register is written to
 * - "uart_read_sr" test hook to be called when SR register is read
 *
 * Example:
 * If you have defined in your code:
 * ```cpp
 * #ifdef MOCK_STM32
 * #define TEST_HOOK(key) Embys::Stm32::Sim::Base::trigger_test_hook(key)
 * #else
 * #define TEST_HOOK(key)
 * #endif
 * ```
 * And then you signal DR register read with:
 * ```cpp
 * uint8_t read_dr() {
 *   auto dr = USART1->DR;
 *   TEST_HOOK("uart_read_dr");
 *   return dr;
 * }
 * ```
 *
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <vector>

#include "../core.hpp"

namespace Embys::Stm32::Sim::Uart
{

/**
 * @brief Runtime state for the UART peripheral simulation, including status
 * register flags, buffers for simulating data transmission and reception, and
 * methods for resetting the runtime state.
 */
struct Runtime
{
  /**
   * @brief Shift register for simulating data reception and transmission.
   */
  uint8_t sr;

  /**
   * @brief Flag indicating shift register is currently receiving data.
   */
  bool sr_receiving;

  /**
   * @brief Flag indicating shift register is currently sending data.
   */
  bool sr_sending;

  /**
   * @brief Flag indicating shift register is full.
   */
  bool sr_full;

  /**
   * @brief Flag indicating a transmission is currently active (data has been
   * written to DR but TC has not been set yet).
   */
  bool tx_active;

  /**
   * @brief Flag indicating SR register has been read and is waiting for the
   * next action to clear error flags.
   */
  bool wait_clear;

  /**
   * @brief Number of cycles until TC flag is set.
   *
   */
  uint32_t tc_cyc;

  /**
   * @brief Buffer for simulating data reception. Data in this buffer will be
   * transferred to the DR register when simulating reception.
   */
  std::vector<uint8_t> read_buffer;

  /**
   * @brief Buffers for simulating data transmission. Each buffer represents a
   * separate transmission that has been initiated by writing to DR.
   */
  std::vector<std::vector<uint8_t>> write_buffers;

  /**
   * @brief Pointer to the current write buffer being transmitted.
   */
  std::vector<uint8_t> *current_write_buffer;

  /**
   * @brief Current position within the current read buffer.
   */
  uint16_t read_buffer_pos = 0;

  /**
   * @brief Reset the UART runtime state to its initial conditions. This should
   * be called at the beginning of each test to ensure a clean testing
   * environment.
   */
  void
  reset();
};

extern Runtime runtime;

} // namespace Embys::Stm32::Sim::Uart
