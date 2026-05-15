#include "sm.hpp"

#ifdef I2C_HAL_V2

#include <embys/stm32/def.hpp>

#include "../../bus.hpp"
#include "../../hal.hpp"

// I2C V2 state machine (F7 / H7).
// Hardware automatically handles addressing via CR2 atomic write (SADD +
// NBYTES + RD_WRN + START/AUTOEND). Events driven by EV IRQ:
//   TXIS → write one byte to TXDR
//   RXNE → read one byte from RXDR
//   TC   → pivot from reg-write phase to read phase (reg+read case)
//   STOPF → transfer complete (AUTOEND generated STOP)
//   NACKF → slave did not ACK (error path; hardware auto-generates STOP)
// ER IRQ: BERR, ARLO, OVR.
//
// buf_len is assumed to fit in 8 bits (≤255 bytes per transfer).
// Transfers ≥256 bytes require RELOAD mode which is not implemented.

namespace Embys::Stm32::I2c
{

Sm::Sm(Bus *bus_)
  : bus(bus_), i2c(bus_->get_i2c()),
    wait_bus(bus_, {Sm::wait_bus_callback, this}),
    timeout_event(bus_->get_base(), Base::EV_RT, {Sm::timeout_handler, this})
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
  if (state != State::Start)
    return;

  // NACKF: hardware auto-generates STOP after NACK; clear flag, signal error.
  // STOPF will fire afterwards but state will already be Error so done() is
  // not called a second time.
  if (is_nackf(i2c))
  {
    clear_nackf(i2c);
    error(NACK);
    return;
  }

  if (is_txis(i2c))
    handle_txis();

  if (is_rxne(i2c))
    handle_rxne();

  if (is_tc(i2c))
    handle_tc();

  if (is_stopf(i2c))
    handle_stopf();
}

void
Sm::handle_error()
{
  if (state == State::Idle)
    return;

  uint32_t isr = i2c->ISR;

  int code = BUS_ERROR;
  if (isr & I2C_ISR_ARLO)
    code = ARBITRATION_LOST;
  else if (isr & I2C_ISR_OVR)
    code = OVERRUN;
  else if (isr & I2C_ISR_BERR)
    code = BUS_ERROR;

  clear_error_flags(i2c);
  error(code);
}

void
Sm::reset()
{
  (void)timeout_event.disable();
  state = State::Idle;
}

void
Sm::handle_txis()
{
  if (reg_and_restart)
  {
    // Reg-write phase: send the register address byte.
    write_txdr(i2c, reg);
  }
  else if (buf_pos < buf_len)
  {
    write_txdr(i2c, static_cast<uint8_t>(tx_buf[buf_pos]));
    buf_pos = buf_pos + 1u;
  }
}

void
Sm::handle_rxne()
{
  if (buf_pos < buf_len)
  {
    rx_buf[buf_pos] = read_rxdr(i2c);
    buf_pos = buf_pos + 1u;
  }
}

void
Sm::handle_tc()
{
  // TC fires after the reg byte is sent with AUTOEND=0 (no STOP generated).
  // Issue a repeated START for the read phase.
  reg_and_restart = false;
  set_cr2_transfer(i2c, addr7, static_cast<uint8_t>(buf_len), true, true, true);
}

void
Sm::handle_stopf()
{
  clear_stopf(i2c);
  if (state == State::Start)
    done();
}

void
Sm::start()
{
  uint16_t timeout_len = buf_len + (reg_and_restart ? 1u : 0u);
  (void)timeout_event.enable(25000u +
                             static_cast<uint32_t>(timeout_len) * 250u);
  state = State::Start;

  if (reg_and_restart)
  {
    // Phase 1: write register byte (1 byte, no AUTOEND so TC fires after).
    set_cr2_transfer(i2c, addr7, 1u, false, false, true);
  }
  else if (dir == Direction::Write)
  {
    set_cr2_transfer(i2c, addr7, static_cast<uint8_t>(buf_len), false, true,
                     true);
  }
  else
  {
    // Pure read.
    set_cr2_transfer(i2c, addr7, static_cast<uint8_t>(buf_len), true, true,
                     true);
  }
}

void
Sm::stop()
{
  // With AUTOEND normal completion does not need manual STOP.
  // In error paths this forces STOP to abort an in-progress transfer.
  SET_BIT_V(i2c->CR2, I2C_CR2_STOP);
  __DSB();
}

void
Sm::done()
{
  __DSB();
  result = 0;
  state = State::Stop;
}

void
Sm::error(int result_code)
{
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

#endif // I2C_HAL_V2
