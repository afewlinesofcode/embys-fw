#include <array>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/gpio/bus.hpp>
#include <embys/stm32/gpio/pin.hpp>
#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

namespace Sim = Embys::Stm32::Sim;
using namespace Embys::Stm32;

static_assert(Gpio::port_available<Stm32f103xb, Gpio::Port::D>);
static_assert(!Gpio::port_available<Stm32f103xb, Gpio::Port::E>);
static_assert(Gpio::port_available<Stm32f407xx, Gpio::Port::G>);
static_assert(Gpio::port_available<Stm32f407xx, Gpio::Port::I>);
static_assert(!Gpio::port_available<Stm32f411xe, Gpio::Port::G>);
static_assert(!Gpio::port_available<Stm32f411xe, Gpio::Port::I>);
static_assert(Gpio::port_available<Stm32f411xe, Gpio::Port::H>);
static_assert(Gpio::config_valid<Gpio::PinCfg::IN | Gpio::PinCfg::PU>);
static_assert(!Gpio::config_valid<Gpio::PinCfg::IN | Gpio::PinCfg::PU |
                                  Gpio::PinCfg::PD>);

struct CallbackValues
{
  std::array<uint8_t, 2> values{};
  size_t size = 0;

  static void
  record(void *context, uint8_t value) noexcept
  {
    auto *calls = static_cast<CallbackValues *>(context);
    calls->values[calls->size++] = value;
  }
};

// ── fixtures ──────────────────────────────────────────────────────────────

struct GpioBaseFixture
{
  inline static Base::Timer *timer_ptr = nullptr;
  inline static Gpio::BusCore *gpio_bus_ptr = nullptr;

  static void
  TIM2_IRQHandler()
  {
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
    if (timer_ptr)
      timer_ptr->handle_irq();
  }

  static void
  EXTI0_IRQHandler()
  {
    if (gpio_bus_ptr)
      gpio_bus_ptr->handle_irq(0, 0);
  }

  static void
  EXTI1_IRQHandler()
  {
    if (gpio_bus_ptr)
      gpio_bus_ptr->handle_irq(1, 1);
  }

  GpioBaseFixture()
  {
    Sim::reset();
    timer_ptr = nullptr;
    gpio_bus_ptr = nullptr;
    Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
    Sim::EXTI0_IRQHandler_ptr = EXTI0_IRQHandler;
    Sim::EXTI1_IRQHandler_ptr = EXTI1_IRQHandler;
  }
};

struct GpioLoopFixture : GpioBaseFixture
{
  static constexpr size_t events_capacity = 3;
  static constexpr size_t modules_capacity = 1;
  static constexpr size_t pins_capacity = 3;

  Base::Timer timer;
  Base::Loop<events_capacity, modules_capacity> loop;
  Gpio::Bus<pins_capacity> bus;

  GpioLoopFixture() : timer(TIM2), loop(timer), bus(loop)
  {
    timer_ptr = &timer;
    gpio_bus_ptr = &bus;
  }
};

// ── helpers ───────────────────────────────────────────────────────────────

// Extract the 4-bit CRx nibble for the given pin index.
static uint32_t
read_cr_nibble(GPIO_TypeDef *port, uint8_t index)
{
  uint8_t shift = (index & 0x7u) << 2u;
  volatile uint32_t *cr = (index < 8) ? &port->CRL : &port->CRH;
  return (*cr >> shift) & 0xFu;
}

// ── config validation ─────────────────────────────────────────────────────

