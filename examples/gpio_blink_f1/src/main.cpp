/**
 * @file main.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief A simple GPIO blinking example using the Base Loop and Timer
 * abstractions for STM32F1. This example demonstrates how to configure a GPIO
 * pin for output and toggle it at regular intervals using a timer and the main
 * loop, and how to print the LED state when built in the simulation
 * environment.
 * @version 0.1
 * @date 2026-03-17
 * @copyright Copyright (c) 2026
 *
 */

#ifdef STM32F1xx

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/timer.hpp>

#include "def.hpp"
#include "sim.hpp"

/**
 * @brief Global pointer to the timer instance for use in the interrupt handler
 */
Embys::Stm32::Base::Timer *timer_ptr = nullptr;

/**
 * @brief Application context structure to hold state information for callbacks
 */
struct AppContext
{
  bool led_on = false;
};

/**
 * @brief Timer interrupt handler for TIM2.
 */
extern "C" void
TIM2_IRQHandler()
{
  if (timer_ptr)
    timer_ptr->handle_irq(); // Call the timer's callback
  else
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF); // Fallback: clear interrupt flag
}

/**
 * @brief Initializes GPIO pin for LED control.
 * Assumes LED is on GPIOC pin 13.
 */
void
configure_led()
{
  // Configure GPIO pin for LED (assuming it's on GPIOC pin 13)
  SET_BIT_V(RCC->APB2ENR, RCC_APB2ENR_IOPCEN);
  CLEAR_BIT_V(GPIOC->CRH, GPIO_CRH_MODE13 | GPIO_CRH_CNF13);
  // Output mode, max speed 2 MHz, push-pull
  SET_BIT_V(GPIOC->CRH, GPIO_CRH_MODE13_1);
  SET_BIT_V(GPIOC->BSRR, GPIO_BSRR_BS13);
}

/**
 * @brief Toggles the state of the LED.
 *
 * @param context Pointer to a boolean representing the LED state.
 */
void
toggle_led(void *context)
{
  auto *ctx = static_cast<AppContext *>(context);

  if (ctx->led_on)
  {
    // Reset pin to turn on LED (active low)
    SET_BIT_V(GPIOC->BSRR, GPIO_BSRR_BR13);

    SIM_LOG("LED ON");
  }
  else
  {
    // Set pin to turn off LED (active low)
    SET_BIT_V(GPIOC->BSRR, GPIO_BSRR_BS13);

    SIM_LOG("LED OFF");
  }

  ctx->led_on = !ctx->led_on;
}

/**
 * @brief Main function for the GPIO blinking example.
 *
 * @return int
 */
int
main()
{
  SIM_RESET();

  AppContext context;

  // Initialize timer instance and update global pointer for interrupt handler
  Embys::Stm32::Base::Timer timer(TIM2);

  // Allocate event slots and module slots for the loop
  constexpr size_t events_capacity = 5;
  Embys::Stm32::Base::Event *event_slots[events_capacity];
  Embys::Stm32::Base::Event *active_event_slots[events_capacity];
  constexpr size_t modules_capacity = 1;
  Embys::Stm32::Base::Module module_slots[modules_capacity];

  // Initialize the main loop with the timer and event/module slots
  Embys::Stm32::Base::Loop loop(&timer, event_slots, active_event_slots,
                                events_capacity, module_slots,
                                modules_capacity);

  // Create an event to toggle the LED
  Embys::Stm32::Base::Event toggle_led_event(
      &loop, Embys::Stm32::Base::EV_PERSIST, {toggle_led, &context});

  // Configure GPIO for LED control
  configure_led();

  // Set global pointer for timer interrupt handler
  timer_ptr = &timer;

  // Enable the LED toggle event before starting the loop
  toggle_led_event.enable(LED_BLINK_INTERVAL_US);

  // Enable interrupts
  __NVIC_EnableIRQ(TIM2_IRQn);
  __NVIC_SetPriority(TIM2_IRQn, 0x00);

  // Run the main loop
  loop.run();

  return 0;
}

#else

int
main()
{
  return 0;
}

#endif // STM32F1xx
