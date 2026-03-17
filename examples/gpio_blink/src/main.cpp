/**
 * @file main.cpp
 * @author Stanislav Yaranov (stanislav.yaranov@gmail.com)
 * @brief A simple GPIO blinking example using the Base Loop and Timer
 * abstractions. This example demonstrates how to configure a GPIO pin for
 * output and toggle it at regular intervals using a timer and the main loop,
 * and how to print the LED state when built in the simulation environment.
 * @version 0.1
 * @date 2026-03-17
 * @copyright Copyright (c) 2026
 *
 */

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/base/timer.hpp>

/**
 * @brief Global pointer to the timer instance for use in the interrupt handler
 */
Embys::Stm32::Base::Timer *timer_ptr = nullptr;

void
toggle_led(void *context);

/**
 * @brief Timer interrupt handler for TIM2, clears interrupt flag,
 * and calls the timer's callback function.
 */
extern "C" void
TIM2_IRQHandler()
{
  CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF); // Clear interrupt flag

  if (timer_ptr)
    (*timer_ptr)(); // Call the timer's callback
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
  SET_BIT_V(GPIOC->CRH, GPIO_CRH_MODE13_1);
}

/**
 * @brief Toggles the state of the LED.
 *
 * @param context Pointer to a boolean representing the LED state.
 */
void
toggle_led(void *context)
{
  bool *led_state = static_cast<bool *>(context);

  if (*led_state)
  {
    // Reset pin to turn on LED (active low)
    SET_BIT_V(GPIOC->BSRR, GPIO_BSRR_BR13);

#ifdef STM32_SIM
    std::cout << "LED ON" << std::endl;
#endif
  }
  else
  {
    // Set pin to turn off LED (active low)
    SET_BIT_V(GPIOC->BSRR, GPIO_BSRR_BS13);

#ifdef STM32_SIM
    std::cout << "LED OFF" << std::endl;
#endif
  }

  *led_state = !*led_state;
}

/**
 * @brief Main function for the GPIO blinking example.
 *
 * @return int
 */
int
main()
{
#ifdef STM32_SIM
  // // Initialize simulation environment if in simulation mode
  Embys::Stm32::Sim::reset();
  Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
  // Register signal handler for graceful shutdown
  Embys::Stm32::Sim::register_int_signal();
#endif

  // Initialize timer instance and update global pointer for interrupt handler
  Embys::Stm32::Base::Timer timer(TIM2);
  timer_ptr = &timer;

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

  // Configure GPIO for LED control
  configure_led();

  // State variable to track LED status
  bool led_on = false;

  // Create an event to toggle the LED
  Embys::Stm32::Base::Event toggle_led_event(
      &loop, Embys::Stm32::Base::EV_PERSIST, {toggle_led, &led_on});

  // Interval for toggling the LED in microseconds
  uint32_t toggle_interval_us = 500000;

#ifdef STM32_SIM
  // Time in simulation is not real and much slower
  toggle_interval_us = 5000;
#endif

  // Enable the LED toggle event before starting the loop
  toggle_led_event.enable(toggle_interval_us);

  // Start the main loop, it will run indefinitely until stopped
  loop.run();

  return 0;
}
