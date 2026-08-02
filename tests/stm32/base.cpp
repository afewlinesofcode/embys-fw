#include <array>
#include <chrono>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

struct EventCalls
{
  std::array<uint32_t, 4> values{};
  size_t size = 0;

  static void
  record(void *context) noexcept
  {
    auto *calls = static_cast<EventCalls *>(context);
    calls->values[calls->size++] = static_cast<uint32_t>(DWT->CYCCNT);
  }
};

struct CallbackTarget
{
  uint8_t value = 0;

  void
  increment(uint8_t amount) noexcept
  {
    value = static_cast<uint8_t>(value + amount);
  }
};

static void
increment_context(void *context, uint8_t amount) noexcept
{
  auto *value = static_cast<uint8_t *>(context);
  *value = static_cast<uint8_t>(*value + amount);
}

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
      timer_ptr->handle_irq(); // Call the timer's callback
  }
};

/**
 * @brief Fixture for testing loop functionality with events.
 */
struct Stm32BaseLoopFixture : Stm32BaseFixture
{
  static constexpr size_t events_capacity = 5;
  static constexpr size_t modules_capacity = 1;

  Embys::Stm32::Base::Timer timer;
  Embys::Stm32::Base::Loop<events_capacity, modules_capacity> loop;

  Stm32BaseLoopFixture() : timer(TIM2), loop(timer)
  {
    timer_ptr = &timer;
  }
};

