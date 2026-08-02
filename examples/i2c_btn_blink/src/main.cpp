#include <stddef.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/system.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/device.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"
#include "lcd.hpp"
#include "sim.hpp"

/**
 * @brief Global pointer to timer for interrupt handler access
 */
Embys::Stm32::Base::Timer *timer_ptr = nullptr;

/**
 * @brief Global pointer to GPIO bus for interrupt handler access
 */
Embys::Stm32::Gpio::BusCore *gpio_ptr = nullptr;

/**
 * @brief Global pointer to I2C bus for interrupt handler access
 */
Embys::Stm32::I2c::BusCore *i2c_bus_ptr = nullptr;

// Interrupt handlers for each peripheral used in the example
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
  EXTI0_IRQHandler()
  {
    if (gpio_ptr)
      gpio_ptr->handle_irq(0, 0);
    else
      EXTI->PR = (1 << 0);
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
      I2C1->SR1 = 0; // Clear error flags to avoid repeated interrupts
  }
}

/**
 * @brief A startup callback to perform initial LCD setup
 *
 * @param context
 */
static void
on_start(void *context) noexcept
{
  auto *ctx = static_cast<AppContext *>(context);
  (void)ctx;
  ctx->lcd->init();
}

/**
 * @brief A callback to toggle LED state and update LCD
 *
 * @param context
 */
static void
toggle_led(void *context) noexcept
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->led_on = !ctx->led_on;
  (void)ctx->led->write(ctx->led_on ? 0 : 1);

  if (ctx->led_on)
  {
    ctx->blink_count++;
    ctx->lcd->set_blink_count(ctx->blink_count);
    SIM_LOG("LED ON  count=" << ctx->blink_count);
  }
  else
  {
    SIM_LOG("LED OFF count=" << ctx->blink_count);
  }
}

/**
 * @brief A callback to toggle blinking and update LCD
 *
 * @param context
 * @param value
 */
static void
toggle_btn(void *context, uint8_t value) noexcept
{
  auto *ctx = static_cast<AppContext *>(context);
  SIM_LOG("Button: " << (value ? "RELEASED" : "PRESSED"));

  // Act on button release (button is active-low, value=1 means released)
  if (value == 1)
  {
    ctx->blink_on = !ctx->blink_on;

    if (ctx->blink_on)
    {
      (void)ctx->blink_event->enable(
          std::chrono::microseconds{LED_BLINK_INTERVAL_US});
      SIM_LOG("Blinking ON");
    }
    else
    {
      ctx->blink_event->disable();
      ctx->led_on = false;
      (void)ctx->led->write(1);
      ctx->blink_count = 0;
      SIM_LOG("Blinking OFF");
    }

    ctx->lcd->set_blink_status(ctx->blink_on);
  }
}

int
main()
{
  using PinCfg = Embys::Stm32::Gpio::PinCfg;

  static AppContext context;

  SIM_RESET(&context);

  Embys::Stm32::Base::Timer timer(TIM2);

  // Events:
  //   - loop stop (internal to Loop)
  //   - blink event
  //   - I2C bus timeout event
  //   - I2C device delay event
  constexpr size_t events_capacity = 5;
  // Modules:
  //   - GPIO bus
  //   - I2C bus
  constexpr size_t modules_capacity = 2;
  Embys::Stm32::Base::Loop<events_capacity, modules_capacity> loop(timer);

  Embys::Stm32::Base::Event blink_event(
      loop, Embys::Stm32::Base::EventMode::Persistent, {toggle_led, &context});

  // One-shot startup event: fires on the first loop iteration (us=0)
  Embys::Stm32::Base::Event startup_event(
      loop, Embys::Stm32::Base::EventMode::Deferred, {on_start, &context});

  constexpr size_t gpio_pins_capacity = 4;
  Embys::Stm32::Gpio::Bus<gpio_pins_capacity> gpio_bus(loop);

  // Button on PA0 (input floating, EXTI)
  Embys::Stm32::Gpio::Pin<Embys::Stm32::Gpio::Port::A, 0,
                          PinCfg::IN | PinCfg::LISTEN>
      button_pin(gpio_bus);
  button_pin.set_callback({toggle_btn, &context});

  // LED on PC13 (output push-pull, 2 MHz)
  Embys::Stm32::Gpio::Pin<Embys::Stm32::Gpio::Port::C, 13,
                          PinCfg::OUT | PinCfg::MEDIUM>
      led_pin(gpio_bus);
  led_pin.set_init_value(1); // Set initial value to turn off LED (active low)

  // I2C1 SCL on PB6 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin<Embys::Stm32::Gpio::Port::B, 6,
                          PinCfg::I2C | PinCfg::HIGH>
      i2c_scl(gpio_bus);

  // I2C1 SDA on PB7 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin<Embys::Stm32::Gpio::Port::B, 7,
                          PinCfg::I2C | PinCfg::HIGH>
      i2c_sda(gpio_bus);

  Embys::Stm32::I2c::Bus<Embys::Stm32::I2c::Instance::I2c1, 16, 16> i2c_bus(
      loop);

  Embys::Stm32::I2c::Dev::I2cBtnBlink::Lcd lcd(loop, i2c_bus);

  // Set global pointers for interrupt handlers
  timer_ptr = &timer;
  gpio_ptr = &gpio_bus;
  i2c_bus_ptr = &i2c_bus;

  // Set up application context for callbacks
  context.led = &led_pin;
  context.blink_event = &blink_event;
  context.lcd = &lcd;

  // Initialize system (performs DWT setup, needs to be moved to a more
  // central place in the future)
  Embys::Stm32::Base::reset<Embys::Stm32::Family>();

  // Enable peripherals
  if (!gpio_bus.enable() || !button_pin.enable() || !led_pin.enable() ||
      !i2c_scl.enable() || !i2c_sda.enable())
    return 1;
  if (!i2c_bus.enable(100000))
    return 1;

  // Enable interrupts
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0x00);
  __NVIC_EnableIRQ(EXTI0_IRQn);
  __NVIC_EnableIRQ(I2C1_EV_IRQn);
  __NVIC_SetPriority(I2C1_EV_IRQn, 0x01);
  __NVIC_EnableIRQ(I2C1_ER_IRQn);
  __NVIC_SetPriority(I2C1_ER_IRQn, 0x01);

  // Schedule startup event
  (void)startup_event.enable(std::chrono::seconds{1});

  // Run main loop
  loop.run();

  return 0;
}
