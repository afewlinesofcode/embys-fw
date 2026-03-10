#include "runtime.hpp"

#include "../base.hpp"
#include "../uart.hpp"

namespace Embys::Stm32::Sim::Uart
{

Runtime runtime;

/**
 * @brief Helper function to transfer data from the shift register to the DR
 * register and update flags accordingly.
 */
void
transfer_sr_to_dr()
{
  if (!runtime.sr_full)
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
    usart->DR = runtime.sr;
    SET_BIT_V(usart->SR, USART_SR_RXNE);
    runtime.sr_full = false;
    runtime.sr_receiving = true;
    runtime.read_buffer_pos++;
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

  if (runtime.wait_clear)
  {
    runtime.wait_clear = false;
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
  runtime.tx_active = true;
  runtime.tc_cyc = 0;
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
    runtime.wait_clear = true;
}

/**
 * @brief Hook function called to simulate data reception in the UART.
 * Transfers data from the read buffer to the shift register and updates flags
 * accordingly.
 */
void
receiver_hook(uint32_t)
{
  if (runtime.tx_active)
  {
    return;
  }

  if (runtime.sr_receiving)
  {
    if (runtime.read_buffer.empty())
    {
      runtime.sr_receiving = false;
    }
    else if (runtime.read_buffer_pos >= runtime.read_buffer.size())
    {
      // No more bytes in read buffer, reception complete

      runtime.sr_receiving = false;
      runtime.read_buffer.clear();
      runtime.read_buffer_pos = 0;
    }
    else
    {
      // Byte received in shift register, move it to SR and mark SR as full

      runtime.sr = runtime.read_buffer[runtime.read_buffer_pos];
      runtime.sr_full = true;
      runtime.sr_receiving = false;
    }
  }
  else if (runtime.sr_full)
  {
    // Shift register fully received

    transfer_sr_to_dr();
  }
  else if (!runtime.read_buffer.empty())
  {
    // Start receiving new byte in shift register

    runtime.sr_receiving = true;
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
  if (!runtime.tx_active)
  {
    return;
  }

  if (runtime.sr_sending)
  {
    if (runtime.current_write_buffer == nullptr)
    {
      runtime.write_buffers.push_back({});
      runtime.current_write_buffer = &runtime.write_buffers.back();
    }

    runtime.current_write_buffer->push_back(runtime.sr);
    runtime.sr_full = false;
    runtime.sr_sending = false;
    // Set TC to trigger after 10 cycles to simulate transmission time
    runtime.tc_cyc = cyc + 20;
  }
  else if (runtime.sr_full)
  {
    runtime.sr_sending = true;
  }
  else if ((usart->SR & USART_SR_TXE) == 0)
  {
    runtime.sr = usart->DR;
    runtime.sr_full = true;
    SET_BIT_V(usart->SR, USART_SR_TXE);
  }
  else
  {
    if (runtime.tc_cyc && cyc >= runtime.tc_cyc)
    {
      runtime.current_write_buffer = nullptr;
      runtime.tx_active = false;
      runtime.tc_cyc = 0;
      SET_BIT_V(usart->SR, USART_SR_TC);
    }
  }
}

void
Runtime::reset()
{
  sr = 0;
  sr_receiving = false;
  sr_sending = false;
  sr_full = false;
  tx_active = false;
  wait_clear = false;
  tc_cyc = 0;
  read_buffer.clear();
  write_buffers.clear();
  current_write_buffer = nullptr;
  read_buffer_pos = 0;

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
