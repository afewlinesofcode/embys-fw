#include <stddef.h>
#include <stm32f1xx.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/system.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/i2c/api.hpp>
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
Embys::Stm32::Gpio::Bus *gpio_ptr = nullptr;

/**
 * @brief Global pointer to I2C bus for interrupt handler access
 */
Embys::Stm32::I2c::Bus *i2c_bus_ptr = nullptr;

// Interrupt handlers for each peripheral used in the example
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
  }
}

/**
 * @brief Application context to be used in callbacks
 */
struct AppContext
{
  bool led_on = false;
  bool blink_on = false;
  uint32_t blink_count = 0;

  Embys::Stm32::Gpio::Pin *led = nullptr;
  Embys::Stm32::Base::Event *blink_event = nullptr;
  Embys::Stm32::I2c::Dev::I2cBtnBlink::Lcd *lcd = nullptr;
};

/**
 * @brief A startup callback to perform initial LCD setup
 *
 * @param context
 */
static void
on_start(void *context)
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
toggle_led(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);
  ctx->led_on = !ctx->led_on;
  ctx->led->write(ctx->led_on ? 0 : 1);
  ctx->led->write(ctx->led_on ? 1 : 0);

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
toggle_btn(void *context, uint8_t value)
{
  auto *ctx = static_cast<AppContext *>(context);
  SIM_LOG("Button: " << (value ? "RELEASED" : "PRESSED"));

  // Act on button release (button is active-low, value=1 means released)
  if (value == 1)
  {
    ctx->blink_on = !ctx->blink_on;

    if (ctx->blink_on)
    {
      ctx->blink_event->enable(LED_BLINK_INTERVAL_US);
      SIM_LOG("Blinking ON");
    }
    else
    {
      ctx->blink_event->disable();
      ctx->led_on = false;
      ctx->led->write(1);
      ctx->blink_count = 0;
      SIM_LOG("Blinking OFF");
    }

    ctx->lcd->set_blink_status(ctx->blink_on);
  }
}

int
main()
{
  using GpioMode = Embys::Stm32::Gpio::Mode;
  using GpioCnf = Embys::Stm32::Gpio::Cnf;
  using PinCfg = Embys::Stm32::Gpio::PinCfg;

  static AppContext context;

  SIM_RESET();

  Embys::Stm32::Base::Timer timer(TIM2);

  // Events:
  //   - loop stop (internal to Loop)
  //   - blink event
  //   - I2C bus timeout event
  //   - I2C device delay event
  constexpr size_t events_capacity = 5;
  static Embys::Stm32::Base::Event *event_slots[events_capacity];
  static Embys::Stm32::Base::Event *active_event_slots[events_capacity];

  // Modules:
  //   - GPIO bus
  //   - I2C bus
  constexpr size_t modules_capacity = 2;
  static Embys::Stm32::Base::Module module_slots[modules_capacity];

  Embys::Stm32::Base::Loop loop(&timer, event_slots, active_event_slots,
                                events_capacity, module_slots,
                                modules_capacity);

  Embys::Stm32::Base::Event blink_event(&loop, Embys::Stm32::Base::EV_PERSIST,
                                        {toggle_led, &context});

  // One-shot startup event: fires on the first loop iteration (us=0)
  Embys::Stm32::Base::Event startup_event(&loop, 0, {on_start, &context});

  constexpr size_t gpio_pins_capacity = 4;
  static Embys::Stm32::Gpio::Pin *gpio_pin_slots[gpio_pins_capacity];

  Embys::Stm32::Gpio::Bus gpio_bus(&loop, gpio_pin_slots, gpio_pins_capacity);

  // Button on PA0 (input floating, EXTI)
  Embys::Stm32::Gpio::Pin button_pin(&gpio_bus, GPIOA, 0, GpioMode::IN,
                                     GpioCnf::IN_FL, PinCfg::IRQ);
  button_pin.set_callback({toggle_btn, &context});

  // LED on PC13 (output push-pull, 2 MHz)
  Embys::Stm32::Gpio::Pin led_pin(&gpio_bus, GPIOC, 13, GpioMode::OUT_2,
                                  GpioCnf::OUT_PP, PinCfg::NONE);
  led_pin.set_init_value(1); // Set initial value to turn off LED (active low)

  // I2C1 SCL on PB6 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin i2c_scl(&gpio_bus, GPIOB, 6, GpioMode::OUT_50,
                                  GpioCnf::OUT_OD_AF, PinCfg::NONE);

  // I2C1 SDA on PB7 (output open-drain AF, 50 MHz)
  Embys::Stm32::Gpio::Pin i2c_sda(&gpio_bus, GPIOB, 7, GpioMode::OUT_50,
                                  GpioCnf::OUT_OD_AF, PinCfg::NONE);

  Embys::Stm32::I2c::Bus i2c_bus(I2C1, &loop);

  Embys::Stm32::I2c::Dev::I2cBtnBlink::Lcd lcd(&loop, &i2c_bus);

  // Set global pointers for interrupt handlers
  timer_ptr = &timer;
  gpio_ptr = &gpio_bus;
  i2c_bus_ptr = &i2c_bus;

  // Set up application context for callbacks
  context.led = &led_pin;
  context.blink_event = &blink_event;
  context.lcd = &lcd;

  // Initialize system (performs DWT setup, probably will be moved to a more
  // central place in the future)
  Embys::Stm32::Base::system_init();

  // Enable peripherals
  TRY(gpio_bus.enable());
  TRY(button_pin.enable());
  TRY(led_pin.enable());
  TRY(i2c_scl.enable());
  TRY(i2c_sda.enable());
  TRY(i2c_bus.enable(100000));

  // Enable interrupts
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0x00);
  __NVIC_EnableIRQ(EXTI0_IRQn);
  __NVIC_EnableIRQ(I2C1_EV_IRQn);
  __NVIC_SetPriority(I2C1_EV_IRQn, 0x01);
  __NVIC_EnableIRQ(I2C1_ER_IRQn);
  __NVIC_SetPriority(I2C1_ER_IRQn, 0x01);

  // Schedule startup event
  startup_event.enable(0);

  // Run main loop
  loop.run();

  return 0;
}
