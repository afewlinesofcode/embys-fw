/**
 * @file uart.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief UART simulation for the STM32 mock environment.
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

#include "core.hpp"

namespace Embys::Stm32::Sim::Uart
{

/**
 * @brief Pointer to the USART peripheral instance being used in the mock
 * environment.
 */
extern USART_TypeDef *usart;

/**
 * @brief Buffers for simulating data transmission. Each buffer represents a
 * separate transmission that has been initiated by writing to DR.
 */
extern std::vector<std::vector<uint8_t>> tx_buffers;

/**
 * @brief Simulate receiving data on the UART.
 * @param data The data to be received on the UART.
 */
void
simulate_rx(std::vector<uint8_t> data);

/**
 * @brief Reset the UART simulation state, including the runtime state and
 * resetting the USART pointer to the default instance (usart1).
 */
void
reset();

} // namespace Embys::Stm32::Sim::Uart
