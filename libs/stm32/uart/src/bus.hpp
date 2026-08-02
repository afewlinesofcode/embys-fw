/**
 * @file bus.hpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief UART Bus managing UART peripheral communication
 *
 * The Bus class provides the main interface for UART communication,
 * integrating TX/RX interrupt handling with the main Loop.
 *
 * @version 0.1
 * @date 2026-04-29
 * @copyright Copyright (c) 2026
 *
 */
#pragma once

#include <array>
#include <span>
#include <string_view>
#include <type_traits>

#include <stddef.h>
#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/mcu.hpp>
#include <embys/stm32/types.hpp>

#include "def.hpp"
#include "hal.hpp"
#include "instance.hpp"
#include "stm32xx.hpp"

namespace Embys::Stm32::Uart
{

/**
 * @class Bus
 * @brief Interrupt-driven UART transceiver for STM32F1.
 *
 * Integrates with Base::Loop via a Module (for deferred RX/TX callbacks) and
 * a realtime Event (for TX timeout detection in timer-IRQ context).
 *
 * GPIO configuration for TX and RX pins must be done by the caller before
 * calling enable(). The Bus only manages the USART peripheral.
 *
 * The caller is responsible for:
 * - Configuring TX/RX GPIO pins (alternate-function push-pull / input floating)
 * - Wiring USARTn_IRQHandler → bus.handle_irq()
 * - Provisioning one extra event slot in the Loop for the internal timeout
 * event
 *
 * Example:
 * ```
 * Uart::Bus<Uart::Instance::Usart1, 64, 64> uart(loop);
 * uart.set_rx_callback({on_rx_byte, &ctx});
 * uart.set_tx_callback({on_tx_done, &ctx});
 * uart.enable(115200);
 *
 * void USART1_IRQHandler() { uart.handle_irq(); }
 * ```
 */
class BusCore
{
public:
  BusCore() = delete;
  BusCore(const BusCore &) = delete;
  BusCore(BusCore &&) = delete;
  BusCore &
  operator=(const BusCore &) = delete;
  BusCore &
  operator=(BusCore &&) = delete;

  /**
   * @brief Construct a UART Bus.
   * @param usart Peripheral selected by the owning Bus template.
   * @param base Main loop used for module and event registration.
   * @param rx_buffer Storage owned by the derived Bus template.
   * @param rx_capacity Size of rx_buffer in bytes.
   */
  ~BusCore();

  inline bool
  is_enabled() const
  {
    return enabled;
  }

  inline bool
  is_tx_busy() const
  {
    return tx_active;
  }

  inline uint32_t
  get_baud_rate() const
  {
    return baud_rate;
  }

  inline uint32_t
  get_frame_bits() const
  {
    return calc_frame_bits(word_length, stop_bits);
  }

  inline Stm32::Base::LoopCore *
  get_base() const
  {
    return base;
  }

  /**
   * @brief Set an optional RS-485 RE/DE control pin.
   * When set, the pin is driven high before transmission begins and low
   * after the TC (transmission-complete) interrupt fires.
   */
  inline void
  set_rede_pin(Gpio::PinCore *pin)
  {
    rede_pin = pin;
  }

  /**
   * @brief Enable the USART peripheral and register with the loop.
   * @return 0 on success, negative error code on failure.
   */
  int
  enable(uint32_t baud_rate, WordLength word_length = WordLength::W8,
         StopBits stop_bits = StopBits::One, Parity parity = Parity::None);

  /**
   * @brief Disable the USART peripheral and unregister from the loop.
   * @return 0 on success, negative error code on failure.
   */
  int
  disable();

  /**
   * @brief Start an asynchronous transmit operation.
   * Completion (or timeout) is signalled via the TX callback.
   * @param data Data copied into owned transmit storage before returning.
   * @return 0 on success, TX_BUSY if a transmit is already in progress.
   */
  int
  write(std::span<const uint8_t> data);

  int
  write(std::string_view text)
  {
    return write({reinterpret_cast<const uint8_t *>(text.data()), text.size()});
  }

  /**
   * @brief USART IRQ handler — must be called from the application's
   * USARTn_IRQHandler.
   */
  void
  handle_irq();

  /**
   * @brief Register a callback to be invoked for each received byte.
   * Called in main-loop context.
   */
  inline void
  set_rx_callback(Callback<uint8_t> cb)
  {
    rx_cb = cb;
  }

  inline void
  clear_rx_callback()
  {
    rx_cb.clear();
  }

  /**
   * @brief Register a callback invoked when a transmit completes or times out.
   * Argument is 0 on success, negative error code on failure.
   * Called in main-loop context.
   */
  inline void
  set_tx_callback(Callback<int> cb)
  {
    tx_cb = cb;
  }

  inline void
  clear_tx_callback()
  {
    tx_cb.clear();
  }

protected:
  BusCore(USART_TypeDef *usart, Base::LoopCore &base, uint8_t *rx_buffer,
          size_t rx_capacity, uint8_t *tx_buffer, size_t tx_capacity);

private:
  USART_TypeDef *usart;
  Base::LoopCore *base;

  // ── RX state ─────────────────────────────────────────────────────────
  uint8_t *rx_buffer;
  size_t rx_capacity;
  volatile size_t rx_buffer_len = 0;
  size_t rx_buffer_pos = 0;
  volatile bool rx_overflow = false;
  Callback<uint8_t> rx_cb;

  // ── TX state ─────────────────────────────────────────────────────────
  const uint8_t *tx_buffer = nullptr;
  uint8_t *tx_storage;
  size_t tx_capacity;
  size_t tx_buffer_len = 0;
  volatile size_t tx_buffer_pos = 0;
  volatile bool tx_active = false;
  volatile bool tx_ready = false;
  volatile int tx_result = 0;
  Callback<int> tx_cb;

  WordLength word_length = WordLength::W8;
  StopBits stop_bits = StopBits::One;
  uint32_t baud_rate = 0;

  Gpio::PinCore *rede_pin = nullptr;

  bool enabled = false;

  Base::Module *module = nullptr;

  /**
   * @brief Realtime event used to detect TX timeout inside timer-IRQ context.
   * Caller must provision one extra event slot in the Loop for this event.
   */
  Base::Event timeout_event;

  /**
   * @brief Calculate TX timeout in microseconds for len bytes.
   */
  uint32_t
  calc_tx_timeout_us(size_t len) const;

  /**
   * @brief Finalize a transmit operation with the given result code.
   * Called from IRQ context; must be short and non-blocking.
   */
  void
  tx_complete(int result);

  inline void
  set_module_pending()
  {
    base->set_module_pending(module);
  }

  static void
  module_callback(void *context);

  static void
  timeout_handler(void *context);
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
  static_assert(RxCapacity > 0, "A UART bus needs RX storage");
  static_assert(TxCapacity > 0, "A UART bus needs TX storage");
  using Storage = Detail::BusStorage<RxCapacity, TxCapacity>;

public:
  explicit Bus(Base::LoopCore &base)
    : Storage(),
      BusCore(peripheral_address<Peripheral>(), base, Storage::rx.data(),
              RxCapacity, Storage::tx.data(), TxCapacity)
  {
    static_assert(instance_available<Stm32::TargetDevice, Peripheral>,
                  "Selected UART instance is not available on this device");
  }
};

}; // namespace Embys::Stm32::Uart
