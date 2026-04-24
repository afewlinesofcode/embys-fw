/**
 * @file uart.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief UART simulation for the STM32 simulated environment.
 * @version 0.1
 * @date 2026-03-17
 * @copyright Copyright (c) 2026
 */

#include "uart.hpp"

#include <optional>

#include "base.hpp"

namespace Embys::Stm32::Sim::Uart
{

USART_TypeDef *usart = &usart1_instance; // Default to usart1

std::vector<std::vector<uint8_t>> tx_buffers;

/**
 * @brief RX shift register: holds a byte being transferred from rx_buffer to
 * DR.
 */
static uint8_t rx_sr = 0;

/**
 * @brief Flag indicating RX shift register is currently loaded with a byte
 * being moved into DR.
 */
static bool rx_sr_full = false;

/**
 * @brief Flag indicating the RX shift register is receiving its next byte from
 * the rx_buffer (i.e., the byte has been loaded and is waiting to be moved).
 */
static bool rx_sr_receiving = false;

/**
 * @brief TX shift register: holds a byte copied from DR being transmitted.
 */
static uint8_t tx_sr = 0;

/**
 * @brief Flag indicating the TX shift register is full and awaiting
 * transmission to the tx_buffer.
 */
static bool tx_sr_full = false;

/**
 * @brief Flag indicating the TX shift register is actively sending its byte
 * to the tx_buffer this cycle.
 */
static bool tx_sr_sending = false;

/**
 * @brief Flag indicating a transmission is currently active (data has been
 * written to DR but TC has not been set yet).
 */
static bool tx_active = false;

/**
 * @brief Flag indicating SR register has been read and is waiting for the
 * next action to clear error flags.
 */
static bool wait_clear = false;

/**
 * @brief Absolute cycle count at which TC should be set after transmission.
 * std::nullopt when no TC is pending.
 */
static std::optional<uint32_t> tc_cyc;

/**
 * @brief Buffer for simulating data reception. Data in this buffer will be
 * transferred to the DR register when simulating reception.
 */
static std::vector<uint8_t> rx_buffer;

/**
 * @brief Pointer to the current write buffer being transmitted.
 */
static std::vector<uint8_t> *current_tx_buffer = nullptr;

/**
 * @brief Current position within the current read buffer.
 */
static uint16_t rx_buffer_pos = 0;

void
simulate_rx(std::vector<uint8_t> data)
{
  if (!rx_buffer.empty())
  {
    std::cout
        << "Warning: Simulating UART reception while previous data is still "
           "being processed. New data will overwrite the existing buffer."
        << std::endl;
  }

  rx_buffer = data;
  rx_buffer_pos = 0;
}

/**
 * @brief Helper function to transfer data from the shift register to the DR
 * register and update flags accordingly.
 */
void
transfer_sr_to_dr()
{
  if (!rx_sr_full)
  {
    return;
  }

  if ((usart->SR & USART_SR_RXNE) != 0)
  {
    // DR register is still full, set overrun error
    SET_BIT_V(usart->SR, USART_SR_ORE);
  }
  else
  {
    usart->DR = rx_sr;
    SET_BIT_V(usart->SR, USART_SR_RXNE);
    rx_sr_full = false;
    rx_sr_receiving = true;
    rx_buffer_pos++;
  }
}

/**
 * @brief Hook function called when the DR register is read.
 * Clears RXNE flag and transfers data from shift register to DR if applicable.
 */
void
read_dr_hook(uint32_t)
{
  CLEAR_BIT_V(usart->SR, USART_SR_RXNE);
  transfer_sr_to_dr();

  if (wait_clear)
  {
    wait_clear = false;
    CLEAR_BIT_V(usart->SR,
                USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE);
  }
}

/**
 * @brief Hook function called when the DR register is written to.
 * Clears TXE and TC flags.
 */
void
write_dr_hook(uint32_t)
{
  CLEAR_BIT_V(usart->SR, USART_SR_TXE | USART_SR_TC);
  tx_active = true;
  tc_cyc = std::nullopt;
}

/**
 * @brief Hook function called when the SR register is read.
 * Initiates wait_clear state if any error flags are set.
 */
void
read_sr_hook(uint32_t)
{
  if ((usart->SR & (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) !=
      0)
    wait_clear = true;
}

/**
 * @brief Hook function called to simulate data reception in the UART.
 * Transfers data from the read buffer to the shift register and updates flags
 * accordingly.
 */
void
receiver_hook(uint32_t)
{
  if (rx_sr_receiving)
  {
    if (rx_buffer.empty())
    {
      rx_sr_receiving = false;
    }
    else if (rx_buffer_pos >= rx_buffer.size())
    {
      // No more bytes in read buffer, reception complete

      rx_sr_receiving = false;
      rx_buffer.clear();
      rx_buffer_pos = 0;
    }
    else
    {
      // Byte received in shift register, move it to rx_sr and mark as full

      rx_sr = rx_buffer[rx_buffer_pos];
      rx_sr_full = true;
      rx_sr_receiving = false;
    }
  }
  else if (rx_sr_full)
  {
    // Shift register fully received

    transfer_sr_to_dr();
  }
  else if (!rx_buffer.empty())
  {
    // Start receiving new byte in shift register

    rx_sr_receiving = true;
  }
}

/**
 * @brief Hook function called to simulate data transmission in the UART.
 * Transfers data from the shift register to the write buffer and updates flags
 * accordingly.
 */
void
transmitter_hook(uint32_t cyc)
{
  if (!tx_active)
  {
    return;
  }

  if (tx_sr_sending)
  {
    if (current_tx_buffer == nullptr)
    {
      tx_buffers.push_back({});
      current_tx_buffer = &tx_buffers.back();
    }

    current_tx_buffer->push_back(tx_sr);
    tx_sr_full = false;
    tx_sr_sending = false;
    // Schedule TC after a short delay to simulate transmission time
    tc_cyc = cyc + 20;
  }
  else if (tx_sr_full)
  {
    tx_sr_sending = true;
  }
  else if ((usart->SR & USART_SR_TXE) == 0)
  {
    tx_sr = usart->DR;
    tx_sr_full = true;
    SET_BIT_V(usart->SR, USART_SR_TXE);
  }
  else
  {
    if (tc_cyc.has_value() && cyc >= *tc_cyc)
    {
      current_tx_buffer = nullptr;
      tx_active = false;
      tc_cyc = std::nullopt;
      SET_BIT_V(usart->SR, USART_SR_TC);
    }
  }
}

void
reset()
{
  usart = &usart1_instance;
  rx_sr = 0;
  rx_sr_full = false;
  rx_sr_receiving = false;
  tx_sr = 0;
  tx_sr_full = false;
  tx_sr_sending = false;
  tx_active = false;
  wait_clear = false;
  tc_cyc = std::nullopt;
  rx_buffer.clear();
  tx_buffers.clear();
  current_tx_buffer = nullptr;
  rx_buffer_pos = 0;

  usart->DR = 0;
  // Start in idle state: TX empty and complete.
  usart->SR = USART_SR_TXE | USART_SR_TC;

  Base::add_test_hook("uart_read_dr", read_dr_hook);
  Base::add_test_hook("uart_write_dr", write_dr_hook);
  Base::add_test_hook("uart_read_sr", read_sr_hook);
  Base::add_hook(receiver_hook);
  Base::add_hook(transmitter_hook);
}

} // namespace Embys::Stm32::Sim::Uart
