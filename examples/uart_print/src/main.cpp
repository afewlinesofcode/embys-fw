/**
 * @file main.cpp
 * @brief uart_print example — sends "Hello from Blue Pill!\r\n" over USART1
 * every 2 seconds using the interrupt-driven Uart::Bus driver.
 *
 * Hardware connections (Blue Pill / STM32F103C8):
 *   PA9  → TX  (connect to RX of USB-UART adapter)
 *   PA10 → RX  (connect to TX of USB-UART adapter, not used by this example)
 *   GND  → GND (common ground)
 *
 * Open a terminal at 115200 8N1 to see the output.
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

static const char message[] = "Hello from Blue Pill!\r\n";
static bool tx_busy = false;

static void
on_tx_done(void *, int result)
{
  // Result 0 = OK; negative = TX_TIMEOUT or other error.
  // For this example we simply clear the flag regardless — the next
  // periodic timer tick will schedule the next transmission.
  (void)result;
  tx_busy = false;
}

static void
send_message(void *context)
{
  auto *bus = static_cast<Uart::Bus *>(context);

  if (tx_busy)
    return; // Previous transmission still in progress — skip this tick.

  tx_busy = true;
  bus->write(reinterpret_cast<const uint8_t *>(message), sizeof(message) - 1);
}

// ── main ──────────────────────────────────────────────────────────────────

int
main()
{
  SIM_RESET();


  // ── timer + loop ─────────────────────────────────────────────────────
  // events: print event + internal UART timeout event + loop stop event
  constexpr size_t events_capacity = 3;
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
  uint8_t rx_buf[64]; // RX buffer (unused in this example, but Bus requires it)
  Uart::Bus uart(USART1, &loop, rx_buf, sizeof(rx_buf));
  uart_ptr = &uart;
  uart.set_tx_callback({on_tx_done, nullptr});

  uart.enable(UART_BAUD);

  // ── print event ───────────────────────────────────────────────────────
  Base::Event print_event(&loop, Base::EV_PERSIST, {send_message, &uart});
  print_event.enable(PRINT_INTERVAL_US);

  // ── enable NVIC ───────────────────────────────────────────────────────
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0);
  __NVIC_EnableIRQ(USART1_IRQn);
  __NVIC_SetPriority(USART1_IRQn, 1);

  loop.run();

  return 0;
}
