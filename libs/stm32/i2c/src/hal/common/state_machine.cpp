#include "state_machine.hpp"

#ifdef EMBYS_I2C_CLASSIC_REGISTERS

#include <embys/stm32/def.hpp>

#include "../../bus.hpp"
#include "../../hal.hpp"

namespace Embys::Stm32::I2c
{

Sm::Sm(BusCore *bus_)
  : bus(bus_), i2c(bus_->get_i2c()),
    wait_bus(bus_, {Sm::wait_bus_callback, this}),
    timeout_event(*bus_->get_base(), Base::EV_RT,
                  {Sm::timeout_handler, this})
{
}

int
Sm::start_read(uint8_t addr7_, uint8_t *buf, uint16_t len)
{
  if (state != State::Idle)
    return INVALID_STATE;

  if (!buf || !len)
    return INVALID_BUFFER;

  addr7 = addr7_;
  dir = Direction::Read;
  reg_and_restart = false;
  rx_buf = buf;
  buf_len = len;
  buf_pos = 0;
  result = 0;
  state = State::WaitBus;

  return wait_bus.start();
}

int
Sm::start_read(uint8_t addr7_, uint8_t reg_, uint8_t *buf, uint16_t len)
{
  if (state != State::Idle)
    return INVALID_STATE;

  if (!buf || !len)
    return INVALID_BUFFER;

  addr7 = addr7_;
  dir = Direction::Read;
  reg_and_restart = true;
  reg = reg_;
  rx_buf = buf;
  buf_len = len;
  buf_pos = 0;
  result = 0;
  state = State::WaitBus;

  return wait_bus.start();
}

int
Sm::start_write(uint8_t addr7_, const uint8_t *buf, uint16_t len)
{
  if (state != State::Idle)
    return INVALID_STATE;

  if (!buf || !len)
    return INVALID_BUFFER;

  addr7 = addr7_;
  dir = Direction::Write;
  reg_and_restart = false;
  tx_buf = buf;
  buf_len = len;
  buf_pos = 0;
  result = 0;
  state = State::WaitBus;

  return wait_bus.start();
}

void
Sm::handle_irq()
{
  handle_start();
  handle_address();
  handle_write_reg();
  handle_write_data();
  handle_read_data();
}

void
Sm::handle_error()
{
  if (state == State::Idle)
    return;

  uint32_t sr1 = i2c->SR1;

  if (sr1 & I2C_SR1_AF)
    error(NACK);
  else if (sr1 & I2C_SR1_BERR)
    error(BUS_ERROR);
  else if (sr1 & I2C_SR1_ARLO)
    error(ARBITRATION_LOST);
  else if (sr1 & I2C_SR1_OVR)
    error(OVERRUN);
  else if (sr1 & I2C_SR1_TIMEOUT)
    error(TIMEOUT);
}

void
Sm::reset()
{
  (void)timeout_event.disable();
  state = State::Idle;
}

void
Sm::handle_start()
{
  if (!is_sb(i2c))
    return;

  if (state != State::Start)
  {
    // Unexpected SB — write dummy address to clear it
    write_dr(i2c, 0x00u | 1u);
    return;
  }

  state = State::Address;

  if (dir == Direction::Read && !reg_and_restart)
  {
    write_dr(i2c, static_cast<uint8_t>((addr7 << 1u) | 1u)); // read bit

    if (buf_len == 2u)
      pos_enable(i2c); // STM32 2-byte read errata workaround
  }
  else
  {
    write_dr(i2c, static_cast<uint8_t>(addr7 << 1u)); // write bit
  }
}

void
Sm::handle_address()
{
  if (!is_addr(i2c))
    return;

  if (state != State::Address)
  {
    // Unexpected ADDR — clear by reading SR2
    (void)read_sr2(i2c);
    return;
  }

  if (reg_and_restart)
  {
    (void)read_sr2(i2c); // clear ADDR
    write_dr(i2c, reg);
    enable_buf_irq(i2c);
    state = State::WriteReg;
  }
  else if (dir == Direction::Write)
  {
    (void)read_sr2(i2c); // clear ADDR
    if (buf_pos < buf_len)
    {
      write_dr(i2c, static_cast<uint8_t>(tx_buf[buf_pos]));
      buf_pos = buf_pos + 1u;
    }
    enable_buf_irq(i2c);
    state = State::WriteData;
  }
  else // Read
  {
    if (buf_len == 1u)
    {
      nack(i2c);
      (void)read_sr2(i2c); // clear ADDR
      stop();
    }
    else if (buf_len == 2u)
    {
      nack(i2c);           // POS already set; NACK the second byte
      (void)read_sr2(i2c); // clear ADDR
    }
    else
    {
      (void)read_sr2(i2c); // clear ADDR; ACK default
    }
    enable_buf_irq(i2c);
    state = State::ReadData;
  }
}

void
Sm::handle_write_reg()
{
  if (state != State::WriteReg)
    return;

  if (is_btf(i2c))
  {
    // Register byte fully sent — issue repeated START for the read phase
    reg_and_restart = false;
    disable_buf_irq(i2c);
    start_condition(i2c);
    state = State::Start;
  }
}

void
Sm::handle_write_data()
{
  if (state != State::WriteData)
    return;

  if (buf_pos < buf_len)
  {
    if (is_txe(i2c))
    {
      write_dr(i2c, static_cast<uint8_t>(tx_buf[buf_pos]));
      buf_pos = buf_pos + 1u;
    }
  }
  else if (is_btf(i2c))
  {
    // All bytes transmitted — generate STOP
    stop();
    done();
  }
}

void
Sm::handle_read_data()
{
  if (state != State::ReadData)
  {
    if (is_rxne(i2c))
      (void)read_dr(i2c); // discard unexpected byte
    return;
  }

  if (buf_len == 1u)
    handle_read_data_1();
  else if (buf_len == 2u)
    handle_read_data_2();
  else
    handle_read_data_n();
}

void
Sm::handle_read_data_1()
{
  if (is_rxne(i2c))
  {
    rx_buf[buf_pos] = read_dr(i2c);
    buf_pos = buf_pos + 1u;
    done();
  }
}

void
Sm::handle_read_data_2()
{
  // Wait for BTF: both shift register and DR hold one byte each.
  // NACK and POS were set in handle_address; STOP then read both bytes.
  if (is_rxne(i2c) && is_btf(i2c))
  {
    stop();
    rx_buf[buf_pos] = read_dr(i2c);
    buf_pos = buf_pos + 1u;
    rx_buf[buf_pos] = read_dr(i2c);
    buf_pos = buf_pos + 1u;
    done();
  }
}

void
Sm::handle_read_data_n()
{
  if (buf_pos < buf_len - 3u)
  {
    if (is_rxne(i2c))
    {
      rx_buf[buf_pos] = read_dr(i2c);
      buf_pos = buf_pos + 1u;
    }
  }
  else if (buf_pos == buf_len - 3u)
  {
    // At N-3: wait for BTF (N-2 in DR, N-3 in SR).
    // NACK so device stops after N-1, then STOP, read last two.
    if (is_btf(i2c))
    {
      nack(i2c);
      rx_buf[buf_pos] = read_dr(i2c); // byte N-3 (clears BTF)
      buf_pos = buf_pos + 1u;
      stop();
      rx_buf[buf_pos] = read_dr(i2c); // byte N-2
      buf_pos = buf_pos + 1u;
    }
  }
  else if (buf_pos == buf_len - 1u)
  {
    if (is_rxne(i2c))
    {
      rx_buf[buf_pos] = read_dr(i2c); // byte N-1 (last)
      buf_pos = buf_pos + 1u;
      done();
    }
  }
}

void
Sm::start()
{
  uint16_t timeout_len = buf_len + (reg_and_restart ? 1u : 0u);
  (void)timeout_event.enable(std::chrono::microseconds{
      25000u + static_cast<uint32_t>(timeout_len) * 250u});
  state = State::Start;
  ack(i2c);
  pos_disable(i2c);
  start_condition(i2c);
}

void
Sm::stop()
{
  stop_condition(i2c);
  __DSB();
}

void
Sm::done()
{
  __DSB();
  disable_buf_irq(i2c);
  result = 0;
  state = State::Stop;
}

void
Sm::error(int result_code)
{
  disable_buf_irq(i2c);
  CLEAR_BIT_V(i2c->SR1, err_mask);
  stop();
  reset_i2c(i2c);

  result = result_code;
  state = State::Error;
}

void
Sm::timeout_handler(void *context)
{
  auto *self = static_cast<Sm *>(context);

  if (self->state == State::Idle || self->is_complete())
    return;

  self->error(TIMEOUT);
  self->bus->set_module_pending();
}

void
Sm::wait_bus_callback(void *context, int res)
{
  auto *self = static_cast<Sm *>(context);

  if (self->state != State::WaitBus)
    return;

  if (res == 0)
  {
    self->start();
    return;
  }

  self->error(res);
  self->bus->set_module_pending();
}

}; // namespace Embys::Stm32::I2c

#endif // EMBYS_I2C_CLASSIC_REGISTERS