TEST_SUITE("base")
{

  TEST_CASE("Callback is a two-word noexcept callable")
  {
    using ByteCallback = Embys::Callback<uint8_t>;

    static_assert(sizeof(ByteCallback) == 2 * sizeof(void *));
    static_assert(std::is_nothrow_invocable_v<ByteCallback &, uint8_t>);
    static_assert(noexcept(ByteCallback::bind<&CallbackTarget::increment>(
        std::declval<CallbackTarget &>())));

    ByteCallback empty;
    CHECK(empty.empty());
    empty(1);

    uint8_t value = 2;
    ByteCallback callback{increment_context, &value};
    callback(3);
    CHECK(value == 5);
    CHECK(callback == ByteCallback{increment_context, &value});

    CallbackTarget target;
    auto bound = ByteCallback::bind<&CallbackTarget::increment>(target);
    bound(4);
    CHECK(target.value == 4);

    callback.clear();
    CHECK(callback.empty());
    CHECK(callback != bound);
  }

  TEST_CASE("Event modes compose as independent flags")
  {
    using Embys::Stm32::Base::EventMode;
    using Embys::Stm32::Base::has_mode;

    constexpr auto mode = EventMode::Persistent | EventMode::Realtime;

    static_assert(has_mode(mode, EventMode::Persistent));
    static_assert(has_mode(mode, EventMode::Realtime));
    static_assert(!has_mode(EventMode::Deferred, EventMode::Persistent));
  }

  TEST_CASE_FIXTURE(Stm32BaseLoopFixture,
                    "Event rejects durations outside timer representation")
  {
    Embys::Stm32::Base::Event event(
        loop, Embys::Stm32::Base::EventMode::Deferred, {});

    const auto negative = event.enable(std::chrono::microseconds{-1});
    CHECK(negative.error() == Embys::Stm32::Base::EventError::InvalidDuration);
    CHECK_FALSE(event.pending);

    constexpr auto too_large = static_cast<std::chrono::microseconds::rep>(
                                   std::numeric_limits<uint32_t>::max()) +
                               1;
    const auto out_of_range =
        event.enable(std::chrono::microseconds{too_large});
    CHECK(out_of_range.error() ==
          Embys::Stm32::Base::EventError::InvalidDuration);
    CHECK_FALSE(event.pending);
  }

  TEST_CASE_FIXTURE(Stm32BaseLoopFixture,
                    "Loop remains stoppable after invalid stop duration")
  {
    const auto invalid = loop.stop(std::chrono::microseconds{-1});
    CHECK(invalid.error() == Embys::Stm32::Base::EventError::InvalidDuration);
    CHECK(loop.stop(std::chrono::microseconds{1}).has_value());
    loop.run();
    CHECK(DWT->CYCCNT == 72);
  }

  TEST_CASE_FIXTURE(Stm32BaseFixture,
                    "Destroying an event releases its scheduler slot")
  {
    Embys::Stm32::Base::Timer timer(TIM2);
    Embys::Stm32::Base::Loop<1, 1> loop(timer);

    {
      Embys::Stm32::Base::Event first(
          loop, Embys::Stm32::Base::EventMode::Deferred, {});
      CHECK(first.enable(std::chrono::microseconds{1}).has_value());
    }

    Embys::Stm32::Base::Event second(
        loop, Embys::Stm32::Base::EventMode::Deferred, {});
    CHECK(second.enable(std::chrono::microseconds{1}).has_value());
  }

  TEST_CASE_FIXTURE(Stm32BaseFixture,
                    "A full scheduler reports its typed capacity error")
  {
    Embys::Stm32::Base::Timer timer(TIM2);
    Embys::Stm32::Base::Loop<1, 1> loop(timer);
    Embys::Stm32::Base::Event first(
        loop, Embys::Stm32::Base::EventMode::Deferred, {});
    Embys::Stm32::Base::Event second(
        loop, Embys::Stm32::Base::EventMode::Deferred, {});

    CHECK(first.enable(std::chrono::microseconds{1}).has_value());
    const auto result = second.enable(std::chrono::microseconds{1});
    CHECK(result.error() == Embys::Stm32::Base::EventError::CapacityExceeded);
  }

  TEST_CASE_FIXTURE(Stm32BaseFixture,
                    "Initialize loop instance, run and stop after 10us")
  {
    // Create a timer instance (using TIM2 for testing)
    Embys::Stm32::Base::Timer timer(TIM2);
    timer_ptr = &timer;

    constexpr size_t events_capacity = 5;
    constexpr size_t modules_capacity = 1;
    Embys::Stm32::Base::Loop<events_capacity, modules_capacity> loop(timer);

    // Verify that DWT cycle counter is initialized to 0
    CHECK(DWT->CYCCNT == 0);
    // Schedule loop to stop after 10 microseconds
    (void)loop.stop(std::chrono::microseconds{10});
    // Run the loop, it should stop after 10 microseconds
    loop.run();
    // Verify that 720 cycles have elapsed (10 us at 72 MHz)
    CHECK(DWT->CYCCNT == 720);
  }

  TEST_CASE_FIXTURE(
      Stm32BaseLoopFixture,
      "Run loop with a single event scheduled after 5us, stop after 10us")
  {
    EventCalls event_calls;

    // Schedule an event to run after 5 microseconds
    Embys::Stm32::Base::Event event(loop,
                                    Embys::Stm32::Base::EventMode::Deferred,
                                    {EventCalls::record, &event_calls});
    (void)event.enable(std::chrono::microseconds{5});

    // Schedule loop to stop after 10 microseconds
    (void)loop.stop(std::chrono::microseconds{10});
    // Run the loop, it should process the event and then stop
    loop.run();

    // Verify that 720 cycles have elapsed (10 us at 72 MHz)
    CHECK(DWT->CYCCNT == 720);
    REQUIRE(event_calls.size == 1);
    // Verify that the event was called after 360 cycles (5 us)
    CHECK(event_calls.values[0] == 360);
  }

  TEST_CASE_FIXTURE(
      Stm32BaseLoopFixture,
      "Run loop with a single persisted event scheduled every 5us, "
      "stop after 16us")
  {
    EventCalls event_calls;

    // Schedule a persisted event to run every 5 microseconds
    Embys::Stm32::Base::Event event(loop,
                                    Embys::Stm32::Base::EventMode::Persistent,
                                    {EventCalls::record, &event_calls});
    (void)event.enable(std::chrono::microseconds{5});

    // Schedule loop to stop after 16 microseconds
    (void)loop.stop(std::chrono::microseconds{16});
    // Run the loop, it should process the event multiple times and then stop
    loop.run();

    // Verify that 1152 cycles have elapsed (16 us at 72 MHz)
    CHECK(DWT->CYCCNT == 1152);
    REQUIRE(event_calls.size == 3);
    // Verify that the event was called at approximately 360, 720, and 1080
    // cycles
    CHECK(event_calls.values[0] == 360);
    CHECK(event_calls.values[1] == 720);
    CHECK(event_calls.values[2] == 1080);
  }

  TEST_CASE_FIXTURE(
      Stm32BaseLoopFixture,
      "Run loop with two persisted events scheduled every 5us and "
      "10us, stop after "
      "20us")
  {
    EventCalls event1_calls;
    EventCalls event2_calls;

    // Schedule two events to run at different intervals
    Embys::Stm32::Base::Event event1(loop,
                                     Embys::Stm32::Base::EventMode::Persistent,
                                     {EventCalls::record, &event1_calls});
    Embys::Stm32::Base::Event event2(loop,
                                     Embys::Stm32::Base::EventMode::Persistent,
                                     {EventCalls::record, &event2_calls});
    (void)event1.enable(std::chrono::microseconds{5});  // Every 5 us
    (void)event2.enable(std::chrono::microseconds{10}); // Every 10 us

    // Schedule loop to stop after 20 microseconds
    (void)loop.stop(std::chrono::microseconds{20});
    // Run the loop, it should process both events and then stop
    loop.run();

    // Verify that 1440 cycles have elapsed (20 us at 72 MHz)
    CHECK(DWT->CYCCNT == 1440);
    REQUIRE(event1_calls.size == 4);
    REQUIRE(event2_calls.size == 2);
    CHECK(event1_calls.values[0] == 360);
    CHECK(event1_calls.values[1] == 720);
    CHECK(event1_calls.values[2] == 1080);
    CHECK(event1_calls.values[3] == 1440);
    CHECK(event2_calls.values[0] == 720);
    CHECK(event2_calls.values[1] == 1440);
  }

} // TEST_SUITE("base")
