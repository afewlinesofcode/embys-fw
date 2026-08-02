#include <array>
#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c/bus.hpp>
#include <embys/stm32/i2c/def.hpp>
#include <embys/stm32/i2c/hal.hpp>
#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

namespace Sim = Embys::Stm32::Sim;
using namespace Embys::Stm32;

static_assert(I2c::instance_available<Stm32f103xb, I2c::Instance::I2c2>);
static_assert(!I2c::instance_available<Stm32f103xb, I2c::Instance::I2c3>);
static_assert(I2c::instance_available<Stm32f407xx, I2c::Instance::I2c3>);
static_assert(I2c::instance_available<Stm32f411xe, I2c::Instance::I2c3>);
using Embys::Callback;

// ── fixtures ──────────────────────────────────────────────────────────────

struct I2cBaseFixture
{
  inline static I2c::BusCore *i2c_bus_ptr = nullptr;
  inline static Base::Timer *timer_ptr = nullptr;

  static void
  TIM2_IRQHandler()
  {
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
    if (timer_ptr)
      timer_ptr->handle_irq();
  }

  static void
  I2C1_EV_IRQHandler()
  {
    if (i2c_bus_ptr)
      i2c_bus_ptr->handle_ev_irq();
  }

  static void
  I2C1_ER_IRQHandler()
  {
    if (i2c_bus_ptr)
      i2c_bus_ptr->handle_er_irq();
  }

  I2cBaseFixture()
  {
    Sim::reset();
    timer_ptr = nullptr;
    i2c_bus_ptr = nullptr;
    Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
    Sim::I2C1_EV_IRQHandler_ptr = I2C1_EV_IRQHandler;
    Sim::I2C1_ER_IRQHandler_ptr = I2C1_ER_IRQHandler;
  }
};

struct I2cLoopFixture : I2cBaseFixture
{
  // events_capacity: 1 for Bus::timeout_event + 1 for loop.stop()
  static constexpr size_t events_capacity = 2;
  static constexpr size_t modules_capacity = 1;

  Base::Timer timer;
  Base::Loop<events_capacity, modules_capacity> loop;
  I2c::Bus<I2c::Instance::I2c1, 16, 16> bus;

  I2cLoopFixture() : timer(TIM2), loop(timer), bus(loop)
  {
    timer_ptr = &timer;
    i2c_bus_ptr = &bus;
  }
};

struct ReadCapture
{
  bool called = false;
  I2c::Error error = I2c::Error::InvalidState;
  std::array<uint8_t, 16> data{};
  size_t size = 0;

  static void
  callback(void *context, I2c::ReadResult result) noexcept
  {
    auto *capture = static_cast<ReadCapture *>(context);
    capture->called = true;
    if (!result)
    {
      capture->error = result.error();
      return;
    }

    const std::span<const uint8_t> data = result.value();
    capture->size = data.size();
    for (size_t i = 0; i < data.size(); ++i)
      capture->data[i] = data[i];
  }
};

struct StatusCapture
{
  bool called = false;
  bool succeeded = false;
  I2c::Error error = I2c::Error::InvalidState;

  static void
  callback(void *context, I2c::Status result) noexcept
  {
    auto *capture = static_cast<StatusCapture *>(context);
    capture->called = true;
    capture->succeeded = result.has_value();
    if (!result)
      capture->error = result.error();
  }
};

// ── enable_i2c ────────────────────────────────────────────────────────────

