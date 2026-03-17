#include <iostream>
#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

/**
 * @brief Base fixture for testing loop initialization and basic timer
 * functionality.
 */
struct Stm32BaseFixture
{
  Stm32BaseFixture()
  {
    Embys::Stm32::Sim::reset();
    timer_ptr = nullptr;
    Embys::Stm32::Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
  }

  inline static Embys::Stm32::Base::Timer *timer_ptr;

  static void
  TIM2_IRQHandler()
  {
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF); // Clear interrupt flag
    if (timer_ptr)
      (*timer_ptr)(); // Call the timer's callback
  }
};

/**
 * @brief Fixture for testing loop functionality with events.
 */
struct Stm32BaseLoopFixture : Stm32BaseFixture
{
  static constexpr size_t events_capacity = 5;
  Embys::Stm32::Base::Event *event_slots[events_capacity];
  Embys::Stm32::Base::Event *active_event_slots[events_capacity];
  static constexpr size_t modules_capacity = 1;
  Embys::Stm32::Base::Module module_slots[modules_capacity];

  Embys::Stm32::Base::Timer timer;
  Embys::Stm32::Base::Loop loop;

  Stm32BaseLoopFixture()
    : timer(TIM2), loop(&timer, event_slots, active_event_slots,
                        events_capacity, module_slots, modules_capacity)
  {
    timer_ptr = &timer;
  }
};

TEST_CASE_FIXTURE(Stm32BaseFixture,
                  "Initialize loop instance, run and stop after 10us")
{
  // Create a timer instance (using TIM2 for testing)
  Embys::Stm32::Base::Timer timer(TIM2);
  timer_ptr = &timer;

  // Allocate event slots and module slots
  constexpr size_t events_capacity = 5;
  Embys::Stm32::Base::Event *event_slots[events_capacity];
  Embys::Stm32::Base::Event *active_event_slots[events_capacity];
  constexpr size_t modules_capacity = 1;
  Embys::Stm32::Base::Module module_slots[modules_capacity];

  Embys::Stm32::Base::Loop loop(&timer, event_slots, active_event_slots,
                                events_capacity, module_slots,
                                modules_capacity);

  // Verify that DWT cycle counter is initialized to 0
  CHECK(DWT->CYCCNT == 0);
  // Schedule loop to stop after 10 microseconds
  loop.stop(10);
  // Run the loop, it should stop after 10 microseconds
  loop.run();
  // Verify that 720 cycles have elapsed (10 us at 72 MHz)
  CHECK(DWT->CYCCNT == 720);
}

TEST_CASE_FIXTURE(
    Stm32BaseLoopFixture,
    "Run loop with a single event scheduled after 5us, stop after 10us")
{
  std::vector<uint32_t> event_calls;

  auto callback = [](void *context)
  {
    auto *calls = static_cast<std::vector<uint32_t> *>(context);
    calls->push_back((uint32_t)DWT->CYCCNT);
  };

  // Schedule an event to run after 5 microseconds
  Embys::Stm32::Base::Event event(&loop, 0, {callback, &event_calls});
  event.enable(5);

  // Schedule loop to stop after 10 microseconds
  loop.stop(10);
  // Run the loop, it should process the event and then stop
  loop.run();

  // Verify that 720 cycles have elapsed (10 us at 72 MHz)
  CHECK(DWT->CYCCNT == 720);
  REQUIRE(event_calls.size() == 1);
  // Verify that the event was called after 360 cycles (5 us)
  CHECK(event_calls[0] == 360);
}

TEST_CASE_FIXTURE(Stm32BaseLoopFixture,
                  "Run loop with a single persisted event scheduled every 5us, "
                  "stop after 16us")
{
  std::vector<uint32_t> event_calls;

  auto callback = [](void *context)
  {
    auto *calls = static_cast<std::vector<uint32_t> *>(context);
    calls->push_back((uint32_t)DWT->CYCCNT);
  };

  // Schedule a persisted event to run every 5 microseconds
  Embys::Stm32::Base::Event event(&loop, Embys::Stm32::Base::EV_PERSIST,
                                  {callback, &event_calls});
  event.enable(5);

  // Schedule loop to stop after 16 microseconds
  loop.stop(16);
  // Run the loop, it should process the event multiple times and then stop
  loop.run();

  // Verify that 1152 cycles have elapsed (16 us at 72 MHz)
  CHECK(DWT->CYCCNT == 1152);
  REQUIRE(event_calls.size() == 3);
  // Verify that the event was called at approximately 360, 720, and 1080 cycles
  CHECK(event_calls[0] == 360);
  CHECK(event_calls[1] == 720);
  CHECK(event_calls[2] == 1080);
}

TEST_CASE_FIXTURE(Stm32BaseLoopFixture,
                  "Run loop with two persisted events scheduled every 5us and "
                  "10us, stop after "
                  "20us")
{
  std::vector<uint32_t> event1_calls;
  std::vector<uint32_t> event2_calls;

  auto callback1 = [](void *context)
  {
    auto *calls = static_cast<std::vector<uint32_t> *>(context);
    calls->push_back((uint32_t)DWT->CYCCNT);
  };

  auto callback2 = [](void *context)
  {
    auto *calls = static_cast<std::vector<uint32_t> *>(context);
    calls->push_back((uint32_t)DWT->CYCCNT);
  };

  // Schedule two events to run at different intervals
  Embys::Stm32::Base::Event event1(&loop, Embys::Stm32::Base::EV_PERSIST,
                                   {callback1, &event1_calls});
  Embys::Stm32::Base::Event event2(&loop, Embys::Stm32::Base::EV_PERSIST,
                                   {callback2, &event2_calls});
  event1.enable(5);  // Every 5 us
  event2.enable(10); // Every 10 us

  // Schedule loop to stop after 20 microseconds
  loop.stop(20);
  // Run the loop, it should process both events and then stop
  loop.run();

  // Verify that 1440 cycles have elapsed (20 us at 72 MHz)
  CHECK(DWT->CYCCNT == 1440);
  REQUIRE(event1_calls.size() == 4);
  REQUIRE(event2_calls.size() == 2);
  CHECK(event1_calls[0] == 360);
  CHECK(event1_calls[1] == 720);
  CHECK(event1_calls[2] == 1080);
  CHECK(event1_calls[3] == 1440);
  CHECK(event2_calls[0] == 720);
  CHECK(event2_calls[1] == 1440);
}
