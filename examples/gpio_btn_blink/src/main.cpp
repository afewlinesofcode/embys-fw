#include <stm32f1xx.h>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/timer.hpp>
#include <embys/stm32/def.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>

#include "def.hpp"
#include "sim.hpp"

/**
 * @brief Global pointer to timer for interrupt handler access
 */
Embys::Stm32::Base::Timer *timer_ptr = nullptr;

/**
 * @brief Global pointer to GPIO bus for interrupt handler access
 */
Embys::Stm32::Gpio::Bus *gpio_ptr = nullptr;

extern "C"
{
  void
  TIM2_IRQHandler()
  {
    if (timer_ptr)
      timer_ptr->handle_irq();
    else
      // Fallback: clear interrupt flag to prevent lockup
      CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
  }

  void
  EXTI0_IRQHandler()
  {
    if (gpio_ptr)
      gpio_ptr->handle_irq(0, 0);
    else
      // Fallback: clear interrupt flag to prevent lockup
      EXTI->PR = (1 << 0);
  }
}

/**
 * @brief Application context to be used in callbacks
 */
struct AppContext
{
  /**
   * @brief LED status
   *
   */
  bool led_on = false;

  /**
   * @brief Whether blinking is enabled
   *
   */
  bool blink_on = false;

  /**
   * @brief Pointer to LED pin
   */
  Embys::Stm32::Gpio::Pin *led = nullptr;

  /**
   * @brief Pointer to blink event
   */
  Embys::Stm32::Base::Event *blink_event = nullptr;
};

/**
 * @brief A callback to toggle LED state
 *
 * @param context
 */
void
toggle_led(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);

  ctx->led_on = !ctx->led_on;

  if (ctx->led_on)
  {
    ctx->led->write(1);
    SIM_LOG("LED ON");
  }
  else
  {
    ctx->led->write(0);
    SIM_LOG("LED OFF");
  }
}

/**
 * @brief A callback to toggle blinking
 *
 * @param context
 * @param value
 */
void
toggle_btn(void *context, uint8_t value)
{
  SIM_LOG("Button state changed: " << (value ? "RELEASED" : "PRESSED"));
  auto *ctx = static_cast<AppContext *>(context);

  // Toggle blinking on button release (button is active-low)
  if (value)
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
      SIM_LOG("Blinking OFF");
    }
  }
}

int
main()
{
  // Define aliases
  using GpioMode = Embys::Stm32::Gpio::Mode;
  using GpioCnf = Embys::Stm32::Gpio::Cnf;
  using PinCfg = Embys::Stm32::Gpio::PinCfg;

  // Application context with pointers to be used in callbacks
  AppContext context;

  // Reset simulation environment, if running in simulation
  SIM_RESET();

  // Initialize timer and set global pointer for interrupt handler
  Embys::Stm32::Base::Timer timer(TIM2);
  timer_ptr = &timer;

  // Allocate memory for events:
  // - Blink event
  // - Stop event (built into the loop instance)
  constexpr size_t events_capacity = 2;
  Embys::Stm32::Base::Event *event_slots[events_capacity];
  Embys::Stm32::Base::Event *active_event_slots[events_capacity];
  // Allocate memory for modules:
  // - GPIO module
  constexpr size_t modules_capacity = 1;
  Embys::Stm32::Base::Module module_slots[modules_capacity];

  // Initialize the main loop instance
  Embys::Stm32::Base::Loop loop(&timer, event_slots, active_event_slots,
                                events_capacity, module_slots,
                                modules_capacity);

  // Initialize blink event with a callback to toggle LED state
  Embys::Stm32::Base::Event blink_event(&loop, Embys::Stm32::Base::EV_PERSIST,
                                        {toggle_led, &context});

  // Allocate memory for GPIO pins:
  // - One pin for button input
  // - One pin for LED output
  constexpr size_t gpio_pins_capacity = 2;
  Embys::Stm32::Gpio::Pin *gpio_pin_slots[gpio_pins_capacity];

  // Initialize GPIO bus instance
  Embys::Stm32::Gpio::Bus gpio_bus(&loop, gpio_pin_slots, gpio_pins_capacity);
  gpio_ptr = &gpio_bus;

  // Initialize button pin at PA0 (Input floating with IRQ)
  Embys::Stm32::Gpio::Pin button_pin(&gpio_bus, GPIOA, 0, GpioMode::IN,
                                     GpioCnf::IN_FL, PinCfg::IRQ);
  button_pin.set_callback({toggle_btn, &context});

  // Initialize LED pin at PC13 (Output push-pull, 2 MHz)
  Embys::Stm32::Gpio::Pin led_pin(&gpio_bus, GPIOC, 13, GpioMode::OUT_2,
                                  GpioCnf::OUT_PP, PinCfg::NONE);

  // Set up context
  context.blink_event = &blink_event;
  context.led = &led_pin;

  // Enable peripherals before starting main loop
  TRY(gpio_bus.enable());
  TRY(button_pin.enable());
  TRY(led_pin.enable());

  // Enable interrupts
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0x00);
  __NVIC_EnableIRQ(EXTI0_IRQn);

  // Start main loop
  loop.run();

  return 0;
}
