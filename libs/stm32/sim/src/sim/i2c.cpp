/**
 * @file i2c.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C simulation for the STM32 simulated environment.
 * @version 0.1
 * @date 2026-03-09
 * @copyright Copyright (c) 2026
 */

#include "i2c.hpp"

#include "base.hpp"

namespace Embys::Stm32::Sim::I2C
{

I2C_TypeDef *i2c = &i2c1_instance; // Default to i2c1

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
};

static Condition condition = Idle;

/**
 * @brief Condition that the I2C peripheral is expecting after START.
 */
static Condition expecting_condition = Idle;

/**
 * @brief Flag indicating whether the I2C peripheral is waiting for a clear
 * condition (SR1 has been read).
 */
static bool wait_clear = false;

/**
 * @brief Address of the current I2C peripheral.
 */
static std::optional<uint8_t> addr;

/**
 * @brief Shift register for simulating data reception and transmission.
 */
static uint8_t sr = 0;

/**
 * @brief Flag indicating shift register is full.
 */
static bool sr_full = false;

/**
 * @brief Flag indicating shift register is currently receiving data.
 */
static bool sr_receiving = false;

/**
 * @brief Flag indicating shift register is currently sending data.
 */
static bool sr_sending = false;

/**
 * @brief Countdown for simulating NACK when ACK is disabled.
 */
static uint8_t nack_countdown = 0;

/**
 * @brief Buffers for storing data to be read by the I2C peripheral.
 */
static std::vector<std::vector<uint8_t>> read_buffers;

/**
 * @brief Position within the current read buffer.
 */
static size_t read_buffer_pos = 0;

/**
 * @brief Buffers for storing data written by the I2C peripheral.
 */
static std::vector<std::vector<uint8_t>> write_buffers;

/**
 * @brief Whether to block SB flag from being set to simulate a stuck bus or
 * unresponsive slave.
 */
static bool block_sb = false;

/**
 * @brief Whether to block ADDR flag from being set to simulate a slave that
 * does not acknowledge its address.
 */
static bool block_addr = false;

void
simulate_recv(std::vector<uint8_t> data)
{
  read_buffers.push_back(std::move(data));
  if (read_buffers.size() == 1)
  {
    // If this is the first buffer, we need to load it into the shift register
    // to trigger the reception process.
    sr = read_buffers.front()[0];
    sr_full = true;
    sr_receiving = true;
    read_buffer_pos = 1; // Start from the second byte for the next read
  }
}

void
simulate_busy()
{
  i2c->SR2 = I2C_SR2_BUSY; // Bus busy
}

/**
 * @brief Hook for reading SR1 register, used to detect start of SB or ADDR
 * clear conditions.
 */
void
read_sr1_hook(uint32_t)
{
  wait_clear = true;
}

/**
 * @brief Helper for transferring shift register to DR register and maintaining
 * flags.
 */
void
transfer_sr_to_dr()
{
  if (!sr_full)
  {
    return;
  }

  if ((i2c->SR1 & I2C_SR1_RXNE) != 0)
  {
    SET_BIT_V(i2c->SR1, I2C_SR1_BTF);
    return;
  }

  i2c->DR = sr;
  SET_BIT_V(i2c->SR1, I2C_SR1_RXNE);
  sr_full = false;
  sr_receiving = true;
  read_buffer_pos++;
}

/**
 * @brief Hook for reading DR register.
 */
