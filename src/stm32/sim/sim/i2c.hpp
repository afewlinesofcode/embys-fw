/**
 * @file i2c.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C simulation for the STM32 mock environment.
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
 * #define TEST_HOOK(key) Embys::Stm32::Sim::Base::trigger_test_hook(key)
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

#include <map>
#include <optional>
#include <vector>

#include "core.hpp"

namespace Embys::Stm32::Sim::I2C
{

/**
 * @brief Pointer to the I2C peripheral instance being used in the mock
 * environment.
 */
extern I2C_TypeDef *i2c;

/**
 * @brief Simulate receiving data on the I2C bus.
 * @param data The data to be received on the I2C bus.
 */
void
simulate_recv(std::vector<uint8_t> data);

/**
 * @brief Simulate the I2C bus being busy by setting the BUSY flag in the SR2
 * register. This can be used to test how code handles a busy I2C bus condition.
 */
void
simulate_busy();

/**
 * @brief Reset the I2C simulation state, including the runtime state and
 * resetting the I2C pointer to the default instance (i2c1).
 */
void
reset();

}; // namespace Embys::Stm32::Sim::I2C
