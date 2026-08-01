/**
 * @file bus.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief I2C Bus managing I2C peripheral communication
 *
 * The Bus class provides the main interface for I2C communication,
 * integrating the interrupt-driven state machine with the main Loop.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <span>
#include <stdint.h>
#include <type_traits>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/mcu.hpp>
#include <embys/stm32/types.hpp>

#include "def.hpp"
#include "instance.hpp"
#include "sm.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::I2c
{

/**
 * @class Bus
 * @brief Interrupt-driven I2C master for STM32F1.
 *
 * Integrates with Base::Loop via a Module (deferred completion callbacks) and
 * an EV_RT Event (transaction timeout).
 *
 * GPIO configuration for SCL and SDA pins must be done by the caller before
 * calling enable() (open-drain AF output, appropriate speed).
 *
 * The caller is responsible for:
 * - Configuring SCL/SDA GPIO pins
 * - Enabling NVIC for I2Cx_EV_IRQn and I2Cx_ER_IRQn
 * - Wiring I2Cx_EV_IRQHandler → bus.handle_ev_irq()
 * - Wiring I2Cx_ER_IRQHandler → bus.handle_er_irq()
 * - Provisioning one extra event slot in the Loop for the timeout event
 *
 * Example:
 * ```
 * I2c::Bus<I2c::Instance::I2c1, 16, 16> bus(loop);
 * bus.enable(400000);
 *
 * void I2C1_EV_IRQHandler() { bus.handle_ev_irq(); }
 * void I2C1_ER_IRQHandler() { bus.handle_er_irq(); }
 * ```
 */
class BusCore
{
public:
  using ReadCallback = Callback<int, std::span<const uint8_t>>;

  BusCore() = delete;
  BusCore(const BusCore &) = delete;
  BusCore(BusCore &&) = delete;
  BusCore &
  operator=(const BusCore &) = delete;
  BusCore &
  operator=(BusCore &&) = delete;

  /**
   * @brief Construct an I2C Bus.
   * @param i2c Peripheral selected by the owning Bus template.
   * @param base Main loop used for module and event registration.
   */
  ~BusCore();

  inline I2C_TypeDef *
  get_i2c() const
  {
    return i2c;
  }

  inline Base::LoopCore *
  get_base() const
  {
    return base;
  }

  inline uint32_t
  get_scl_hz() const
  {
    return scl_hz;
  }

  inline bool
  is_enabled() const
  {
    return enabled;
  }

  /**
   * @brief Enable the I2C peripheral and register with the loop.
   * If the bus is found busy on entry, a recovery attempt is made.
   * @param scl_hz SCL frequency in Hz (default 100 kHz).
   * @return 0 on success, negative error code on failure.
   */
  int
  enable(uint32_t scl_hz = 100000u);

  /**
   * @brief Disable the I2C peripheral and unregister from the loop.
   * @return 0 on success, negative error code on failure.
   */
  int
  disable();

  /**
   * @brief Asynchronous read of len bytes from I2C address addr7.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  read(uint8_t addr7, uint16_t len, ReadCallback cb);

  /**
   * @brief Asynchronous register-addressed read.
   * Writes reg in the first frame, issues a repeated START, then reads.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  read(uint8_t addr7, uint8_t reg, uint16_t len, ReadCallback cb);

  /**
   * @brief Asynchronous write of len bytes to I2C address addr7.
   * @param cb  Invoked in loop context with 0 on success, negative on error.
   */
  int
  write(uint8_t addr7, const uint8_t *buf, uint16_t len, Callback<int> cb);

  /**
   * @brief I2C event IRQ handler — call from I2Cx_EV_IRQHandler.
   */
  void
  handle_ev_irq();

  /**
   * @brief I2C error IRQ handler — call from I2Cx_ER_IRQHandler.
   */
  void
  handle_er_irq();

  void
  set_module_pending()
  {
    base->set_module_pending(module);
  }

protected:
  BusCore(I2C_TypeDef *i2c, Base::LoopCore &base, uint8_t *rx_buffer,
          size_t rx_capacity, uint8_t *tx_buffer, size_t tx_capacity);

private:
  I2C_TypeDef *i2c;
  Base::LoopCore *base;
  Sm sm;
  Base::Module *module = nullptr;
  uint32_t scl_hz = 0u;
  bool enabled = false;
  Callback<int> cb;
  ReadCallback read_cb;
  uint8_t *rx_buffer;
  size_t rx_capacity;
  uint8_t *tx_buffer;
  size_t tx_capacity;
  uint16_t transfer_len = 0;
  bool reading = false;

  static void
  module_callback(void *context);
};

namespace Detail
{

template <size_t RxCapacity, size_t TxCapacity>
struct BusStorage
{
  std::array<uint8_t, RxCapacity> rx{};
  std::array<uint8_t, TxCapacity> tx{};
};

} // namespace Detail

template <Instance Peripheral, size_t RxCapacity, size_t TxCapacity>
class Bus final : private Detail::BusStorage<RxCapacity, TxCapacity>,
                  public BusCore
{
  static_assert(RxCapacity > 0, "An I2C bus needs RX storage");
  static_assert(TxCapacity > 0, "An I2C bus needs TX storage");
  using Storage = Detail::BusStorage<RxCapacity, TxCapacity>;

public:
  explicit Bus(Base::LoopCore &base)
    : Storage(),
      BusCore(peripheral_address<Peripheral>(), base, Storage::rx.data(),
              RxCapacity, Storage::tx.data(), TxCapacity)
  {
    static_assert(instance_available<Stm32::TargetDevice, Peripheral>,
                  "Selected I2C instance is not available on this device");
  }
};

}; // namespace Embys::Stm32::I2c
