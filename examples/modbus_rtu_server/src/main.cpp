/**
 * @file main.cpp
 * @brief Modbus RTU server example.
 *
 * Slave address : 0x01
 * UART          : USART1 — PA9 (TX), PA10 (RX), PA8 (RE/DE), 9600 8N1
 * LCD           : HD44780 via PCF8574 backpack on I2C1 (PB6/PB7), addr 0x27
 * LED           : PC13 (active-low) — blinks on each received Modbus request
 *
 * Modbus tables (all starting at on-wire address 0x1000, 10 items each):
 *   Coils            FC 01 / 05 / 0F
 *   Discrete inputs  FC 02
 *   Holding regs     FC 03 / 06 / 10
 *   Input regs       FC 04
 *
 * LCD line 0 : "Modbus RTU App"  (static title)
 * LCD line 1 : "R CO 0x1000"     (last processed operation)
 */

#ifndef STM32_SIM
#ifdef STM32F1xx
#include <stm32f1xx.h>
#elif defined(STM32F4xx)
#include <stm32f4xx.h>
#elif defined(STM32F7xx)
#include <stm32f7xx.h>
#elif defined(STM32H7xx)
#include <stm32h7xx.h>
#else
#error                                                                         \
    "No STM32 family defined. Define STM32F1xx, STM32F4xx, STM32F7xx, or STM32H7xx."
#endif
#endif

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/system.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/i2c/bus.hpp>
#include <embys/stm32/modbus-rtu/server.hpp>
#include <embys/stm32/modbus/def.hpp>
#include <embys/stm32/modbus/handler.hpp>
#include <embys/stm32/modbus/store.hpp>
#include <embys/stm32/uart/bus.hpp>

#include "def.hpp"
#include "lcd.hpp"
#include "sim.hpp"

#ifndef STM32_SIM
#include "memory.hpp"
#endif

namespace Gpio = Embys::Stm32::Gpio;
namespace Base = Embys::Stm32::Base;
namespace Uart = Embys::Stm32::Uart;
namespace Modbus = Embys::Stm32::Modbus;
namespace I2c = Embys::Stm32::I2c;

static Base::Timer *timer_ptr = nullptr;
static Uart::BusCore *uart_ptr = nullptr;
static I2c::BusCore *i2c_bus_ptr = nullptr;

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

  void
  I2C1_EV_IRQHandler()
  {
    if (i2c_bus_ptr)
      i2c_bus_ptr->handle_ev_irq();
  }

  void
  I2C1_ER_IRQHandler()
  {
    if (i2c_bus_ptr)
      i2c_bus_ptr->handle_er_irq();
    else
      I2C1->SR1 = 0;
  }
}

// Blink management

static void
blink(AppContext *ctx)
{
  ctx->led->write(0); // active-low: 0 = on
  ctx->blink_off_event->enable(std::chrono::microseconds{LED_BLINK_US});
}

static void
on_blink_off(void *ctx)
{
  auto *context = static_cast<AppContext *>(ctx);
  context->led->write(1); // active-low: 1 = off
}

// Modbus

static void
on_request(void *ctx, const uint8_t *buf, uint16_t len)
{
  if (len < 4U) // need at least: device_id + FC + 2-byte address
    return;

  auto *context = static_cast<AppContext *>(ctx);
  blink(context);

  uint8_t fc = buf[1];
  uint16_t addr =
      (static_cast<uint16_t>(buf[2]) << 8) | static_cast<uint16_t>(buf[3]);

  char op = '?';
  const char *type = "??";

  switch (fc)
  {
    case Modbus::FunctionCode::ReadCoils:
      op = 'R';
      type = "CO";
      break;
    case Modbus::FunctionCode::ReadDiscreteInputs:
      op = 'R';
      type = "DI";
      break;
    case Modbus::FunctionCode::ReadHoldingRegisters:
      op = 'R';
      type = "HR";
      break;
    case Modbus::FunctionCode::ReadInputRegisters:
      op = 'R';
      type = "IR";
      break;
    case Modbus::FunctionCode::WriteSingleCoil:
    case Modbus::FunctionCode::WriteMultipleCoils:
      op = 'W';
      type = "CO";
      break;
    case Modbus::FunctionCode::WriteSingleRegister:
    case Modbus::FunctionCode::WriteMultipleRegisters:
      op = 'W';
      type = "HR";
      break;
    default:
      return;
  }

  SIM_LOG("[Modbus] " << op << " " << type << " 0x" << std::hex << addr);

  // Update LCD
  context->lcd->show_operation(op, type, addr);
}

// Startup

static void
on_start(void *ctx)
{
  auto *context = static_cast<AppContext *>(ctx);
  context->lcd->init();
}

