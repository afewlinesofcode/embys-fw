#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/i2c/api.hpp>
#include <embys/stm32/i2c/bus.hpp>
#include <embys/stm32/i2c/def.hpp>
#include <embys/stm32/sim/sim.hpp>

#include "test.hpp"

namespace Sim = Embys::Stm32::Sim;
using namespace Embys::Stm32;
using Embys::Callable;

// ── fixtures ──────────────────────────────────────────────────────────────

struct I2cBaseFixture
{
  inline static I2c::Bus *i2c_bus_ptr = nullptr;
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

  Base::Event *event_slots[events_capacity];
  Base::Event *active_event_slots[events_capacity];
  Base::Module module_slots[modules_capacity];

  Base::Timer timer;
  Base::Loop loop;
  I2c::Bus bus;

  I2cLoopFixture()
    : timer(TIM2), loop(&timer, event_slots, active_event_slots,
                        events_capacity, module_slots, modules_capacity),
      bus(I2C1, &loop)
  {
    timer_ptr = &timer;
    i2c_bus_ptr = &bus;
  }
};

// ── enable_i2c ────────────────────────────────────────────────────────────

TEST_SUITE("i2c")
{

  TEST_CASE_FIXTURE(
      I2cBaseFixture,
      "enable_i2c: enables APB1 clock and sets PE, ITEVTEN, ITERREN")
  {
    CHECK(I2c::enable_i2c(I2C1, 100000u) == 0);

    CHECK((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) != 0);
    CHECK((I2C1->CR1 & I2C_CR1_PE) != 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITEVTEN) != 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITERREN) != 0);
    // ITBUFEN must be off — enabled on demand by the state machine
    CHECK((I2C1->CR2 & I2C_CR2_ITBUFEN) == 0);
  }

  TEST_CASE_FIXTURE(I2cBaseFixture,
                    "enable_i2c: returns INVALID_I2C for unknown peripheral")
  {
    CHECK(I2c::enable_i2c(nullptr, 100000u) == I2c::INVALID_I2C);
  }

  TEST_CASE_FIXTURE(
      I2cBaseFixture,
      "enable_i2c: standard mode CCR matches ceil(pclk / (2*scl))")
  {
    constexpr uint32_t scl_hz = 100000u;
    I2c::enable_i2c(I2C1, scl_hz);

    uint32_t pclk_hz = SystemCoreClock / 2u;
    uint32_t expected_ccr = (pclk_hz + (2u * scl_hz - 1u)) / (2u * scl_hz);
    CHECK((I2C1->CCR & ~I2C_CCR_FS) == expected_ccr);
    CHECK((I2C1->CCR & I2C_CCR_FS) == 0); // standard mode
  }

  TEST_CASE_FIXTURE(I2cBaseFixture, "enable_i2c: fast mode sets FS bit in CCR")
  {
    I2c::enable_i2c(I2C1, 400000u);

    CHECK((I2C1->CCR & I2C_CCR_FS) != 0);
  }

  // ── disable_i2c ───────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cBaseFixture,
                    "disable_i2c: clears APB1 clock and disables interrupts")
  {
    I2c::enable_i2c(I2C1, 100000u);
    CHECK(I2c::disable_i2c(I2C1) == 0);

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
    CHECK(bus.enable(100000u) == 0);
    CHECK(bus.is_enabled());
    CHECK((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) != 0);
    CHECK((I2C1->CR1 & I2C_CR1_PE) != 0);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture, "Bus::enable is idempotent")
  {
    bus.enable(100000u);
    uint32_t ccr = I2C1->CCR;

    bus.enable(400000u); // second call — no-op
    CHECK(I2C1->CCR == ccr);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus::disable deregisters module and disables peripheral")
  {
    bus.enable(100000u);
    CHECK(bus.disable() == 0);
    CHECK(!bus.is_enabled());
    CHECK((RCC->APB1ENR & RCC_APB1ENR_I2C1EN) == 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITEVTEN) == 0);
    CHECK((I2C1->CR2 & I2C_CR2_ITERREN) == 0);
  }

  // ── write ─────────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus::write returns BUS_NOT_ENABLED when not enabled")
  {
    const uint8_t data[] = {0x01};
    int result = -1;
    CHECK(bus.write(0x50u, data, 1u,
                    {[](void *ctx, int r) { *static_cast<int *>(ctx) = r; },
                     &result}) == I2c::BUS_NOT_ENABLED);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: write completes successfully, callback receives zero")
  {
    bus.enable(100000u);

    int result = -1;
    auto cb = Callable<int>{[](void *ctx, int r)
                            { *static_cast<int *>(ctx) = r; }, &result};

    const uint8_t data[] = {0xDE, 0xAD};
    CHECK(bus.write(0x50u, data, 2u, cb) == 0);

    loop.stop(500u);
    loop.run();

    CHECK(result == 0);
  }

  TEST_CASE_FIXTURE(
      I2cLoopFixture,
      "Bus::write returns INVALID_STATE while a transaction is in progress")
  {
    bus.enable(100000u);

    int r1 = -1, r2 = -1;
    auto cb1 = Callable<int>{[](void *ctx, int r)
                             { *static_cast<int *>(ctx) = r; }, &r1};
    auto cb2 = Callable<int>{[](void *ctx, int r)
                             { *static_cast<int *>(ctx) = r; }, &r2};

    const uint8_t data[] = {0x01};
    CHECK(bus.write(0x50u, data, 1u, cb1) == 0);
    CHECK(bus.write(0x50u, data, 1u, cb2) == I2c::INVALID_STATE);
  }

  // ── read ──────────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: single byte read delivers correct value, "
                    "callback receives zero result")
  {
    bus.enable(100000u);

    Sim::I2C::simulate_recv({0xAB});

    int result = -1;
    uint8_t buf[1] = {};
    auto cb = Callable<int>{[](void *ctx, int r)
                            { *static_cast<int *>(ctx) = r; }, &result};

    CHECK(bus.read(0x50u, buf, 1u, cb) == 0);

    loop.stop(500u);
    loop.run();

    CHECK(result == 0);
    CHECK(buf[0] == 0xABu);
  }

  // 2-byte path: POS flag set before address, NACK applied to both bytes
  // via POS, STOP issued only after BTF (both bytes sit in DR+SR together).
  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: two-byte read (POS+BTF path) delivers both bytes")
  {
    bus.enable(100000u);

    Sim::I2C::simulate_recv({0xCA, 0xFE});

    int result = -1;
    uint8_t buf[2] = {};
    auto cb = Callable<int>{[](void *ctx, int r)
                            { *static_cast<int *>(ctx) = r; }, &result};

    CHECK(bus.read(0x50u, buf, 2u, cb) == 0);

    loop.stop(500u);
    loop.run();

    CHECK(result == 0);
    CHECK(buf[0] == 0xCAu);
    CHECK(buf[1] == 0xFEu);
  }

  // 3-byte path: ACK through address, at byte N-3 (index 0) wait BTF,
  // then NACK+read+STOP+read, last byte arrives via RXNE.
  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: three-byte read (BTF at N-3 path) delivers all bytes")
  {
    bus.enable(100000u);

    Sim::I2C::simulate_recv({0xA1, 0xB2, 0xC3});

    int result = -1;
    uint8_t buf[3] = {};
    auto cb = Callable<int>{[](void *ctx, int r)
                            { *static_cast<int *>(ctx) = r; }, &result};

    CHECK(bus.read(0x50u, buf, 3u, cb) == 0);

    loop.stop(500u);
    loop.run();

    CHECK(result == 0);
    CHECK(buf[0] == 0xA1u);
    CHECK(buf[1] == 0xB2u);
    CHECK(buf[2] == 0xC3u);
  }

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: multi-byte read delivers bytes in order")
  {
    bus.enable(100000u);

    Sim::I2C::simulate_recv({0x11, 0x22, 0x33, 0x44});

    int result = -1;
    uint8_t buf[4] = {};
    auto cb = Callable<int>{[](void *ctx, int r)
                            { *static_cast<int *>(ctx) = r; }, &result};

    CHECK(bus.read(0x50u, buf, 4u, cb) == 0);

    loop.stop(500u);
    loop.run();

    CHECK(result == 0);
    CHECK(buf[0] == 0x11u);
    CHECK(buf[1] == 0x22u);
    CHECK(buf[2] == 0x33u);
    CHECK(buf[3] == 0x44u);
  }

  // ── register-addressed read ───────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: register-addressed read writes reg then reads bytes")
  {
    bus.enable(100000u);

    Sim::I2C::simulate_recv({0x55, 0x66});

    int result = -1;
    uint8_t buf[2] = {};
    auto cb = Callable<int>{[](void *ctx, int r)
                            { *static_cast<int *>(ctx) = r; }, &result};

    CHECK(bus.read(0x50u, 0x10u, buf, 2u, cb) == 0);

    loop.stop(500u);
    loop.run();

    CHECK(result == 0);
    CHECK(buf[0] == 0x55u);
    CHECK(buf[1] == 0x66u);
  }

  // ── error handling ───────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(I2cLoopFixture,
                    "Bus: read returns BUS_BUSY when I2C bus is already busy")
  {
    bus.enable(100000u);

    Sim::I2C::simulate_busy();

    uint8_t buf[1] = {};
    int dummy = 0;
    auto cb = Callable<int>{[](void *ctx, int r)
                            { *static_cast<int *>(ctx) = r; }, &dummy};

    CHECK(bus.read(0x50u, buf, 1u, cb) == I2c::BUS_BUSY);
  }

} // TEST_SUITE("i2c")
