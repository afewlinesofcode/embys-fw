#pragma once

#include <stddef.h>
#include <stdint.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/types.hpp>

#include "api.hpp"
#include "def.hpp"
#include "stm32f1xx.hpp"

namespace Embys::Stm32::Uart
{

/**
 * @class Bus
 * @brief Interrupt-driven UART transceiver for STM32F1.
 *
 * Integrates with Base::Loop via a Module (for deferred RX/TX callbacks) and
 * an EV_RT Event (for TX timeout detection in timer-IRQ context).
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
 * uint8_t rx_buf[64];
 * Uart::Bus uart(USART1, &loop, rx_buf, sizeof(rx_buf));
 * uart.set_rx_callback({on_rx_byte, &ctx});
 * uart.set_tx_callback({on_tx_done, &ctx});
 * uart.enable(115200);
 *
 * void USART1_IRQHandler() { uart.handle_irq(); }
 * ```
 */
class Bus
{
public:
  Bus() = delete;
  Bus(const Bus &) = delete;
  Bus(Bus &&) = delete;
  Bus &
  operator=(const Bus &) = delete;
  Bus &
  operator=(Bus &&) = delete;

  /**
   * @brief Construct a UART Bus.
   * @param usart     Peripheral instance (USART1, USART2, or USART3).
   * @param base      Main loop for module and event registration.
   * @param rx_buffer Caller-allocated buffer for incoming bytes.
   * @param rx_capacity Size of rx_buffer in bytes.
   */
  Bus(USART_TypeDef *usart, Base::Loop *base, uint8_t *rx_buffer,
      size_t rx_capacity);

  ~Bus();

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
   * @param buf  Data to transmit.
   * @param len  Number of bytes to transmit.
   * @return 0 on success, TX_BUSY if a transmit is already in progress.
   */
  int
  write(const uint8_t *buf, size_t len);

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
  set_rx_callback(Callable<uint8_t> cb)
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
  set_tx_callback(Callable<int> cb)
  {
    tx_cb = cb;
  }

  inline void
  clear_tx_callback()
  {
    tx_cb.clear();
  }

  /**
   * @brief Set an optional RS-485 RE/DE control pin.
   * When set, the pin is driven high before transmission begins and low
   * after the TC (transmission-complete) interrupt fires.
   */
  inline void
  set_rede_pin(Gpio::Pin *pin)
  {
    rede_pin = pin;
  }

private:
  USART_TypeDef *usart;
  Base::Loop *base;

  // ── RX state ─────────────────────────────────────────────────────────
  uint8_t *rx_buffer;
  size_t rx_capacity;
  volatile size_t rx_buffer_len = 0;
  size_t rx_buffer_pos = 0;
  volatile bool rx_overflow = false;
  Callable<uint8_t> rx_cb;

  // ── TX state ─────────────────────────────────────────────────────────
  const uint8_t *tx_buffer = nullptr;
  size_t tx_buffer_len = 0;
  volatile size_t tx_buffer_pos = 0;
  volatile bool tx_active = false;
  volatile bool tx_ready = false;
  volatile int tx_result = 0;
  Callable<int> tx_cb;

  WordLength word_length = WordLength::W8;
  StopBits stop_bits = StopBits::One;
  uint32_t baud_rate = 0;

  Gpio::Pin *rede_pin = nullptr;

  bool enabled = false;

  Base::Module *module = nullptr;

  /**
   * @brief EV_RT event used to detect TX timeout inside timer-IRQ context.
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

  void
  module_notify();

  static void
  module_handler(void *context);

  static void
  timeout_handler(void *context);
};

}; // namespace Embys::Stm32::Uart
