/**
 * @file spi.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief SPI simulation for the STM32 mock environment.
 *
 * Requires:
 * - "spi_write_dr" test hook to be called when DR register is written to
 * - "spi_read_dr" test hook to be called when DR register is read
 * - "spi_begin" test hook to be called when CS is asserted (transaction start)
 * - "spi_end" test hook to be called when CS is deasserted (transaction end)
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
 * And then you signal CS assert / DR write with:
 * ```cpp
 * void begin() {
 *   cs_low();
 *   TEST_HOOK("spi_begin");
 * }
 * uint8_t transfer(uint8_t byte) {
 *   SPI1->DR = byte;
 *   TEST_HOOK("spi_write_dr");
 *   while (!(SPI1->SR & SPI_SR_RXNE)) {}
 *   auto rx = SPI1->DR;
 *   TEST_HOOK("spi_read_dr");
 *   return rx;
 * }
 * void end() {
 *   TEST_HOOK("spi_end");
 *   cs_high();
 * }
 * ```
 *
 * @version 0.1
 * @date 2026-05-11
 * @copyright Copyright (c) 2026
 */

#pragma once

#include <vector>

#include <embys/stm32/types.hpp>

#include "core.hpp"

namespace Embys::Stm32::Sim::SPI
{

/**
 * @brief Pointer to the SPI peripheral instance being used in the mock
 * environment.
 */
extern SPI_TypeDef *spi;

/**
 * @brief Buffers for simulating data transmission. Each buffer represents a
 * separate CS transaction initiated by a "spi_begin" hook.
 */
extern std::vector<std::vector<uint8_t>> tx_buffers;

/**
 * @brief Callback invoked when a SPI transaction ends ("spi_end" hook fires).
 * Receives the bytes that were transmitted during the transaction.
 */
extern Callable<std::vector<uint8_t>> on_tx;

/**
 * @brief Simulate receiving data on the SPI bus.
 * Bytes in @p data will be returned (one per DR read, indexed by the current
 * byte position within the transaction) during the next transaction.
 * @param data The data to be received.
 */
void
simulate_rx(std::vector<uint8_t> data);

/**
 * @brief Reset the SPI simulation state, including the runtime state and
 * resetting the SPI pointer to the default instance (spi1).
 */
void
reset();

} // namespace Embys::Stm32::Sim::SPI