TEST_SUITE("i2c")
{

  TEST_CASE_FIXTURE(
      I2cBaseFixture,
      "enable_i2c: enables APB1 clock and sets PE, ITEVTEN, ITERREN")
  {
    CHECK(I2c::enable_i2c(I2C1, 100000u).has_value());

    CHECK((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) != 0);
    CHECK((I2C1->CR1 & I2C_CR1_PE) != 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITEVTEN) != 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITERREN) != 0);
    // ITBUFEN must be off — enabled on demand by the state machine
    CHECK((I2C1->CR2 & I2C_CR2_ITBUFEN) == 0);
  }

  TEST_CASE_FIXTURE(I2cBaseFixture, "enable_i2c: rejects an unknown peripheral")
  {
    const I2c::Status result = I2c::enable_i2c(nullptr, 100000u);
    REQUIRE(!result);
    CHECK(result.error() == I2c::Error::InvalidInstance);
  }

  TEST_CASE_FIXTURE(
      I2cBaseFixture,
      "enable_i2c: standard mode CCR matches ceil(pclk / (2*scl))")
  {
    constexpr uint32_t scl_hz = 100000u;
    REQUIRE(I2c::enable_i2c(I2C1, scl_hz));

    uint32_t pclk_hz = SystemCoreClock / 2u;
    uint32_t expected_ccr = (pclk_hz + (2u * scl_hz - 1u)) / (2u * scl_hz);
    CHECK((I2C1->CCR & ~I2C_CCR_FS) == expected_ccr);
    CHECK((I2C1->CCR & I2C_CCR_FS) == 0); // standard mode
  }

  TEST_CASE_FIXTURE(I2cBaseFixture, "enable_i2c: fast mode sets FS bit in CCR")
  {
    REQUIRE(I2c::enable_i2c(I2C1, 400000u));

    CHECK((I2C1->CCR & I2C_CCR_FS) != 0);
  }

  // ── disable_i2c ───────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cBaseFixture,
                    "disable_i2c: clears APB1 clock and disables interrupts")
  {
    REQUIRE(I2c::enable_i2c(I2C1, 100000u));
    CHECK(I2c::disable_i2c(I2C1));

    CHECK((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) == 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITEVTEN) == 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITERREN) == 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITBUFEN) == 0);
  }

  // ── Bus::enable / disable ─────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus::enable registers module and reports is_enabled")
  {
    CHECK(!bus.is_enabled());
    CHECK(bus.enable(100000u));
    CHECK(bus.is_enabled());
    CHECK((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) != 0);
    CHECK((I2C1->CR1 & I2C_CR1_PE) != 0);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture, "Bus::enable is idempotent")
  {
    REQUIRE(bus.enable(100000u));
    uint32_t ccr = I2C1->CCR;

    REQUIRE(bus.enable(400000u)); // second call — no-op
    CHECK(I2C1->CCR == ccr);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus::disable deregisters module and disables peripheral")
  {
    REQUIRE(bus.enable(100000u));
    CHECK(bus.disable());
    CHECK(!bus.is_enabled());
    CHECK((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) == 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITEVTEN) == 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITERREN) == 0);
  }

  // ── write ─────────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus::write reports not enabled when the bus is disabled")
  {
    const uint8_t data[] = {0x01};
    const I2c::Status result = bus.write(0x50u, std::span{data}.first<1>(), {});
    REQUIRE(!result);
    CHECK(result.error() == I2c::Error::NotEnabled);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: write completes successfully, callback receives zero")
  {
    REQUIRE(bus.enable(100000u));

    StatusCapture result;
    auto cb = I2c::BusCore::WriteCallback{StatusCapture::callback, &result};

    const uint8_t data[] = {0xDE, 0xAD};
    CHECK(bus.write(0x50u, data, cb));

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    CHECK(result.called);
    CHECK(result.succeeded);
  }

  TEST_CASE_FIXTURE(
      I2cLoopFixture,
      "Bus::write returns INVALID_STATE while a transaction is in progress")
  {
    REQUIRE(bus.enable(100000u));

    StatusCapture r1;
    StatusCapture r2;
    auto cb1 = I2c::BusCore::WriteCallback{StatusCapture::callback, &r1};
    auto cb2 = I2c::BusCore::WriteCallback{StatusCapture::callback, &r2};

    const uint8_t data[] = {0x01};
    REQUIRE(bus.write(0x50u, data, cb1));
    const I2c::Status result = bus.write(0x50u, data, cb2);
    REQUIRE(!result);
    CHECK(result.error() == I2c::Error::InvalidState);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus::write rejects data larger than owned TX storage")
  {
    REQUIRE(bus.enable(100000u));
    uint8_t data[17] = {};
    const I2c::Status result = bus.write(0x50u, data, {});
    REQUIRE(!result);
    CHECK(result.error() == I2c::Error::BufferTooSmall);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus::write owns data for the asynchronous transfer")
  {
    REQUIRE(bus.enable(100000u));
    uint8_t data[] = {0x12, 0x34};
    REQUIRE(bus.write(0x50u, data, {}));
    data[0] = 0xFF;
    data[1] = 0xFF;

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    REQUIRE(!Sim::I2C::tx_buffers.empty());
    CHECK(Sim::I2C::tx_buffers.back() == std::vector<uint8_t>({0x12, 0x34}));
  }

  // ── read ──────────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: single byte read delivers correct value, "
                    "callback receives zero result")
  {
    REQUIRE(bus.enable(100000u));

    Sim::I2C::simulate_rx({0xAB});

    ReadCapture capture;

    CHECK(bus.read(0x50u, 1u, {ReadCapture::callback, &capture}));

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    CHECK(capture.called);
    CHECK(capture.data[0] == 0xABu);
  }

  // 2-byte path: POS flag set before address, NACK applied to both bytes
  // via POS, STOP issued only after BTF (both bytes sit in DR+SR together).
  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: two-byte read (POS+BTF path) delivers both bytes")
  {
    REQUIRE(bus.enable(100000u));

    Sim::I2C::simulate_rx({0xCA, 0xFE});

    ReadCapture capture;

    CHECK(bus.read(0x50u, 2u, {ReadCapture::callback, &capture}));

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    CHECK(capture.called);
    CHECK(capture.data[0] == 0xCAu);
    CHECK(capture.data[1] == 0xFEu);
  }

  // 3-byte path: ACK through address, at byte N-3 (index 0) wait BTF,
  // then NACK+read+STOP+read, last byte arrives via RXNE.
  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: three-byte read (BTF at N-3 path) delivers all bytes")
  {
    REQUIRE(bus.enable(100000u));

    Sim::I2C::simulate_rx({0xA1, 0xB2, 0xC3});

    ReadCapture capture;

    CHECK(bus.read(0x50u, 3u, {ReadCapture::callback, &capture}));

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    CHECK(capture.called);
    CHECK(capture.data[0] == 0xA1u);
    CHECK(capture.data[1] == 0xB2u);
    CHECK(capture.data[2] == 0xC3u);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: multi-byte read delivers bytes in order")
  {
    REQUIRE(bus.enable(100000u));

    Sim::I2C::simulate_rx({0x11, 0x22, 0x33, 0x44});

    ReadCapture capture;

    CHECK(bus.read(0x50u, 4u, {ReadCapture::callback, &capture}));

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    CHECK(capture.called);
    CHECK(capture.data[0] == 0x11u);
    CHECK(capture.data[1] == 0x22u);
    CHECK(capture.data[2] == 0x33u);
    CHECK(capture.data[3] == 0x44u);
  }

  // ── register-addressed read ───────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: register-addressed read writes reg then reads bytes")
  {
    REQUIRE(bus.enable(100000u));

    Sim::I2C::simulate_rx({0x55, 0x66});

    ReadCapture capture;

    CHECK(bus.read(0x50u, 0x10u, 2u, {ReadCapture::callback, &capture}));

    (void)loop.stop(std::chrono::microseconds{100});
    loop.run();

    CHECK(capture.called);
    CHECK(capture.data[0] == 0x55u);
    CHECK(capture.data[1] == 0x66u);
  }

  // ── error handling ───────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: read returns BUS_BUSY when I2C bus is already busy")
  {
    REQUIRE(bus.enable(400000u));

    Sim::I2C::simulate_busy();

    ReadCapture capture;

    CHECK(bus.read(0x50u, 1u, {ReadCapture::callback, &capture}));

    (void)loop.stop(std::chrono::microseconds{500});
    loop.run();

    CHECK(capture.called);
    CHECK(capture.error == I2c::Error::BusBusy);
  }

} // TEST_SUITE("i2c")
