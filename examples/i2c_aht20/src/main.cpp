#include <stddef.h>
#include <stm32f1xx.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/system.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/i2c-aht20/device.hpp>
#include <embys/stm32/i2c/bus.hpp>

#include "def.hpp"
#include "lcd.hpp"
#include "sim.hpp"

/**
 * @brief Global pointer to timer for interrupt handler access
 */
Embys::Stm32::Base::Timer *timer_ptr = nullptr;

/**
 * @brief Global pointer to I2C bus for interrupt handler access
 */
Embys::Stm32::I2c::BusCore *i2c_bus_ptr = nullptr;

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

/**
 * @brief Turn LED off (called by the led_off_event)
 */
static void
led_off(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->led->write(1); // active-low: write 1 = LED off
}

/**
 * @brief AHT20 query result callback
 */
static void
on_query(void *context, int rc,
         Embys::Stm32::I2c::Dev::Aht20::Device::Values *values)
{
  auto *ctx = static_cast<AppContext *>(context);

  // Blink LED: turn on, schedule a short off event
  ctx->led->write(0); // active-low: write 0 = LED on
  ctx->led_off_event->enable(std::chrono::microseconds{LED_BLINK_US});

  if (rc != 0)
  {
    SIM_LOG("AHT20 error: " << rc);
    ctx->lcd->set_unavailable();
  }
  else
  {
    SIM_LOG("AHT20: temp_centi_c=" << values->temperature.centi_celsius
                                    << " humidity_centi_percent="
                                    << values->humidity.centi_percent);
    ctx->lcd->set_values(values->temperature, values->humidity);
  }
}

/**
 * @brief Periodic query event callback — triggers an AHT20 measurement
 */
static void
do_query(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->aht20->query({on_query, context});
}

/**
 * @brief AHT20 enable callback
 */
static void
on_aht20_enabled(void *context, int rc)
{
  auto *ctx = static_cast<AppContext *>(context);

  if (rc != 0)
  {
    SIM_LOG("AHT20 enable error: " << rc);
    ctx->lcd->set_unavailable();
    return;
  }

  SIM_LOG("AHT20 ready, starting periodic query");
  ctx->query_event->enable(std::chrono::microseconds{QUERY_INTERVAL_US});
}

/**
 * @brief Startup callback — initialise LCD, then enable AHT20
 */
static void
on_start(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->lcd->init();
}

/**
 * @brief LCD ready callback — called once the title line has been written
 *
 * Defined in lcd.cpp; exposed here via a small trampoline so that the LCD
 * class can notify main when it reaches the Ready state.
 */
static void
on_lcd_ready(void *context, int rc)
{
  auto *ctx = static_cast<AppContext *>(context);

  if (rc != 0)
  {
    SIM_LOG("LCD init error: " << rc);
    return;
  }

  SIM_LOG("LCD ready, enabling AHT20");
  ctx->aht20->enable({on_aht20_enabled, context});
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
  //   - startup event
  //   - query periodic event
  //   - led_off one-shot event
  //   - I2C bus timeout event
  //   - AHT20 delay event
  constexpr size_t events_capacity = 7;
  // Modules:
  //   - GPIO bus
  //   - I2C bus
  constexpr size_t modules_capacity = 2;
  Embys::Stm32::Base::Loop<events_capacity, modules_capacity> loop(timer);

  Embys::Stm32::Base::Event startup_event(loop, 0, {on_start, &context});

  Embys::Stm32::Base::Event query_event(loop, Embys::Stm32::Base::EV_PERSIST,
                                        {do_query, &context});

  Embys::Stm32::Base::Event led_off_event(loop, 0, {led_off, &context});

  constexpr size_t gpio_pins_capacity = 3;
  Embys::Stm32::Gpio::Bus<gpio_pins_capacity> gpio_bus(loop);

  // LED on PC13 (output push-pull, 2 MHz, active-low)
  Embys::Stm32::Gpio::Pin led_pin(&gpio_bus, GPIOC, 13,
                                  PinCfg::OUT | PinCfg::MEDIUM);
  led_pin.set_init_value(1);

  // I2C1 SCL on PB6 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin i2c_scl(&gpio_bus, GPIOB, 6,
                                  PinCfg::I2C | PinCfg::HIGH);

  // I2C1 SDA on PB7 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin i2c_sda(&gpio_bus, GPIOB, 7,
                                  PinCfg::I2C | PinCfg::HIGH);

  Embys::Stm32::I2c::Bus<Embys::Stm32::I2c::Instance::I2c1, 16, 16>
      i2c_bus(loop);

  Embys::Stm32::I2c::Dev::Aht20::Device aht20(&loop, &i2c_bus);
  Embys::Stm32::I2c::Dev::I2cAht20::Lcd lcd(&loop, &i2c_bus);

  // Set global pointers for interrupt handlers
  timer_ptr = &timer;
  i2c_bus_ptr = &i2c_bus;

  // Set up application context for callbacks
  context.led = &led_pin;
  context.query_event = &query_event;
  context.led_off_event = &led_off_event;
  context.aht20 = &aht20;
  context.lcd = &lcd;

  // Register LCD ready callback so main knows when to enable AHT20
  lcd.set_ready_cb({on_lcd_ready, &context});

  Embys::Stm32::Base::reset<Embys::Stm32::Family>();

  TRY(gpio_bus.enable());
  TRY(led_pin.enable());
  TRY(i2c_scl.enable());
  TRY(i2c_sda.enable());
  TRY(i2c_bus.enable(100000));

  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0x00);
  __NVIC_EnableIRQ(I2C1_EV_IRQn);
  __NVIC_SetPriority(I2C1_EV_IRQn, 0x01);
  __NVIC_EnableIRQ(I2C1_ER_IRQn);
  __NVIC_SetPriority(I2C1_ER_IRQn, 0x01);

  startup_event.enable(std::chrono::microseconds::zero());

  loop.run();

  return 0;
}