int
main()
{
  static AppContext context;

  SIM_RESET(&context);

  Base::Timer timer(TIM2);

  // Events:
  //   1. loop stop (internal)
  //   2. UART Bus timeout
  //   3. Modbus RTU frame timeout
  //   4. I2C Bus timeout
  //   5. HD44780 device delay
  //   6. LED blink-off (one-shot)
  //   7. startup (one-shot)
  constexpr size_t events_capacity = 8;
  // Modules: GPIO bus, UART bus, I2C bus
  constexpr size_t modules_capacity = 3;
  Base::Loop<events_capacity, modules_capacity> loop(timer);

  Base::Event blink_off_event(loop, 0, {on_blink_off, &context});
  Base::Event startup_event(loop, 0, {on_start, &context});

  constexpr size_t gpio_pins_capacity = 6;
  Gpio::Bus<gpio_pins_capacity> gpio_bus(loop);

  // PC13: LED (output push-pull, 2 MHz, active-low)
  Gpio::Pin<Gpio::Port::C, 13, Gpio::PinCfg::OUT | Gpio::PinCfg::MEDIUM>
      led_pin(gpio_bus);
  led_pin.set_init_value(1); // off at start

  // PA8: RE/DE (output push-pull, 50 MHz) — MAX485 direction control
  Gpio::Pin<Gpio::Port::A, 8, Gpio::PinCfg::OUT | Gpio::PinCfg::MEDIUM>
      uart_rede(gpio_bus);
  uart_rede.set_init_value(0); // start in receive mode

  // PA9: TX (AF push-pull, 50 MHz)
  Gpio::Pin<Gpio::Port::A, 9, Gpio::PinCfg::UART | Gpio::PinCfg::HIGH> uart_tx(
      gpio_bus);

  // PA10: RX (AF7 on F4/F7/H7; no-op on F1)
  Gpio::Pin<Gpio::Port::A, 10, Gpio::PinCfg::UART | Gpio::PinCfg::HIGH> uart_rx(
      gpio_bus);

  Uart::Bus<Uart::Instance::Usart1, Modbus::kFrameSize, Modbus::kFrameSize>
      uart_bus(loop);
  uart_bus.set_rede_pin(&uart_rede);

  Modbus::Store<MODBUS_TABLE_SIZE, MODBUS_TABLE_SIZE, MODBUS_TABLE_SIZE,
                MODBUS_TABLE_SIZE>
      modbus_store;

  Modbus::Handler modbus_handler(modbus_store);
  modbus_handler.set_server_id(reinterpret_cast<const uint8_t *>("EMBYS"), 5);
  modbus_handler.set_coils_offset(MODBUS_BASE_ADDR);
  modbus_handler.set_discrete_inputs_offset(MODBUS_BASE_ADDR);
  modbus_handler.set_holding_registers_offset(MODBUS_BASE_ADDR);
  modbus_handler.set_input_registers_offset(MODBUS_BASE_ADDR);

  Modbus::Rtu::Server modbus_server(MODBUS_SLAVE_ADDR, modbus_handler,
                                    uart_bus);
  modbus_server.set_on_request_callback({on_request, &context});

  // PB6: SCL (open-drain AF, 50 MHz)
  Gpio::Pin<Gpio::Port::B, 6, Gpio::PinCfg::I2C | Gpio::PinCfg::HIGH> i2c_scl(
      gpio_bus);

  // PB7: SDA (open-drain AF, 50 MHz)
  Gpio::Pin<Gpio::Port::B, 7, Gpio::PinCfg::I2C | Gpio::PinCfg::HIGH> i2c_sda(
      gpio_bus);

  I2c::Bus<I2c::Instance::I2c1, 16, 16> i2c_bus(loop);
  ModbusRtuServer::Lcd lcd(loop, i2c_bus);

  // Global pointers for IRQ handlers
  timer_ptr = &timer;
  uart_ptr = &uart_bus;
  i2c_bus_ptr = &i2c_bus;

  // Application context
  context.led = &led_pin;
  context.blink_off_event = &blink_off_event;
  context.lcd = &lcd;

  Base::reset<Embys::Stm32::Family>();

  // Enable peripherals
  TRY(gpio_bus.enable());
  TRY(led_pin.enable());
  TRY(uart_rede.enable());
  TRY(uart_tx.enable());
  TRY(uart_rx.enable());
  TRY(i2c_scl.enable());
  TRY(i2c_sda.enable());
  TRY(uart_bus.enable(UART_BAUD));
  TRY(i2c_bus.enable(100000));

  modbus_server.enable();
#ifdef STM32_SIM
  modbus_server.override_frame_delay_us(100);
#endif

  // Enable interrupts
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0x00);
  __NVIC_EnableIRQ(USART1_IRQn);
  __NVIC_SetPriority(USART1_IRQn, 0x01);
  __NVIC_EnableIRQ(I2C1_EV_IRQn);
  __NVIC_SetPriority(I2C1_EV_IRQn, 0x01);
  __NVIC_EnableIRQ(I2C1_ER_IRQn);
  __NVIC_SetPriority(I2C1_ER_IRQn, 0x01);

  startup_event.enable(std::chrono::seconds{1});

  loop.run();

  return 0;
}