TEST_SUITE("gpio")
{

  TEST_CASE("Pin: compile-time validation rejects pull-up plus pull-down")
  {
    CHECK((Gpio::config_valid<Gpio::PinCfg::IN | Gpio::PinCfg::PU>));
    CHECK_FALSE((Gpio::config_valid<Gpio::PinCfg::IN | Gpio::PinCfg::PU |
                                    Gpio::PinCfg::PD>));
  }

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "ONLY Pin: enable accepts input pull-up via unified PinCfg")
  {
    bus.enable();

    // Unified PinCfg allows input pull-up directly without CNF coupling.
    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN | Gpio::PinCfg::PU> pin(bus);

    CHECK(pin.enable() == 0);
    CHECK(pin.is_enabled());
  }

  // ── hardware enable ───────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(
      GpioLoopFixture,
      "Pin: enable configures GPIO clock and CRL for input floating")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN> pin(bus);

    CHECK(pin.enable() == 0);

    // Clock enabled
    CHECK((RCC->APB2ENR & RCC_APB2ENR_IOPAEN) != 0);
    // CRL nibble matches IN + IN_FL = 0b0100
    CHECK(read_cr_nibble(GPIOA, 0) == 0b0100u); // IN + IN_FL nibble
  }

  TEST_CASE_FIXTURE(GpioLoopFixture, "Pin: enable with PULL_UP sets ODR bit")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 3, Gpio::PinCfg::IN | Gpio::PinCfg::PU> pin(bus);

    CHECK(pin.enable() == 0);
    CHECK((GPIOA->ODR & (1u << 3)) != 0);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Pin: enable with PULL_DOWN clears ODR bit")
  {
    bus.enable();

    // Pre-set the ODR bit to make sure the test actually clears it
    SET_BIT_V(GPIOA->ODR, (1u << 3));

    Gpio::Pin<Gpio::Port::A, 3, Gpio::PinCfg::IN | Gpio::PinCfg::PD> pin(bus);

    CHECK(pin.enable() == 0);
    CHECK((GPIOA->ODR & (1u << 3)) == 0);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture, "Pin: enable with IRQ configures EXTI")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN | Gpio::PinCfg::LISTEN> pin(
        bus);

    CHECK(pin.enable() == 0);

    // AFIO clock and EXTICR routing: EXTICR[0] bits [3:0] = 0 (GPIOA = port 0)
    CHECK((RCC->APB2ENR & RCC_APB2ENR_AFIOEN) != 0);
    CHECK((AFIO->EXTICR[0] & 0xFu) == 0u); // PA → port 0

    uint32_t pin_bit = (1u << 0);
    CHECK((EXTI->IMR & pin_bit) != 0);
    CHECK((EXTI->RTSR & pin_bit) != 0);
    CHECK((EXTI->FTSR & pin_bit) != 0);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture, "Pin: enable is idempotent")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN> pin(bus);

    CHECK(pin.enable() == 0);
    uint32_t crl_after_first = GPIOA->CRL;

    CHECK(pin.enable() == 0); // second call is no-op
    CHECK(GPIOA->CRL == crl_after_first);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Pin: enable returns BUS_NOT_ENABLED when Bus not enabled")
  {
    // bus.enable() deliberately NOT called

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN> pin(bus);

    CHECK(pin.enable() == Gpio::BUS_NOT_ENABLED);
    CHECK(!pin.is_enabled());
  }

  // ── disable ───────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Pin: disable resets pin to floating input")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::OUT> pin(bus);

    CHECK(pin.enable() == 0);
    CHECK(pin.disable() == 0);

    // CRL nibble should be 0b0100 = IN_FL (reset state)
    CHECK(read_cr_nibble(GPIOA, 0) == 0b0100u);
    CHECK((GPIOA->ODR & (1u << 0)) == 0);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture, "Pin: disable clears EXTI configuration")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN | Gpio::PinCfg::LISTEN> pin(
        bus);

    CHECK(pin.enable() == 0);
    CHECK(pin.disable() == 0);

    uint32_t pin_bit = (1u << 0);
    CHECK((EXTI->IMR & pin_bit) == 0);
    CHECK((EXTI->RTSR & pin_bit) == 0);
    CHECK((EXTI->FTSR & pin_bit) == 0);
  }

  TEST_CASE_FIXTURE(
      GpioLoopFixture,
      "Pin: disable disables GPIO clock when last pin on port removed")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN> pin(bus);

    CHECK(pin.enable() == 0);
    CHECK((RCC->APB2ENR & RCC_APB2ENR_IOPAEN) != 0);

    CHECK(pin.disable() == 0);
    CHECK((RCC->APB2ENR & RCC_APB2ENR_IOPAEN) == 0);
  }

  // ── I/O ───────────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(GpioLoopFixture, "Pin: read returns current IDR state")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 2, Gpio::PinCfg::IN> pin(bus);

    CHECK(pin.enable() == 0);

    SET_BIT_V(GPIOA->IDR, (1u << 2));
    uint8_t val = 0;
    CHECK(pin.read(&val) == 0);
    CHECK(val == 1);

    CLEAR_BIT_V(GPIOA->IDR, (1u << 2));
    CHECK(pin.read(&val) == 0);
    CHECK(val == 0);
  }

  TEST_CASE_FIXTURE(
      GpioLoopFixture,
      "Pin: write high uses BSRR set bits, write low uses BSRR reset bits")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 2, Gpio::PinCfg::OUT> pin(bus);

    CHECK(pin.enable() == 0);

    GPIOA->BSRR = 0;
    CHECK(pin.write(1) == 0);
    CHECK((GPIOA->BSRR & (1u << 2)) != 0); // set bit in lower half

    GPIOA->BSRR = 0;
    CHECK(pin.write(0) == 0);
    CHECK((GPIOA->BSRR & (1u << (2 + 16))) != 0); // reset bit in upper half
  }

  // ── bus capacity ──────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Bus: returns BUS_FULL when all slots are taken")
  {
    bus.enable();

    // Fill all 3 slots (GpioLoopFixture has pins_capacity = 3)
    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN> pin0(bus);
    Gpio::Pin<Gpio::Port::A, 1, Gpio::PinCfg::IN> pin1(bus);
    Gpio::Pin<Gpio::Port::A, 2, Gpio::PinCfg::IN> pin2(bus);

    CHECK(pin0.enable() == 0);
    CHECK(pin1.enable() == 0);
    CHECK(pin2.enable() == 0);

    // Fourth pin exceeds capacity
    Gpio::Pin<Gpio::Port::A, 3, Gpio::PinCfg::IN> pin3(bus);

    CHECK(pin3.enable() == Gpio::BUS_FULL);
  }

  // ── IRQ → callback dispatch ───────────────────────────────────────────────

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Pin: rising edge triggers callback with value=1")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN | Gpio::PinCfg::LISTEN> pin(
        bus);
    CHECK(pin.enable() == 0);

    CallbackValues calls;
    pin.set_callback({CallbackValues::record, &calls});

    // Drive PA0 high → EXTI0 fires → module marked interrupted
    Sim::Gpio::trigger_pin(GPIOA, 0, 1);
    for (int i = 0; i < 10; ++i)
      __NOP();

    // Run loop for 1 us to process the deferred module
    (void)loop.stop(std::chrono::microseconds{1});
    loop.run();

    REQUIRE(calls.size == 1);
    CHECK(calls.values[0] == 1);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Pin: falling edge triggers callback with value=0")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN | Gpio::PinCfg::LISTEN> pin(
        bus);
    CHECK(pin.enable() == 0);

    CallbackValues calls;
    pin.set_callback({CallbackValues::record, &calls});

    // Start high so the falling edge is a meaningful transition
    SET_BIT_V(GPIOA->IDR, (1u << 0));

    Sim::Gpio::trigger_pin(GPIOA, 0, 0);
    for (int i = 0; i < 10; ++i)
      __NOP();

    (void)loop.stop(std::chrono::microseconds{1});
    loop.run();

    REQUIRE(calls.size == 1);
    CHECK(calls.values[0] == 0);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Two pins: only the triggered pin's callback fires")
  {
    bus.enable();

    // PA0 → EXTI0, PA1 → EXTI1
    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN | Gpio::PinCfg::LISTEN> pin0(
        bus);
    Gpio::Pin<Gpio::Port::A, 1, Gpio::PinCfg::IN | Gpio::PinCfg::LISTEN> pin1(
        bus);

    CHECK(pin0.enable() == 0);
    CHECK(pin1.enable() == 0);

    CallbackValues calls0;
    CallbackValues calls1;

    pin0.set_callback({CallbackValues::record, &calls0});
    pin1.set_callback({CallbackValues::record, &calls1});

    // Only trigger PA0
    Sim::Gpio::trigger_pin(GPIOA, 0, 1);
    for (int i = 0; i < 10; ++i)
      __NOP();

    (void)loop.stop(std::chrono::microseconds{1});
    loop.run();

    REQUIRE(calls0.size == 1);
    CHECK(calls1.size == 0);
  }

  TEST_CASE_FIXTURE(GpioLoopFixture,
                    "Disabled pin does not trigger callback after disable")
  {
    bus.enable();

    Gpio::Pin<Gpio::Port::A, 0, Gpio::PinCfg::IN | Gpio::PinCfg::LISTEN> pin(
        bus);
    CHECK(pin.enable() == 0);

    int call_count = 0;
    pin.set_callback({[](void *ctx, uint8_t) noexcept
                      { ++*static_cast<int *>(ctx); }, &call_count});

    // First trigger — pin is still enabled
    Sim::Gpio::trigger_pin(GPIOA, 0, 1);
    for (int i = 0; i < 10; ++i)
      __NOP();
    (void)loop.stop(std::chrono::microseconds{1});
    loop.run();

    CHECK(call_count == 1);

    CHECK(pin.disable() == 0);

    // Second trigger — pin is disabled, no callback
    Sim::Gpio::trigger_pin(GPIOA, 0, 0);
    for (int i = 0; i < 10; ++i)
      __NOP();
    (void)loop.stop(std::chrono::microseconds{1});
    loop.run();

    CHECK(call_count == 1); // unchanged
  }

} // TEST_SUITE("gpio")
