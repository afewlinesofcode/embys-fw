#include "bus.hpp"

#include <cstring>

#include <embys/stm32/def.hpp>

#include "hal.hpp"

namespace Embys::Stm32::I2c
{

BusCore::BusCore(I2C_TypeDef *i2c_, Base::LoopCore &base_, uint8_t *rx_buffer_,
                 size_t rx_capacity_, uint8_t *tx_buffer_, size_t tx_capacity_)
  : i2c(i2c_), base(&base_), sm(this), rx_buffer(rx_buffer_),
    rx_capacity(rx_capacity_), tx_buffer(tx_buffer_), tx_capacity(tx_capacity_)
{
}

BusCore::~BusCore()
{
  if (enabled)
    (void)disable();
}

int
BusCore::enable(uint32_t scl_hz_)
{
  if (enabled)
    return 0;

  TRY(enable_i2c(i2c, scl_hz_));
  TRY(reset_i2c(i2c));


  module = base->add_module({BusCore::module_callback, this});
  if (!module)
  {
    (void)disable_i2c(i2c);
    return BUS_NOT_ENABLED;
  }

  scl_hz = scl_hz_;
  enabled = true;

  return 0;
}

int
BusCore::disable()
{
  if (!enabled)
    return 0;

  sm.reset();
  base->remove_module(module);
  module = nullptr;

  TRY(disable_i2c(i2c));

  enabled = false;

  return 0;
}

int
BusCore::read(uint8_t addr7, uint16_t len, ReadCallback cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  if (len > rx_capacity)
    return BUFFER_TOO_SMALL;

  read_cb = cb;
  reading = true;
  transfer_len = len;
  TRY(sm.start_read(addr7, rx_buffer, len));

  return 0;
}

int
BusCore::read(uint8_t addr7, uint8_t reg, uint16_t len, ReadCallback cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  if (len > rx_capacity)
    return BUFFER_TOO_SMALL;

  read_cb = cb;
  reading = true;
  transfer_len = len;
  TRY(sm.start_read(addr7, reg, rx_buffer, len));

  return 0;
}

int
BusCore::write(uint8_t addr7, std::span<const uint8_t> data, Callback<int> cb)
{
  if (!enabled)
    return BUS_NOT_ENABLED;

  if (data.size() > tx_capacity)
    return BUFFER_TOO_SMALL;

  if (!data.empty())
    std::memcpy(tx_buffer, data.data(), data.size());
  this->cb = cb;
  reading = false;
  transfer_len = static_cast<uint16_t>(data.size());
  TRY(sm.start_write(addr7, tx_buffer, transfer_len));

  return 0;
}

void
BusCore::handle_ev_irq()
{
  sm.handle_irq();

  if (sm.is_complete())
    set_module_pending();
}

void
BusCore::handle_er_irq()
{
  sm.handle_error();

  if (sm.is_complete())
    set_module_pending();

  clear_error_flags(i2c); // Clear error flags to avoid repeated interrupts
}

void
BusCore::module_callback(void *context) noexcept
{
  auto *self = static_cast<BusCore *>(context);

  if (self->sm.is_complete())
  {
    int result = self->sm.get_result();
    self->sm.reset();
    if (self->reading)
      self->read_cb(result, {self->rx_buffer, self->transfer_len});
    else
      self->cb(result);
  }
}

}; // namespace Embys::Stm32::I2c
