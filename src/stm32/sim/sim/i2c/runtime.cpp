#include "runtime.hpp"

#include <iostream>

#include "../base.hpp"
#include "../i2c.hpp"

namespace Embys::Stm32::Sim::I2C
{

Runtime runtime;

/**
 * @brief Hook for reading SR1 register, used to detect start of SB or ADDR
 * clear conditions.
 */
void
read_sr1_hook(uint32_t)
{
  runtime.wait_clear = true;
}

/**
 * @brief Helper for transferring shift register to DR register and maintaining
 * flags.
 */
void
transfer_sr_to_dr()
{
  if (!runtime.sr_full)
  {
    return;
  }

  if ((i2c->SR1 & I2C_SR1_RXNE) != 0)
  {
    SET_BIT_V(i2c->SR1, I2C_SR1_BTF);
    return;
  }

  i2c->DR = runtime.sr;
  SET_BIT_V(i2c->SR1, I2C_SR1_RXNE);
  runtime.sr_full = false;
  runtime.sr_receiving = true;
  runtime.read_buffer_pos++;
}

/**
 * @brief Hook for reading DR register.
 */
void
read_dr_hook(uint32_t)
{
  if (runtime.condition != Runtime::Reading || (i2c->SR1 & I2C_SR1_RXNE) == 0)
  {
    return;
  }

  CLEAR_BIT_V(i2c->SR1, I2C_SR1_RXNE);
  CLEAR_BIT_V(i2c->SR1, I2C_SR1_BTF);
  transfer_sr_to_dr();
}

/**
 * @brief Hook for writing DR register.
 */
void
write_dr_hook(uint32_t)
{
  if (runtime.condition != Runtime::Writing &&
      runtime.condition != Runtime::Starting)
  {
    return;
  }

  CLEAR_BIT_V(i2c->SR1, I2C_SR1_TXE);
  CLEAR_BIT_V(i2c->SR1, I2C_SR1_BTF);

  if (runtime.wait_clear)
  {
    runtime.wait_clear = false;

    if ((i2c->SR1 & I2C_SR1_SB) != 0)
    {
      CLEAR_BIT_V(i2c->SR1, I2C_SR1_SB);

      Base::add_delayed_hook(5,
                             [](uint32_t)
                             {
                               runtime.expecting_condition =
                                   (i2c->DR & 0x1) ? Runtime::Reading
                                                   : Runtime::Writing;
                               runtime.addr = i2c->DR >> 1;

                               // Ack the address if not blocked
                               if (!runtime.block_addr)
                                 SET_BIT_V(i2c->SR1, I2C_SR1_ADDR);
                             });
    }
  }
}

/**
 * @brief Hook for detecting START condition.
 * Checks for repeated start conditions and invalid start conditions, and sets
 * BERR if invalid repeated start is detected. Simulates hardware delay for
 * START condition to be recognized and SB flag to be set.
 * Sets BUSY and MSL flags on START condition, and initializes state for address
 * transmission.
 */
void
start_hook(uint32_t)
{
  if ((i2c->CR1 & I2C_CR1_START) == 0)
  {
    return;
  }

  if (runtime.condition != Runtime::Idle &&
      runtime.condition != Runtime::Reading &&
      runtime.condition != Runtime::Writing)
  {
    return;
  }

  bool is_repeated_start = (runtime.condition == Runtime::Reading ||
                            runtime.condition == Runtime::Writing);

  if (is_repeated_start)
  {
    // Check for TXE set, and RXNE not set, and emit BERR if not valid
    if (!(i2c->SR1 & I2C_SR1_TXE) || (i2c->SR1 & I2C_SR1_RXNE))
    {
      std::cout << "start_hook: Invalid repeated START condition, setting BERR"
                << std::endl;
      // Invalid repeated start, set BERR and ignore start condition
      SET_BIT_V(i2c->SR1, I2C_SR1_BERR);
      CLEAR_BIT_V(i2c->CR1, I2C_CR1_START);
      return;
    }
  }
  else if ((i2c->SR2 & I2C_SR2_BUSY))
  {
    // Already busy, ignore START condition
    // TODO: Emit error condition
    std::cout << "start_hook: Already busy, ignoring START condition"
              << std::endl;
    return;
  }

  SET_BIT_V(i2c->SR2, I2C_SR2_BUSY | I2C_SR2_MSL);
  SET_BIT_V(i2c->SR1, I2C_SR1_TXE);
  CLEAR_BIT_V(i2c->SR1, I2C_SR1_RXNE);
  CLEAR_BIT_V(i2c->SR1, I2C_SR1_BTF);
  runtime.condition = Runtime::Starting;

  // Simulate hardware delay for START condition to be recognized and SB flag to
  // be set
  Base::add_delayed_hook(5,
                         [](uint32_t)
                         {
                           CLEAR_BIT_V(i2c->CR1, I2C_CR1_START);

                           if (!runtime.block_sb)
                             SET_BIT_V(i2c->SR1, I2C_SR1_SB);
                         });
}

/**
 * @brief Hook for reading SR2 register.
 * Used to detect when ADDR flag is cleared by hardware after address
 * transmission, and to set up state for reading/writing data after address is
 * latched.
 */
void
read_sr2_hook(uint32_t)
{
  if ((i2c->SR1 & I2C_SR1_ADDR) == 0 ||
      runtime.condition != Runtime::Starting || !runtime.wait_clear)
  {
    return;
  }

  runtime.wait_clear = false;
  CLEAR_BIT_V(i2c->SR1, I2C_SR1_ADDR);

  runtime.condition = runtime.expecting_condition;

  if (runtime.condition == Runtime::Reading)
  {
    // Immediately start receiving first byte into shift register, and set up
    // state for it
    runtime.sr_receiving = true;
    runtime.read_buffer_pos = 0;
  }
  else if (runtime.condition == Runtime::Writing)
  {
    // Create new buffer to store transmitted data and set up state for it
    runtime.write_buffers.emplace_back();
    SET_BIT_V(i2c->SR1, I2C_SR1_TXE);
  }
}

/**
 * @brief A helper function to initialize the NACK countdown when ACK is
 * disabled.
 * Simulates the behavior of the hardware where if ACK is disabled, it
 * will NACK after one byte is received (or two bytes if POS is set).
 */
void
init_nack_countdown()
{
  if (!(i2c->CR1 & I2C_CR1_ACK) && runtime.nack_countdown == 0xFF)
  {
    runtime.nack_countdown = 1;

    if (i2c->CR1 & I2C_CR1_POS)
      runtime.nack_countdown++;
  }
}

/**
 * @brief Hook for handling data reception in the I2C peripheral.
 *
 */
void
receiver_hook(uint32_t)
{
  if (runtime.condition != Runtime::Reading)
  {
    return;
  }

  init_nack_countdown();

  if (runtime.sr_receiving)
  {
    if (runtime.nack_countdown > 0)
    {
      // Wasn't NACKed yet, continue receiving next byte

      if (runtime.nack_countdown != 0xFF)
        runtime.nack_countdown--;

      if (!runtime.read_buffers.empty() &&
          runtime.read_buffer_pos < runtime.read_buffers.front().size())
      {
        runtime.sr = runtime.read_buffers.front()[runtime.read_buffer_pos];
      }
      else
      {
        std::cout << "receiver_hook: No more data in buffers" << std::endl;
      }

      runtime.sr_receiving = false;
      runtime.sr_full = true;
    }
    else
    {
      runtime.sr_receiving = false;
    }
  }
  else if (runtime.sr_full)
  {
    transfer_sr_to_dr();
  }
  else if (!(i2c->SR1 & I2C_SR1_RXNE))
  {
    if (i2c->CR1 & I2C_CR1_STOP)
    {
      // Finished receiving all data
      runtime.condition = Runtime::Idle;
      runtime.nack_countdown = 0xFF;
      runtime.read_buffers.erase(runtime.read_buffers.begin());

      Base::add_delayed_hook(5,
                             [](uint32_t)
                             {
                               CLEAR_BIT_V(i2c->CR1, I2C_CR1_STOP);
                               CLEAR_BIT_V(i2c->SR2,
                                           I2C_SR2_BUSY | I2C_SR2_MSL);
                             });
    }
  }
}

/**
 * @brief Hook for handling data transmission in the I2C peripheral.
 *
 */
void
transmitter_hook(uint32_t)
{
  if (runtime.condition != Runtime::Writing)
  {
    return;
  }

  if (runtime.sr_sending)
  {
    auto &write_buffer = runtime.write_buffers.back();
    write_buffer.push_back(static_cast<uint8_t>(runtime.sr));
    runtime.sr_sending = false;
    runtime.sr_full = false;

    // Both DR and SR are now empty, can set BTF now
    if (i2c->SR1 & I2C_SR1_TXE)
    {
      SET_BIT_V(i2c->SR1, I2C_SR1_BTF);
    }
  }
  else if (runtime.sr_full)
  {
    runtime.sr_sending = true;
  }
  else if (!(i2c->SR1 & I2C_SR1_TXE))
  {
    runtime.sr = i2c->DR;
    runtime.sr_full = true;

    SET_BIT_V(i2c->SR1, I2C_SR1_TXE);
  }
  else
  {
    if (i2c->CR1 & I2C_CR1_STOP)
    {
      // Finished sending all data
      runtime.condition = Runtime::Idle;

      Base::add_delayed_hook(5,
                             [](uint32_t)
                             {
                               CLEAR_BIT_V(i2c->CR1, I2C_CR1_STOP);
                               CLEAR_BIT_V(i2c->SR2,
                                           I2C_SR2_BUSY | I2C_SR2_MSL);
                               CLEAR_BIT_V(i2c->SR1, I2C_SR1_BTF);
                             });
    }
  }
}

void
Runtime::reset()
{
  wait_clear = false;
  addr.reset();
  condition = Runtime::Idle;
  sr = 0;
  sr_full = false;
  sr_receiving = false;
  sr_sending = false;
  nack_countdown = 0xFF;
  read_buffers.clear();
  read_buffer_pos = 0;
  write_buffers.clear();
  block_sb = false;
  block_addr = false;

  Base::add_test_hook("i2c_read_dr", read_dr_hook);
  Base::add_test_hook("i2c_write_dr", write_dr_hook);
  Base::add_test_hook("i2c_read_sr1", read_sr1_hook);
  Base::add_test_hook("i2c_read_sr2", read_sr2_hook);
  Base::add_hook(start_hook);
  Base::add_hook(receiver_hook);
  Base::add_hook(transmitter_hook);
}

}; // namespace Embys::Stm32::Sim::I2C