void
read_dr_hook(uint32_t)
{
  if (condition != Reading || (i2c->SR1 & I2C_SR1_RXNE) == 0)
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
  if (condition != Writing && condition != Starting)
  {
    return;
  }

  CLEAR_BIT_V(i2c->SR1, I2C_SR1_TXE);
  CLEAR_BIT_V(i2c->SR1, I2C_SR1_BTF);

  if (wait_clear)
  {
    wait_clear = false;

    if ((i2c->SR1 & I2C_SR1_SB) != 0)
    {
      CLEAR_BIT_V(i2c->SR1, I2C_SR1_SB);

      Base::add_delayed_hook(5,
                             [](uint32_t)
                             {
                               expecting_condition =
                                   (i2c->DR & 0x1) ? Reading : Writing;
                               addr = i2c->DR >> 1;

                               // Ack the address if not blocked
                               if (!block_addr)
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

  if (condition != Idle && condition != Reading && condition != Writing)
  {
    return;
  }

  bool is_repeated_start = (condition == Reading || condition == Writing);

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
  condition = Starting;

  // Simulate hardware delay for START condition to be recognized and SB flag to
  // be set
  Base::add_delayed_hook(5,
                         [](uint32_t)
                         {
                           CLEAR_BIT_V(i2c->CR1, I2C_CR1_START);

                           if (!block_sb)
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
  if ((i2c->SR1 & I2C_SR1_ADDR) == 0 || condition != Starting || !wait_clear)
  {
    return;
  }

  wait_clear = false;
  CLEAR_BIT_V(i2c->SR1, I2C_SR1_ADDR);

  condition = expecting_condition;

  if (condition == Reading)
  {
    // Immediately start receiving first byte into shift register, and set up
    // state for it
    sr_receiving = true;
    read_buffer_pos = 0;
  }
  else if (condition == Writing)
  {
    // Create new buffer to store transmitted data and set up state for it
    write_buffers.emplace_back();
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
  if (!(i2c->CR1 & I2C_CR1_ACK) && nack_countdown == 0xFF)
  {
    nack_countdown = 1;

    if (i2c->CR1 & I2C_CR1_POS)
      nack_countdown++;
  }
}

/**
 * @brief Hook for handling data reception in the I2C peripheral.
 *
 */
void
receiver_hook(uint32_t)
{
  if (condition != Reading)
  {
    return;
  }

  init_nack_countdown();

  if (sr_receiving)
  {
    if (nack_countdown > 0)
    {
      // Wasn't NACKed yet, continue receiving next byte

      if (nack_countdown != 0xFF)
        nack_countdown--;

      if (!read_buffers.empty() &&
          read_buffer_pos < read_buffers.front().size())
      {
        sr = read_buffers.front()[read_buffer_pos];
      }
      else
      {
        std::cout << "receiver_hook: No more data in buffers" << std::endl;
      }

      sr_receiving = false;
      sr_full = true;
    }
    else
    {
      sr_receiving = false;
    }
  }
  else if (sr_full)
  {
    transfer_sr_to_dr();
  }
  else if (!(i2c->SR1 & I2C_SR1_RXNE))
  {
    if (i2c->CR1 & I2C_CR1_STOP)
    {
      // Finished receiving all data
      condition = Idle;
      nack_countdown = 0xFF;
      read_buffers.erase(read_buffers.begin());

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
  if (condition != Writing)
  {
    return;
  }

  if (sr_sending)
  {
    auto &write_buffer = write_buffers.back();
    write_buffer.push_back(static_cast<uint8_t>(sr));
    sr_sending = false;
    sr_full = false;

    // Both DR and SR are now empty, can set BTF now
    if (i2c->SR1 & I2C_SR1_TXE)
    {
      SET_BIT_V(i2c->SR1, I2C_SR1_BTF);
    }
  }
  else if (sr_full)
  {
    sr_sending = true;
  }
  else if (!(i2c->SR1 & I2C_SR1_TXE))
  {
    sr = i2c->DR;
    sr_full = true;

    SET_BIT_V(i2c->SR1, I2C_SR1_TXE);
  }
  else
  {
    if (i2c->CR1 & I2C_CR1_STOP)
    {
      // Finished sending all data
      condition = Idle;

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
reset()
{
  i2c = &i2c1_instance;
  wait_clear = false;
  addr.reset();
  condition = Idle;
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
