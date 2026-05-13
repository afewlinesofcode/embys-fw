/**
 * @file spi.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief SPI simulation for the STM32 simulated environment.
 * @version 0.1
 * @date 2026-05-11
 * @copyright Copyright (c) 2026
 */

#include "spi.hpp"

#include <optional>

#include "base.hpp"

namespace Embys::Stm32::Sim::SPI
{

SPI_TypeDef *spi = &spi1_instance; // Default to spi1

std::vector<std::vector<uint8_t>> tx_buffers;

Callable<std::vector<uint8_t>> on_tx;

/**
 * @brief Number of cycles after a DR write before TXE is set again,
 * simulating the time to shift the byte out.
 */
static constexpr uint32_t TXE_DELAY = 5;

/**
 * @brief Number of cycles after a DR write before RXNE is set,
 * simulating the full-duplex receive completing.
 */
static constexpr uint32_t RXNE_DELAY = 10;

/**
 * @brief Number of cycles after the last activity before BSY is cleared.
 */
static constexpr uint32_t BSY_DELAY = 15;

/**
 * @brief Condition of the SPI peripheral, tracking whether it is idle or
 * actively transferring (SPE bit set).
 */
enum Condition
{
  Idle,
  Transferring
};

static Condition condition = Idle;

/**
 * @brief Target cycle at which TXE should be set again after a write to DR.
 * std::nullopt when no TXE restoration is pending.
 */
static std::optional<uint32_t> txe_cyc;

/**
 * @brief Target cycle at which RXNE should be set after a write to DR.
 * std::nullopt when no RXNE is pending.
 */
static std::optional<uint32_t> rxne_cyc;

/**
 * @brief Target cycle at which BSY should be cleared after activity.
 * std::nullopt when no BSY clear is pending.
 */
static std::optional<uint32_t> bsy_cyc;

/**
 * @brief Queue of receive buffers to be returned during DR reads.
 * Each entry corresponds to one simulated transaction response.
 */
static std::vector<std::vector<uint8_t>> rx_buffers;

/**
 * @brief Byte position within the current transaction, incremented on each
 * DR write and used to index the matching byte in the rx_buffer.
 */
static uint16_t buffer_pos = 0;

void
simulate_rx(std::vector<uint8_t> data)
{
  rx_buffers.push_back(std::move(data));
}

/**
 * @brief Hook called when "spi_begin" is triggered (CS asserted).
 * Starts a new transaction buffer and updates SR flags.
 */
void
begin_hook(uint32_t)
{
  if (condition != Transferring)
    return;

  tx_buffers.emplace_back();
  buffer_pos = 0;

  CLEAR_BIT_V(spi->SR, SPI_SR_RXNE);
  SET_BIT_V(spi->SR, SPI_SR_BSY);
}

/**
 * @brief Hook called when "spi_end" is triggered (CS deasserted).
 * Finalises the transaction and fires the on_tx callback.
 */
void
end_hook(uint32_t)
{
  if (condition != Transferring || tx_buffers.empty())
    return;

  if (!rx_buffers.empty())
    rx_buffers.erase(rx_buffers.begin());

  on_tx(tx_buffers.back());
}

/**
 * @brief Hook called when "spi_write_dr" is triggered (byte written to DR).
 * Captures the transmitted byte and schedules flag updates.
 */
void
write_dr_hook(uint32_t cyc)
{
  if (condition != Transferring)
    return;

  if (tx_buffers.empty())
    tx_buffers.emplace_back();

  tx_buffers.back().push_back(static_cast<uint8_t>(spi->DR));
  INC_V(buffer_pos);

  CLEAR_BIT_V(spi->SR, SPI_SR_TXE);
  SET_BIT_V(spi->SR, SPI_SR_BSY);

  txe_cyc = cyc + TXE_DELAY;
  rxne_cyc = cyc + RXNE_DELAY;
  bsy_cyc = cyc + BSY_DELAY;
}

/**
 * @brief Hook called when "spi_read_dr" is triggered (DR register read).
 * Provides the next simulated receive byte and updates flags.
 */
void
read_dr_hook(uint32_t cyc)
{
  if (condition != Transferring)
    return;

  if (rx_buffers.empty())
  {
    spi->DR = 0;
    return;
  }

  auto &rx_buffer = rx_buffers.front();
  auto pos = buffer_pos > 0 ? static_cast<size_t>(buffer_pos - 1) : 0u;

  spi->DR = pos < rx_buffer.size() ? static_cast<uint8_t>(rx_buffer[pos]) : 0u;

  CLEAR_BIT_V(spi->SR, SPI_SR_RXNE);
  bsy_cyc = cyc + BSY_DELAY;
}

/**
 * @brief Persistent hook called every cycle.
 * Detects SPE enable/disable transitions and restores delayed flags.
 */
void
peripheral_hook(uint32_t cyc)
{
  if ((spi->CR1 & SPI_CR1_SPE) && condition == Idle)
  {
    condition = Transferring;
    SET_BIT_V(spi->SR, SPI_SR_TXE);
    CLEAR_BIT_V(spi->SR, SPI_SR_RXNE | SPI_SR_BSY);
  }
  else if (!(spi->CR1 & SPI_CR1_SPE) && condition == Transferring)
  {
    condition = Idle;
    CLEAR_BIT_V(spi->SR, SPI_SR_TXE | SPI_SR_RXNE | SPI_SR_BSY);
    txe_cyc = std::nullopt;
    rxne_cyc = std::nullopt;
    bsy_cyc = std::nullopt;
  }

  if (condition != Transferring)
    return;

  if (txe_cyc.has_value() && cyc >= txe_cyc.value())
  {
    SET_BIT_V(spi->SR, SPI_SR_TXE);
    txe_cyc = std::nullopt;
  }

  if (rxne_cyc.has_value() && cyc >= rxne_cyc.value())
  {
    SET_BIT_V(spi->SR, SPI_SR_RXNE);
    rxne_cyc = std::nullopt;
  }

  if (bsy_cyc.has_value() && cyc >= bsy_cyc.value())
  {
    CLEAR_BIT_V(spi->SR, SPI_SR_BSY);
    bsy_cyc = std::nullopt;
  }
}

void
reset()
{
  spi = &spi1_instance;
  condition = Idle;
  txe_cyc = std::nullopt;
  rxne_cyc = std::nullopt;
  bsy_cyc = std::nullopt;
  buffer_pos = 0;
  rx_buffers.clear();
  tx_buffers.clear();

  on_tx.clear();

  spi->DR = 0;
  spi->SR = SPI_SR_TXE; // Idle: transmit buffer empty

  Base::add_test_hook("spi_begin", begin_hook);
  Base::add_test_hook("spi_end", end_hook);
  Base::add_test_hook("spi_write_dr", write_dr_hook);
  Base::add_test_hook("spi_read_dr", read_dr_hook);
  Base::add_hook(peripheral_hook);
}

} // namespace Embys::Stm32::Sim::SPI
