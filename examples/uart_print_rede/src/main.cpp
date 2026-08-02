/**
 * @file main.cpp
 * @brief uart_print example — sends "Hello from STM32!\r\n" over USART1
 * every 2 seconds using the interrupt-driven Uart::Bus driver and USB-RS485
 * adapter via MAX485.
 *
 * Hardware connections (STM32F1 / STM32F4):
 *   PA9  → TX to MAX485 DI
 *   PA10 → RX from MAX485 RO
 *   PA8  → MAX485 RE/DE
 *   GND  → GND (common ground)
 *
 * Open a terminal at 9600 8N1 to see the output.
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

#define TRY_ASYNC(ctx, op)                                                     \
  do                                                                           \
  {                                                                            \
    int res = (op);                                                            \
    if (res < 0)                                                               \
      static_cast<AppContext *>(ctx)->loop->terminate(res, ctx);               \
  } while (0)

namespace Gpio = Embys::Stm32::Gpio;
namespace Uart = Embys::Stm32::Uart;
namespace Base = Embys::Stm32::Base;

// ── IRQ handler globals ───────────────────────────────────────────────────

static Base::Timer *timer_ptr = nullptr;
static Uart::BusCore *uart_ptr = nullptr;

extern "C"
{
  void
  panic(int code);

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

static const char message[] = "Hello from STM32!\r\n";

struct AppContext
{
  bool tx_busy = false;
  Base::LoopCore *loop = nullptr;
  Uart::BusCore *uart = nullptr;
  Gpio::PinCore *led_pin = nullptr;
  Base::Event *blink_off_event = nullptr;
};

static void
blink(AppContext *ctx)
{
  ctx->led_pin->write(0); // active-low: 0 = on
  ctx->blink_off_event->enable(std::chrono::microseconds{LED_BLINK_US});
}

static void
on_blink_off(void *ctx)
{
  auto *context = static_cast<AppContext *>(ctx);
  context->led_pin->write(1); // active-low: 1 = off
}

static void
on_tx_done(void *context, int result)
{
  TRY_ASYNC(context, result);
  auto *ctx = static_cast<AppContext *>(context);
  ctx->tx_busy = false;
}

static void
send_message(void *context)
{
  // panic(2);
  auto *ctx = static_cast<AppContext *>(context);

  if (ctx->tx_busy)
    return; // Previous transmission still in progress — skip this tick.

  ctx->tx_busy = true;
  TRY_ASYNC(ctx, ctx->uart->write(message));
  blink(ctx);
  SIM_LOG(message);
}

// ── main ──────────────────────────────────────────────────────────────────

int
main()
{
  SIM_RESET();

  AppContext app_ctx;

  // events:
  // print event, internal UART timeout event, loop stop event, blink-off event
  constexpr size_t events_capacity = 5;
  // modules: GPIO bus module + UART module
  constexpr size_t modules_capacity = 2;
  Base::Timer timer(TIM2);

  Base::Loop<events_capacity, modules_capacity> loop(timer);

  Base::Event print_event(loop, Base::EV_PERSIST, {send_message, &app_ctx});
  Base::Event blink_off_event(loop, 0, {on_blink_off, &app_ctx});

  // PA9  = TX: alternate-function push-pull, 10 MHz
  // PA10 = RX: input floating
  Gpio::Bus<4> gpio_bus(loop);
  // PC13: LED (output push-pull, 2 MHz, active-low)
  Gpio::Pin<Gpio::Port::C, 13, Gpio::PinCfg::OUT | Gpio::PinCfg::MEDIUM>
      led_pin(gpio_bus);
  led_pin.set_init_value(1); // start with LED off
  // PA8: RE/DE (output push-pull, 50 MHz) — MAX485 direction control
  Gpio::Pin<Gpio::Port::A, 8, Gpio::PinCfg::OUT | Gpio::PinCfg::MEDIUM>
      uart_rede(gpio_bus);
  uart_rede.set_init_value(0); // start in receive mode
  // USART1: PA9/PA10 use AF7 via PinCfg::UART on F4/F7/H7; it is a no-op on
  // F1, where USART1 stays on the default pin mapping.
  Gpio::Pin<Gpio::Port::A, 9, Gpio::PinCfg::UART | Gpio::PinCfg::HIGH> pin_tx(
      gpio_bus);
  Gpio::Pin<Gpio::Port::A, 10, Gpio::PinCfg::UART | Gpio::PinCfg::HIGH> pin_rx(
      gpio_bus);


  Uart::Bus<Uart::Instance::Usart1, 64, 64> uart(loop);
  uart.set_rede_pin(&uart_rede);
  uart.set_tx_callback({on_tx_done, &app_ctx});

  // Set global pointers for IRQ handlers (not strictly necessary in this
  // example since handlers are simple, but included for demonstration and
  // future extensibility)
  timer_ptr = &timer;
  uart_ptr = &uart;

  app_ctx.loop = &loop;
  app_ctx.blink_off_event = &blink_off_event;
  app_ctx.uart = &uart;
  app_ctx.led_pin = &led_pin;

  // Enable peripherals
  gpio_bus.enable();
  pin_tx.enable();
  pin_rx.enable();
  uart_rede.enable();
  led_pin.enable();
  uart.enable(UART_BAUD);
  print_event.enable(std::chrono::microseconds{PRINT_INTERVAL_US});

  // Enable IRQs
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0);
  __NVIC_EnableIRQ(USART1_IRQn);
  __NVIC_SetPriority(USART1_IRQn, 1);

  // Run the main loop
  loop.run();

  return loop.get_exit_code();
}
