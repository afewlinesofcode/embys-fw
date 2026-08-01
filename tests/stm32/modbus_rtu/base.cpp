#include <cstring>
#include <vector>

#include <embys/stm32/base/loop.hpp>
#include <embys/stm32/modbus-rtu/base.hpp>
#include <embys/stm32/modbus/def.hpp>
#include <embys/stm32/sim/sim.hpp>
#include <embys/stm32/uart/bus.hpp>

#include "test.hpp"

using namespace Embys::Stm32;

namespace
{

// ── TestablBase ───────────────────────────────────────────────────────────
// Derives from Base to expose its protected members for white-box testing.

struct TestablBase : Modbus::Rtu::Base
{
  explicit TestablBase(Uart::BusCore *uart) : Modbus::Rtu::Base(uart)
  {
  }

  using Modbus::Rtu::Base::append_crc;
  using Modbus::Rtu::Base::buffer_in;
  using Modbus::Rtu::Base::buffer_in_len;
  using Modbus::Rtu::Base::buffer_in_overflow;
  using Modbus::Rtu::Base::buffer_out;
  using Modbus::Rtu::Base::buffer_out_len;
  using Modbus::Rtu::Base::calculate_crc;
  using Modbus::Rtu::Base::frame_delay_us;
  using Modbus::Rtu::Base::set_frame_in_callback;
  using Modbus::Rtu::Base::validate_crc;
};

// ── Fixtures ──────────────────────────────────────────────────────────────

struct RtuBaseFixture
{
  inline static Base::Timer *timer_ptr = nullptr;
  inline static Uart::BusCore *uart_bus_ptr = nullptr;

  static void
  TIM2_IRQHandler()
  {
    CLEAR_BIT_V(TIM2->SR, TIM_SR_UIF);
    if (timer_ptr)
      timer_ptr->handle_irq();
  }

  static void
  USART2_IRQHandler()
  {
    if (uart_bus_ptr)
      uart_bus_ptr->handle_irq();
  }

  RtuBaseFixture()
  {
    Sim::reset();
    Sim::Uart::usart = USART2;
    USART2->DR = 0;
    USART2->SR = USART_SR_TXE | USART_SR_TC;
    timer_ptr = nullptr;
    uart_bus_ptr = nullptr;
    Sim::TIM2_IRQHandler_ptr = TIM2_IRQHandler;
    Sim::USART2_IRQHandler_ptr = USART2_IRQHandler;
  }
};

static constexpr size_t kEventsCapacity = 5;
static constexpr size_t kModulesCapacity = 1;

struct RtuLoopFixture : RtuBaseFixture
{
  Base::Timer timer;
  Base::Loop<kEventsCapacity, kModulesCapacity> loop;
  Uart::Bus<Modbus::kFrameSize, Modbus::kFrameSize> uart;

  RtuLoopFixture()
    : timer(TIM2), loop(timer), uart(USART2, loop)
  {
    timer_ptr = &timer;
    uart_bus_ptr = &uart;
    uart.enable(9600);
  }
};

struct BaseFixture : RtuLoopFixture
{
  TestablBase base;

  BaseFixture() : base(&uart)
  {
    base.enable();
  }
};

struct BaseFixture38400 : RtuBaseFixture
{
  Base::Timer timer;
  Base::Loop<kEventsCapacity, kModulesCapacity> loop;
  Uart::Bus<Modbus::kFrameSize, Modbus::kFrameSize> uart;
  TestablBase base;

  BaseFixture38400()
    : timer(TIM2), loop(timer), uart(USART2, loop), base(&uart)
  {
    timer_ptr = &timer;
    uart_bus_ptr = &uart;
    uart.enable(38400);
    base.enable();
  }
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────

TEST_SUITE("modbus_rtu_base")
{

  // ── CRC calculation ───────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(BaseFixture, "calculate_crc: Modbus spec example "
                                 "{11 03 00 6B 00 03} returns 0x8776")
  {
    // Read 3 holding registers from device 17; from Modbus spec section 6.1.
    const uint8_t data[] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03};
    CHECK(base.calculate_crc(data, sizeof(data)) == 0x8776U);
  }

