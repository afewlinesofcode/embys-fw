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

#include <embys/stm32/types.hpp>

#include "core.hpp"

namespace Embys::Stm32::Sim::I2C
{

/**
 * @brief Pointer to the I2C peripheral instance being used in the mock
 * environment.
 */
extern I2C_TypeDef *i2c;

/**
 * @brief Address of the current I2C peripheral.
 */
extern std::optional<uint8_t> addr;

/**
 * @brief Buffers for storing data written by the I2C peripheral.
 */
extern std::vector<std::vector<uint8_t>> tx_buffers;

/**
 * @brief Callback for handling data transmission in the I2C peripheral.
 * The callback takes the I2C address and the data bytes that were "sent" by the
 * peripheral.
 */
extern Callback<uint8_t, std::vector<uint8_t>> on_tx;

/**
 * @brief Simulate receiving data on the I2C bus.
 * @param data The data to be received on the I2C bus.
 */
void
simulate_rx(std::vector<uint8_t> data);

/**
 * @brief Register a simulated response for a given I2C address and register.
 * Whenever a read to @p addr starts (optionally preceded by a write of @p reg),
 * @p data is automatically injected into the rx buffers.
 * Use @p reg = 0 for plain reads with no preceding register write.
 * @param addr  7-bit I2C device address.
 * @param reg   Register byte written before the read (0 if none).
 * @param data  Bytes to return on each matching read.
 */
void
simulate_response(uint8_t addr, uint8_t reg, std::vector<uint8_t> data);

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
