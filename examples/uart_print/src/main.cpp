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

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/device.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/uart/bus.hpp>

#include "def.hpp"
#include "sim.hpp"

namespace Gpio = Embys::Stm32::Gpio;
namespace Uart = Embys::Stm32::Uart;
namespace Base = Embys::Stm32::Base;

// ── IRQ handler globals ───────────────────────────────────────────────────

static Base::Timer *timer_ptr = nullptr;
static Uart::BusCore *uart_ptr = nullptr;

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
  auto *bus = static_cast<Uart::BusCore *>(context);

  if (tx_busy)
    return; // Previous transmission still in progress — skip this tick.

  tx_busy = true;
  bus->write(message);
  SIM_LOG(message);
}

// ── main ──────────────────────────────────────────────────────────────────

int
main()
{
  SIM_RESET();


  // events: print event + internal UART timeout event + loop stop event
  constexpr size_t events_capacity = 3;
  // modules: GPIO bus module + UART module
  constexpr size_t modules_capacity = 2;
  Base::Timer timer(TIM2);

  Base::Loop<events_capacity, modules_capacity> loop(timer);

  // PA9  = TX: alternate-function push-pull, 10 MHz
  // PA10 = RX: input floating
  Gpio::Bus<2> gpio_bus(loop);
  Gpio::Pin<Gpio::Port::A, 9, Gpio::PinCfg::UART | Gpio::PinCfg::HIGH> pin_tx(
      gpio_bus);
  Gpio::Pin<Gpio::Port::A, 10, Gpio::PinCfg::UART | Gpio::PinCfg::HIGH> pin_rx(
      gpio_bus);


  Uart::Bus<Uart::Instance::Usart1, 64, 64> uart(loop);
  uart.set_tx_callback({on_tx_done, nullptr});

  Base::Event print_event(loop, Base::EventMode::Persistent,
                          {send_message, &uart});

  // Set global pointers for IRQ handlers (not strictly necessary in this
  // example since handlers are simple, but included for demonstration and
  // future extensibility)
  timer_ptr = &timer;
  uart_ptr = &uart;

  // Enable peripherals
  gpio_bus.enable();
  pin_tx.enable();
  pin_rx.enable();
  uart.enable(UART_BAUD);
  (void)print_event.enable(std::chrono::microseconds{PRINT_INTERVAL_US});

  // Enable IRQs
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0);
  __NVIC_EnableIRQ(USART1_IRQn);
  __NVIC_SetPriority(USART1_IRQn, 1);

  // Run the main loop
  loop.run();

  return 0;
}
