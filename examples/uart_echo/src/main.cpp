/**
 * @file main.cpp
 * @brief uart_echo example — receives bytes over USART1 and echoes each
 * complete line back (terminated by '\n' or '\r').
 *
 * Hardware connections (Blue Pill / STM32F103C8):
 *   PA9  → TX  (connect to RX of USB-UART adapter)
 *   PA10 → RX  (connect to TX of USB-UART adapter)
 *   GND  → GND (common ground)
 *
 * Open a terminal at 115200 8N1. Type a line and press Enter — the device
 * echoes the whole line back followed by "\r\n".
 *
 * Implementation notes:
 *   - Received bytes are buffered in line_buf[].
 *   - When '\r' or '\n' arrives the accumulated line is sent back.
 *   - While a TX is in progress incoming bytes are still buffered; if the
 *     buffer fills before TX completes, it is flushed immediately.
 *   - The UART Bus timeout event uses one event slot, so events_capacity
 *     must be at least 2 (timeout event + loop stop event).
 */

#ifndef STM32_SIM
#include <stm32f1xx.h>
#endif

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/uart/api.hpp>
#include <embys/stm32/uart/bus.hpp>

#include "def.hpp"
#include "sim.hpp"

namespace Gpio = Embys::Stm32::Gpio;
namespace Uart = Embys::Stm32::Uart;
namespace Base = Embys::Stm32::Base;

// ── IRQ handler globals ───────────────────────────────────────────────────

static Base::Timer *timer_ptr = nullptr;
static Uart::Bus *uart_ptr = nullptr;

extern "C"
{
  void
  TIM2_IRQHandler()
  {
    if (timer_ptr)
      timer_ptr->handle_irq();
    else
      CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
  }

  void
  USART1_IRQHandler()
  {
    if (uart_ptr)
      uart_ptr->handle_irq();
  }
}

// ── application state ─────────────────────────────────────────────────────

struct AppContext
{
  Uart::Bus *uart;

  // Accumulation buffer for the current incoming line
  uint8_t line_buf[LINE_BUF_LEN];
  size_t line_len = 0;

  // Separate TX buffer — must stay valid until the TX callback fires
  uint8_t tx_buf[LINE_BUF_LEN + 2]; // +2 for appended "\r\n"
  size_t tx_len = 0;

  bool tx_busy = false;
};

// ── callbacks ─────────────────────────────────────────────────────────────

static void
flush_line(AppContext *ctx);

static void
on_rx_byte(void *context, uint8_t byte)
{
  auto *ctx = static_cast<AppContext *>(context);

  bool end_of_line = (byte == '\r' || byte == '\n');

  if (!end_of_line && ctx->line_len < LINE_BUF_LEN)
  {
    ctx->line_buf[ctx->line_len++] = byte;
    return;
  }

  // Either end-of-line arrived or the buffer is full — echo what we have.
  if (ctx->line_len > 0)
    flush_line(ctx);
}

static void
on_tx_done(void *context, int /* result */)
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->tx_busy = false;
}

// Send accumulated line followed by "\r\n".
static void
flush_line(AppContext *ctx)
{
  if (ctx->tx_busy)
    return; // TX still running; the buffer will be flushed on next character.

  // Copy line into the TX buffer and append CRLF
  for (size_t i = 0; i < ctx->line_len; ++i)
    ctx->tx_buf[i] = ctx->line_buf[i];
  ctx->tx_buf[ctx->line_len] = '\r';
  ctx->tx_buf[ctx->line_len + 1] = '\n';
  ctx->tx_len = ctx->line_len + 2;
  ctx->line_len = 0;

  ctx->tx_busy = true;
  ctx->uart->write(ctx->tx_buf, ctx->tx_len);
}

// ── main ──────────────────────────────────────────────────────────────────

int
main()
{
  SIM_RESET();


  // ── timer + loop ─────────────────────────────────────────────────────
  // events: UART timeout event + loop stop event
  constexpr size_t events_capacity = 2;
  Base::Event *event_slots[events_capacity];
  Base::Event *active_event_slots[events_capacity];

  // modules: GPIO bus module + UART module
  constexpr size_t modules_capacity = 2;
  Base::Module module_slots[modules_capacity];

  Base::Timer timer(TIM2);
  timer_ptr = &timer;

  Base::Loop loop(&timer, event_slots, active_event_slots, events_capacity,
                  module_slots, modules_capacity);

  // ── configure USART1 GPIO ────────────────────────────────────
  // PA9  = TX: alternate-function push-pull, 10 MHz
  // PA10 = RX: input floating
  Gpio::Pin *gpio_pin_slots[2];
  Gpio::Bus gpio_bus(&loop, gpio_pin_slots, 2);
  Gpio::Pin pin_tx(&gpio_bus, GPIOA, 9, Gpio::Mode::OUT_10,
                   Gpio::Cnf::OUT_PP_AF, Gpio::PinCfg::NONE);
  Gpio::Pin pin_rx(&gpio_bus, GPIOA, 10, Gpio::Mode::IN, Gpio::Cnf::IN_FL,
                   Gpio::PinCfg::NONE);

  gpio_bus.enable();
  pin_tx.enable();
  pin_rx.enable();

  // ── UART bus ──────────────────────────────────────────────────────────
  uint8_t rx_buf[64];
  Uart::Bus uart(USART1, &loop, rx_buf, sizeof(rx_buf));
  uart_ptr = &uart;

  AppContext ctx;
  ctx.uart = &uart;
  ctx.line_len = 0;
  ctx.tx_busy = false;

  uart.set_rx_callback({on_rx_byte, &ctx});
  uart.set_tx_callback({on_tx_done, &ctx});
  uart.enable(UART_BAUD);

  // ── enable NVIC ───────────────────────────────────────────────────────
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0);
  __NVIC_EnableIRQ(USART1_IRQn);
  __NVIC_SetPriority(USART1_IRQn, 1);

  loop.run();

  return 0;
}