  // ── append_crc ────────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(BaseFixture,
                    "append_crc: increments length by 2 and writes CRC in "
                    "big-endian order")
  {
    const uint8_t payload[] = {0x11, 0x03, 0x00, 0x6B, 0x00, 0x03};
    memcpy(base.buffer_out, payload, sizeof(payload));
    base.buffer_out_len = static_cast<uint16_t>(sizeof(payload));

    base.append_crc(base.buffer_out, &base.buffer_out_len);

    // Length grows by exactly 2 bytes.
    CHECK(base.buffer_out_len == static_cast<uint16_t>(sizeof(payload)) + 2U);
    // CRC 0x8776: low byte 0x76 first, high byte 0x87 second (little-endian).
    CHECK(base.buffer_out[sizeof(payload)] == 0x76U);
    CHECK(base.buffer_out[sizeof(payload) + 1U] == 0x87U);
  }

  // ── validate_crc ──────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(BaseFixture,
                    "validate_crc: returns true for a correctly CRC-appended "
                    "frame")
  {
    const uint8_t payload[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
    memcpy(base.buffer_out, payload, sizeof(payload));
    base.buffer_out_len = static_cast<uint16_t>(sizeof(payload));
    base.append_crc(base.buffer_out, &base.buffer_out_len);

    CHECK(base.validate_crc(base.buffer_out, base.buffer_out_len));
  }

  TEST_CASE_FIXTURE(BaseFixture,
                    "validate_crc: returns false when a CRC byte is corrupted")
  {
    const uint8_t payload[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02};
    memcpy(base.buffer_out, payload, sizeof(payload));
    base.buffer_out_len = static_cast<uint16_t>(sizeof(payload));
    base.append_crc(base.buffer_out, &base.buffer_out_len);

    base.buffer_out[sizeof(payload)] ^= 0xFFU; // corrupt high CRC byte

    CHECK_FALSE(base.validate_crc(base.buffer_out, base.buffer_out_len));
  }

  // ── Frame timing ──────────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(BaseFixture,
                    "enable: calculates 3.5-character delay at 9600 baud 8N1")
  {
    // 8N1 = 10 bits/frame; char_time = 10*1 000 000/9600 = 1041 us (integer).
    // frame_delay = (1041 * 7) >> 1 = 3643 us.
    CHECK(base.frame_delay_us == 3643U);
  }

  TEST_CASE_FIXTURE(BaseFixture38400,
                    "enable: uses fixed 1750 us delay for baud rates above "
                    "19200 (Modbus spec section 2.5.1)")
  {
    CHECK(base.frame_delay_us == 1750U);
  }

  // ── RX frame reception ────────────────────────────────────────────────────

  TEST_CASE_FIXTURE(BaseFixture,
                    "rx: bytes accumulate in buffer_in and frame_in_cb fires "
                    "after the inter-frame gap")
  {
    base.override_frame_delay_us(400); // use short delay for faster tests

    bool cb_called = false;
    base.set_frame_in_callback(
        {[](void *ctx) { *static_cast<bool *>(ctx) = true; }, &cb_called});

    // Base does not validate CRC; any 4-byte payload triggers the callback.
    Sim::Uart::simulate_rx({0x01, 0x03, 0x00, 0x04});

    loop.stop(500);
    loop.run();

    CHECK(cb_called);
    CHECK(base.buffer_in_len == 4U);
    CHECK(base.buffer_in[0] == 0x01U);
    CHECK(base.buffer_in[1] == 0x03U);
  }

  TEST_CASE_FIXTURE(BaseFixture,
                    "rx: frame_in_cb is not called when frame is shorter than "
                    "4 bytes (2 header + 2 CRC minimum)")
  {
    bool cb_called = false;
    base.set_frame_in_callback(
        {[](void *ctx) { *static_cast<bool *>(ctx) = true; }, &cb_called});

    Sim::Uart::simulate_rx({0x01, 0x03, 0x00}); // 3 bytes — below minimum

    loop.stop(500);
    loop.run();

    CHECK_FALSE(cb_called);
  }

  TEST_CASE_FIXTURE(BaseFixture,
                    "rx: buffer overflow discards the frame and frame_in_cb "
                    "is not called")
  {
    base.override_frame_delay_us(400); // use short delay for faster tests
    bool cb_called = false;
    base.set_frame_in_callback(
        {[](void *ctx) { *static_cast<bool *>(ctx) = true; }, &cb_called});

    // kFrameSize + 1 bytes triggers the overflow path in recv_callback.
    std::vector<uint8_t> oversized(Modbus::kFrameSize + 1U, 0xAAU);
    Sim::Uart::simulate_rx(oversized);

    loop.stop(500);
    loop.run();

    CHECK_FALSE(cb_called);
  }

} // TEST_SUITE("modbus_rtu_base")
