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

Status
BusCore::enable(uint32_t scl_hz_)
{
  if (enabled)
    return Status::success();

  const Status enable_result = enable_i2c(i2c, scl_hz_);
  if (!enable_result)
    return enable_result;

  const Status reset_result = reset_i2c(i2c);
  if (!reset_result)
  {
    (void)disable_i2c(i2c);
    return reset_result;
  }


  module = base->add_module({BusCore::module_callback, this});
  if (!module)
  {
    (void)disable_i2c(i2c);
    return Status::failure(Error::ModuleCapacity);
  }

  scl_hz = scl_hz_;
  enabled = true;

  return Status::success();
}

Status
BusCore::disable()
{
  if (!enabled)
    return Status::success();

  sm.reset();
  base->remove_module(module);
  module = nullptr;

  const Status disable_result = disable_i2c(i2c);
  if (!disable_result)
    return disable_result;

  enabled = false;

  return Status::success();
}

Status
BusCore::read(uint8_t addr7, uint16_t len, ReadCallback cb)
{
  if (!enabled)
    return Status::failure(Error::NotEnabled);

  if (len > rx_capacity)
    return Status::failure(Error::BufferTooSmall);

  read_cb = cb;
  reading = true;
  transfer_len = len;
  const Status start_result = sm.start_read(addr7, rx_buffer, len);
  if (!start_result)
  {
    reading = false;
    transfer_len = 0;
    read_cb.clear();
    return start_result;
  }

  return Status::success();
}

Status
BusCore::read(uint8_t addr7, uint8_t reg, uint16_t len, ReadCallback cb)
{
  if (!enabled)
    return Status::failure(Error::NotEnabled);

  if (len > rx_capacity)
    return Status::failure(Error::BufferTooSmall);

  read_cb = cb;
  reading = true;
  transfer_len = len;
  const Status start_result = sm.start_read(addr7, reg, rx_buffer, len);
  if (!start_result)
  {
    reading = false;
    transfer_len = 0;
    read_cb.clear();
    return start_result;
  }

  return Status::success();
}

Status
BusCore::write(uint8_t addr7, std::span<const uint8_t> data, WriteCallback cb)
{
  if (!enabled)
    return Status::failure(Error::NotEnabled);

  if (data.size() > tx_capacity)
    return Status::failure(Error::BufferTooSmall);

  if (!data.empty())
    std::memcpy(tx_buffer, data.data(), data.size());
  this->cb = cb;
  reading = false;
  transfer_len = static_cast<uint16_t>(data.size());
  const Status start_result = sm.start_write(addr7, tx_buffer, transfer_len);
  if (!start_result)
  {
    transfer_len = 0;
    this->cb.clear();
    return start_result;
  }

  return Status::success();
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
    const Status result = self->sm.get_result();
    self->sm.reset();
    if (self->reading)
    {
      if (result)
        self->read_cb(ReadResult::success(
            {self->rx_buffer, static_cast<size_t>(self->transfer_len)}));
      else
        self->read_cb(ReadResult::failure(result.error()));
    }
    else
      self->cb(result);
  }
}

}; // namespace Embys::Stm32::I2c
