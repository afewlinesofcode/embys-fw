#include "bus.hpp"

#include <cstring>

#include <embys/stm32/base/cs.hpp>
#include <embys/stm32/def.hpp>

#include "diag.hpp"

namespace Embys::Stm32::Uart
{

BusCore::BusCore(USART_TypeDef *usart, Base::LoopCore &base,
                 uint8_t *rx_buffer, size_t rx_capacity, uint8_t *tx_buffer,
                 size_t tx_capacity)
  : usart(usart), base(&base), rx_buffer(rx_buffer), rx_capacity(rx_capacity),
    tx_storage(tx_buffer), tx_capacity(tx_capacity),
    timeout_event(base, Base::EV_RT, {BusCore::timeout_handler, this})
{
}

BusCore::~BusCore()
{
  if (enabled)
    (void)disable();
}

int
BusCore::enable(uint32_t baud_rate_, WordLength word_length_, StopBits stop_bits_,
            Parity parity_)
{
  if (enabled)
    return 0;

  TRY(enable_uart(usart, baud_rate_, word_length_, stop_bits_, parity_));

  module = base->add_module({BusCore::module_callback, this});
  if (!module)
  {
    (void)disable_uart(usart);
    return BUS_NOT_ENABLED;
  }

  baud_rate = baud_rate_;
  word_length = word_length_;
  stop_bits = stop_bits_;

  rx_buffer_len = 0;
  rx_buffer_pos = 0;
  rx_overflow = false;

  tx_buffer = nullptr;
  tx_buffer_len = 0;
  tx_buffer_pos = 0;
  tx_active = false;
  tx_ready = false;
  tx_result = 0;

  enabled = true;

  return 0;
}

int
BusCore::disable()
{
  if (!enabled)
    return 0;

  (void)timeout_event.disable();
  base->remove_module(module);
  module = nullptr;

  TRY(disable_uart(usart));

  enabled = false;

  return 0;
}

int
BusCore::write(const uint8_t *buf, size_t len)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  if (tx_active)
    return TX_BUSY;

  if (len > tx_capacity)
    return BUFFER_TOO_SMALL;

  std::memcpy(tx_storage, buf, len);
  tx_buffer = tx_storage;
  tx_buffer_len = len;
  tx_buffer_pos = 0;
  tx_active = true;
  tx_ready = false;
  tx_result = 0;

  if (rede_pin)
  {
    rede_pin->write(1);
    clear_tc(usart);
  }

  // Enable TXE interrupt — first byte sent from IRQ
  enable_txe_irq(usart);

  (void)timeout_event.enable(calc_tx_timeout_us(len));

  return 0;
}

void
BusCore::handle_irq()
{
  uint32_t sr = read_sr(usart);

  // RX
  if (sr & SR_RXNE)
  {
    uint8_t byte = read_dr(usart); // read clears RXNE

    if (rx_buffer_len < rx_capacity)
    {
      rx_buffer[rx_buffer_len] = byte;
      INC_V(rx_buffer_len);
    }
    else
    {
      rx_overflow = true;
    }

    set_module_pending();
  }

  // TX
  if ((sr & SR_TXE) && (usart->CR1 & USART_CR1_TXEIE))
  {
    if (tx_buffer_pos < tx_buffer_len)
    {
      write_dr(usart, tx_buffer[tx_buffer_pos]);
      INC_V(tx_buffer_pos);
    }
    else
    {
      disable_txe_irq(usart);

      if (rede_pin)
      {
        // Wait for physical transmission to complete before de-asserting REDE
        enable_tc_irq(usart);
      }
      else
      {
        tx_complete(0);
      }
    }
  }

  // TC (transmission complete)
  if ((sr & SR_TC) && (usart->CR1 & USART_CR1_TCIE))
  {
    disable_tc_irq(usart);
    clear_tc(usart);

    if (rede_pin)
      rede_pin->write(0);

    tx_complete(0);
  }
}

uint32_t
BusCore::calc_tx_timeout_us(size_t len) const
{
  uint32_t frame_bits = calc_frame_bits(word_length, stop_bits);
#ifndef STM32_SIM
  // +1000 us guard time to account for system latency
  return (frame_bits * static_cast<uint32_t>(len) * 1'000'000u / baud_rate) +
         1000u;
#else
  // 100 times less in simulation to speed up tests
  return (frame_bits * static_cast<uint32_t>(len) * 10'000u / baud_rate) + 10u;
#endif
}

void
BusCore::tx_complete(int result)
{
  {
    IrqGuard guard;

    tx_active = false;
    tx_ready = true;
    tx_result = result;
    disable_txe_irq(usart);
    if (rede_pin)
      disable_tc_irq(usart);
  }

  set_module_pending();
}

void
BusCore::module_callback(void *context)
{
  auto *self = static_cast<BusCore *>(context);

  // Drain received bytes
  while (self->rx_buffer_pos < self->rx_buffer_len)
  {
    uint8_t byte = self->rx_buffer[self->rx_buffer_pos];
    self->rx_buffer_pos = self->rx_buffer_pos + 1;
    self->rx_cb(byte);
  }

  if (self->rx_buffer_pos >= self->rx_buffer_len)
  {
    bool is_overflow = false;

    {
      IrqGuard guard;

      self->rx_buffer_pos = 0;
      self->rx_buffer_len = 0;
      if (self->rx_overflow)
      {
        self->rx_overflow = false;
        is_overflow = true;
      }
    }

    if (is_overflow)
      self->rx_cb(static_cast<uint8_t>(RX_OVERFLOW));
  }

  // TX completion
  if (self->tx_ready)
  {
    self->tx_ready = false;
    (void)self->timeout_event.disable();
    self->tx_cb(self->tx_result);
  }
}

void
BusCore::timeout_handler(void *context)
{
  auto *self = static_cast<BusCore *>(context);

  if (self->tx_active)
    self->tx_complete(TX_TIMEOUT);
}

}; // namespace Embys::Stm32::Uart
